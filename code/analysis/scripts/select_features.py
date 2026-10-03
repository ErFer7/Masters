from argparse import ArgumentParser
from os.path import join

import numpy as np
import pandas as pd

RankingResult = list[tuple[str, float | int | str]]

DEFAULT_EXCLUDE_COLUMNS = [
    'core',
    'run_id',
    'thread',
    'is_idle',
    'benchmark',
    'start_index',
    'end_index',
    'n_samples',
    'n_valid_samples',
    'utilization_consistent',
    'utilization_values_seen',
    'utilization',
]


def prepare_features(
    df: pd.DataFrame, target_column: str, exclude_columns: list[str]
) -> tuple[pd.DataFrame, pd.Series]:
    numeric_df = df.select_dtypes(include=['number']).copy()

    drop_columns = set(exclude_columns) | {target_column}
    feature_columns = [col for col in numeric_df.columns if col not in drop_columns]

    X = numeric_df[feature_columns]
    y = numeric_df[target_column]

    valid_columns = []
    for col in feature_columns:
        if X[col].isna().all():
            print(f"Dropping '{col}': all values are NaN.")
            continue
        if X[col].nunique(dropna=True) <= 1:
            print(f"Dropping '{col}': constant value, no discriminative power.")
            continue
        valid_columns.append(col)

    X = X[valid_columns]

    missing_counts = X.isna().sum()
    for col in valid_columns:
        if missing_counts[col] > 0:
            median = X[col].median()
            print(f"Imputing {missing_counts[col]} missing value(s) in '{col}' with median ({median:.4g}).")
            X[col] = X[col].fillna(median)

    rows_with_target = y.notna()
    if not rows_with_target.all():
        print(f'Dropping {(~rows_with_target).sum()} row(s) with missing target.')
        X = X[rows_with_target]
        y = y[rows_with_target]

    return X, y


def save_ranking(output_dir: str, method_name: str, ranking: RankingResult, notes: str) -> None:
    path = join(output_dir, f'{method_name}.txt')

    with open(path, 'w', encoding='utf-8') as file:
        file.write(f'Feature ranking -- {method_name}\n')
        file.write(f'{notes}\n')
        file.write('=' * 70 + '\n')

        for rank, (name, score) in enumerate(ranking, start=1):
            if isinstance(score, float):
                file.write(f'{rank:3d}. {name:<45s} {score:.6f}\n')
            else:
                file.write(f'{rank:3d}. {name:<45s} {score}\n')

    print(f'Wrote {path}')


def fit_random_forest(X: pd.DataFrame, y: pd.Series):
    from sklearn.ensemble import RandomForestRegressor

    model = RandomForestRegressor(n_estimators=500, random_state=42, n_jobs=-1)
    model.fit(X.values, y.values)

    return model


# --- Linear baselines -------------------------------------------------------------------


def select_pearson_correlation(X: pd.DataFrame, y: pd.Series, k: int) -> RankingResult:
    correlations = X.corrwith(y, method='pearson').abs().dropna().sort_values(ascending=False)

    return [(name, float(score)) for name, score in correlations.items()]


def select_lasso_cv(X: pd.DataFrame, y: pd.Series, k: int) -> RankingResult:
    from sklearn.linear_model import LassoCV
    from sklearn.preprocessing import StandardScaler

    X_scaled = StandardScaler().fit_transform(X.values)
    y_scaled = StandardScaler().fit_transform(y.values.reshape(-1, 1)).ravel()

    lasso = LassoCV(cv=5, max_iter=200000, random_state=42)
    lasso.fit(X_scaled, y_scaled)

    ranked = sorted(zip(X.columns, np.abs(lasso.coef_)), key=lambda item: item[1], reverse=True)

    return [(name, float(score)) for name, score in ranked]


# --- Nonlinear filters -------------------------------------------------------------------


def select_mutual_information(X: pd.DataFrame, y: pd.Series, k: int) -> RankingResult:
    from sklearn.feature_selection import mutual_info_regression

    scores = mutual_info_regression(X.values, y.values, random_state=42)
    ranked = sorted(zip(X.columns, scores), key=lambda item: item[1], reverse=True)

    return [(name, float(score)) for name, score in ranked]


def select_distance_correlation(X: pd.DataFrame, y: pd.Series, k: int) -> RankingResult:
    import dcor

    y_values = y.values
    scores = [(col, float(dcor.distance_correlation(X[col].values, y_values))) for col in X.columns]
    scores.sort(key=lambda item: item[1], reverse=True)

    return scores


def select_mrmr(X: pd.DataFrame, y: pd.Series, k: int) -> RankingResult:
    from mrmr import mrmr_regression

    selected = mrmr_regression(X=X, y=y, K=min(k, X.shape[1]))

    return [(name, rank) for rank, name in enumerate(selected, start=1)]


def select_hsic_lasso(X: pd.DataFrame, y: pd.Series, k: int) -> RankingResult:
    from pyHSICLasso import HSICLasso

    hsic = HSICLasso()
    hsic.input(X.values, y.values.reshape(-1, 1), featname=list(X.columns))
    hsic.regression(num_feat=min(k, X.shape[1]))

    selected = hsic.get_features()
    scores = hsic.get_index_score()

    ranked = sorted(zip(selected, scores), key=lambda item: item[1], reverse=True)

    return [(name, float(score)) for name, score in ranked]


# --- Embedded / model-driven methods ------------------------------------------------------


def select_random_forest_importance(X: pd.DataFrame, y: pd.Series, k: int) -> RankingResult:
    model = fit_random_forest(X, y)
    ranked = sorted(zip(X.columns, model.feature_importances_), key=lambda item: item[1], reverse=True)

    return [(name, float(score)) for name, score in ranked]


def select_permutation_importance(X: pd.DataFrame, y: pd.Series, k: int) -> RankingResult:
    from sklearn.inspection import permutation_importance

    model = fit_random_forest(X, y)
    result = permutation_importance(model, X.values, y.values, n_repeats=20, random_state=42, n_jobs=-1)

    ranked = sorted(zip(X.columns, result.importances_mean), key=lambda item: item[1], reverse=True)

    return [(name, float(score)) for name, score in ranked]


def select_shap_importance(X: pd.DataFrame, y: pd.Series, k: int) -> RankingResult:
    import shap

    model = fit_random_forest(X, y)
    explainer = shap.TreeExplainer(model)
    shap_values = explainer.shap_values(X.values)

    mean_abs_shap = np.abs(shap_values).mean(axis=0)
    ranked = sorted(zip(X.columns, mean_abs_shap), key=lambda item: item[1], reverse=True)

    return [(name, float(score)) for name, score in ranked]


def select_boruta(X: pd.DataFrame, y: pd.Series, k: int) -> RankingResult:
    from boruta import BorutaPy
    from sklearn.ensemble import RandomForestRegressor

    model = RandomForestRegressor(n_estimators=200, random_state=42, n_jobs=-1)
    selector = BorutaPy(model, n_estimators='auto', random_state=42, verbose=0)
    selector.fit(X.values, y.values)

    statuses = []
    for name, confirmed, tentative, rank in zip(
        X.columns, selector.support_, selector.support_weak_, selector.ranking_
    ):
        label = 'Confirmed' if confirmed else ('Tentative' if tentative else 'Rejected')
        statuses.append((name, label, rank))

    statuses.sort(key=lambda item: item[2])

    return [(name, f'{label} (boruta rank {rank})') for name, label, rank in statuses]


# --- Neighbor-based method -----------------------------------------------------------------


def select_relieff(X: pd.DataFrame, y: pd.Series, k: int) -> RankingResult:
    from skrebate import ReliefF

    selector = ReliefF(n_features_to_select=min(k, X.shape[1]), n_neighbors=10, n_jobs=-1)
    selector.fit(X.values, y.values)

    ranked = sorted(zip(X.columns, selector.feature_importances_), key=lambda item: item[1], reverse=True)

    return [(name, float(score)) for name, score in ranked]


# --- Orchestration ---------------------------------------------------------------------

SELECTION_METHODS = [
    (
        'pearson_correlation',
        select_pearson_correlation,
        'Score = |Pearson correlation| with target. Linear only -- a low score here does '
        'not rule out a nonlinear relationship.',
    ),
    (
        'mutual_information',
        select_mutual_information,
        'Score = mutual information with target (sklearn mutual_info_regression). Captures '
        'nonlinear dependence, no redundancy awareness.',
    ),
    (
        'distance_correlation',
        select_distance_correlation,
        'Score = distance correlation with target (nonlinear; zero iff independent).',
    ),
    (
        'lasso_cv',
        select_lasso_cv,
        'Score = |LassoCV coefficient| on standardized features (linear, cross-validated regularization strength).',
    ),
    (
        'mrmr',
        select_mrmr,
        'Score = selection order under mRMR (1 = most relevant & least redundant given prior picks). Redundancy-aware.',
    ),
    (
        'hsic_lasso',
        select_hsic_lasso,
        'Score = HSIC-Lasso coefficient (nonlinear, redundancy-aware via kernel independence).',
    ),
    (
        'random_forest_importance',
        select_random_forest_importance,
        'Score = Random Forest impurity-based importance. Biased toward high-cardinality / '
        'continuous features -- cross-check against permutation_importance and '
        'shap_importance before trusting this alone.',
    ),
    (
        'permutation_importance',
        select_permutation_importance,
        'Score = mean drop in R^2 when a feature is shuffled, on a fitted Random Forest. '
        'Model-agnostic, nonlinear, not biased the way impurity importance is.',
    ),
    (
        'shap_importance',
        select_shap_importance,
        'Score = mean |SHAP value| from a fitted Random Forest. Nonlinear, captures feature interactions.',
    ),
    (
        'boruta',
        select_boruta,
        'Status = Confirmed / Tentative / Rejected, tiebroken by Boruta rank (nonlinear, '
        'RF-based all-relevant method -- keeps anything genuinely informative, not just '
        'the top-k).',
    ),
    (
        'relieff',
        select_relieff,
        'Score = ReliefF feature weight (nonlinear, nearest-neighbor based; used as a proxy '
        'for RReliefF on a continuous target).',
    ),
]


def select_features(
    df: pd.DataFrame,
    target_column: str,
    output_dir: str,
    top_k: int,
    exclude_columns: list[str] = DEFAULT_EXCLUDE_COLUMNS,
) -> None:
    X, y = prepare_features(df, target_column, exclude_columns)

    print(f'Running feature selection on {X.shape[0]} rows, {X.shape[1]} candidate features.')
    print(f'Candidate features: {list(X.columns)}')

    for method_name, method_func, notes in SELECTION_METHODS:
        print(f'\n--- {method_name} ---')

        try:
            ranking = method_func(X, y, top_k)
        except ImportError as error:
            print(f"Skipping '{method_name}': required package not installed ({error}).")
            continue
        except Exception as error:  # noqa: BLE001 -- one method's failure shouldn't stop the rest
            print(f"Skipping '{method_name}': {error}")
            continue

        save_ranking(output_dir, method_name, ranking, notes)


def main(
    dataset_path: str, output_directory: str, target_column: str, top_k: int, include_own_utilization: bool
) -> None:
    df = pd.read_csv(dataset_path)

    exclude_columns = list(DEFAULT_EXCLUDE_COLUMNS)
    if include_own_utilization:
        exclude_columns.remove('utilization')

    select_features(df, target_column, output_directory, top_k, exclude_columns)


if __name__ == '__main__':
    parser = ArgumentParser(description='Run several feature selection methods against a per-job dataset')
    parser.add_argument('dataset_path', help='Path to the per-job CSV (virtual metrics added, idle dropped)')
    parser.add_argument('output_directory', help='Directory to save one ranking file per method')
    parser.add_argument(
        '--target-column', default='utilization', help='Name of the target column (default: utilization)'
    )
    parser.add_argument(
        '--top-k',
        type=int,
        default=15,
        help='Feature budget for methods that select a fixed-size subset (mRMR, HSIC-Lasso, ReliefF)',
    )
    parser.add_argument(
        '--include-own-utilization',
        action='store_true',
        help="Include the current job's own 'utilization' as a candidate feature, for a baseline-comparison run.",
    )

    args = parser.parse_args()

    main(args.dataset_path, args.output_directory, args.target_column, args.top_k, args.include_own_utilization)
