"""
Variance-Metric / Utilization Correlation Analysis.
"""

from sys import argv
from json import load
from os.path import join

import numpy as np
import pandas as pd
import matplotlib

matplotlib.use('QtAgg')
import matplotlib.pyplot as plt
from sklearn.linear_model import LassoCV

from definitions import Dataset, Task
from analyze_data import compute_job_metrics, get_readable_label

FILTER_TASK = Task.DISPARITY


# ── Normalization (mirrors select_features.normalize_data) ─────────────────────


def normalize_variance_features(df: pd.DataFrame, target_column: str = 'utilization') -> pd.DataFrame:
    """
    Mirrors select_features.normalize_data(): min-max scale each `_var`
    column to [0, 1], then weight it by utilization. This is the same
    preprocessing the original run_feature_selection() consumes (it's run
    on the dataframe *after* normalize_data() in select_features.process()) --
    applied here only to the `_var` columns. compute_job_metrics() already
    expresses utilization as a 0-1-ish fraction (divided by 1e6 at the
    source), so no extra rescale is needed for the target column itself.
    """
    df = df.copy()
    var_cols = [c for c in df.columns if c.endswith('_var')]

    for column in var_cols:
        column_min = df[column].min()
        column_max = df[column].max()
        if column_min == column_max:
            print(f'Warning: Column {column} has constant value. Setting to 0.5.')
            df[column] = 0.5
        else:
            df[column] = (df[column] - column_min) / (column_max - column_min)

    utilization = df[target_column]
    for column in var_cols:
        df[column] = df[column] * utilization

    return df


# ── Feature selection (mirrors select_features.run_feature_selection) ──────────


def select_variance_features(df: pd.DataFrame, target_column: str = 'utilization') -> pd.DataFrame:
    """
    Apply Pearson correlation + LassoCV embedded ranking -- the same two
    techniques select_features.run_feature_selection() uses -- restricted
    to the `_var` (burstiness) columns produced by compute_job_metrics().

    Returns a DataFrame indexed by counter column name with columns:
      pearson  - |Pearson correlation| with utilization
      lasso    - normalized |LassoCV coefficient| (0-1, within variance features)
    sorted by `pearson`, descending.
    """
    var_cols = [c for c in df.columns if c.endswith('_var')]
    if not var_cols:
        raise ValueError("No '_var' columns found -- run compute_job_metrics() first.")
    if target_column not in df.columns:
        raise ValueError(f"Target column '{target_column}' not found in DataFrame.")
    if df.empty:
        raise ValueError('DataFrame is empty -- check that the task filter matched any rows.')

    numeric_df = df[var_cols + [target_column]].apply(pd.to_numeric, errors='coerce').dropna()

    # --- Pearson correlation ---------------------------------------------------
    pearson = numeric_df[var_cols].corrwith(other=numeric_df[target_column], method='pearson')
    pearson = pearson.abs()

    # --- LassoCV embedded ranking, restricted to variance features -------------
    # Fit directly on the incoming features. Scale comparability across
    # counters is handled upstream by normalize_variance_features() (min-max
    # scale + utilization weighting, same as select_features.normalize_data()),
    # so by this point the `_var` columns are already on comparable scales.
    features = numeric_df[var_cols].values
    target = numeric_df[target_column].values

    lasso = LassoCV(cv=5, max_iter=1_000_000, tol=1e-3)
    lasso.fit(features, target)

    lasso_coef = pd.Series(np.abs(lasso.coef_), index=var_cols)
    l_min, l_max = lasso_coef.min(), lasso_coef.max()
    if l_max > l_min:
        lasso_coef = (lasso_coef - l_min) / (l_max - l_min)
    else:
        lasso_coef[:] = 0.0

    result = pd.DataFrame({'pearson': pearson, 'lasso': lasso_coef})
    result = result.sort_values('pearson', ascending=False)

    print('\n--- Variance-metric correlation with utilization ---')
    print(result)
    print(f'LassoCV best alpha : {lasso.alpha_:.6g}')
    print(f'LassoCV R^2 score  : {lasso.score(features, target):.4f}')

    return result


# ── Plotting ─────────────────────────────────────────────────────────────────


def plot_variance_correlation(result: pd.DataFrame, plot_directory: str, top_n: int = 15) -> None:
    """
    Horizontal bar chart of variance-metric correlation with utilization,
    in the same PCC/Lasso combined-bar style as run_feature_selection()'s
    'utilization_correlation.pdf'.
    """
    top = result.head(top_n)
    readable_labels = [get_readable_label(name[: -len('_var')]) for name in top.index]

    fig, ax = plt.subplots(figsize=(7, max(4, 0.4 * len(top))))
    y = np.arange(len(top))
    height = 0.35

    ax.barh(y + height / 2, top['pearson'], height, label='Pearson |r|', color='black')
    ax.barh(y - height / 2, top['lasso'], height, label='LassoCV (normalized)', color='silver')

    ax.set_yticks(y)
    ax.set_yticklabels(readable_labels)
    ax.invert_yaxis()
    ax.set_xlim(0, 1.0)
    ax.set_xticks([0.0, 0.25, 0.50, 0.75, 1.0])
    ax.set_xlabel('Correlation with Utilization')
    ax.set_title('Variance (Burstiness) Metrics vs. Utilization')
    ax.xaxis.grid(True, linestyle='-', alpha=0.6)
    for spine in ('top', 'right'):
        ax.spines[spine].set_visible(False)
    ax.legend(loc='upper center', bbox_to_anchor=(0.5, 1.08), ncol=2, frameon=False)

    out_file = join(plot_directory, 'variance_utilization_correlation.pdf')
    plt.savefig(out_file, format='pdf', bbox_inches='tight')
    print(f'  --> Saved variance-correlation plot to: {out_file}')
    plt.show()


# ── Entry point ──────────────────────────────────────────────────────────────


def process(dataset_path: str, plot_directory: str) -> None:
    with open(dataset_path, 'r', encoding='utf-8') as f:
        dataset = Dataset(**load(f))

    print('Computing per-job rate/variance/shape metrics...')
    df = compute_job_metrics(dataset)
    print(f'  Total job executions found: {len(df)}')

    if FILTER_TASK is not None:
        print(f'  Filtering for task        : {FILTER_TASK.value}')
        df = df[df['task'].str.contains(FILTER_TASK.value, case=False, na=False)]
        print(f'  Executions after filter   : {len(df)}')

    print('\nNormalizing variance features...')
    df = normalize_variance_features(df)

    print('\nRunning feature selection on variance metrics...')
    result = select_variance_features(df)

    print('\nPlotting variance vs. utilization correlation...')
    plot_variance_correlation(result, plot_directory)
    print('Done.')


if __name__ == '__main__':
    process(argv[1], argv[2])
