from argparse import ArgumentParser, Namespace
from os import makedirs
from os.path import join
from typing import Callable, Iterator

import numpy as np
import pandas as pd
from sklearn.impute import SimpleImputer
from sklearn.metrics import mean_absolute_error, mean_squared_error, r2_score
from sklearn.preprocessing import StandardScaler

FIXED_COUNTERS = ['INSTRUCTIONS_RETIRED', 'CPU_CYCLES']
MAX_PROGRAMMABLE_COUNTERS = 2
MAX_L2_COUNTERS = 6

GLOBAL_PREFIX = 'GLOBAL_'
TARGET_COLUMN = 'next_utilization'
OWN_UTILIZATION_COLUMN = 'utilization'

BASELINE_NAME = 'baseline_last_value'
PROTOCOLS = ['temporal', 'leave_thread_out', 'leave_taskset_out']

ModelFactory = Callable[[], object]


def add_common_arguments(parser: ArgumentParser) -> None:
    parser.add_argument('output_directory', help='Directory to save the results')
    parser.add_argument(
        'dataset_paths',
        nargs='+',
        help='One or more forecasting CSVs (one per taskset), as produced by generate_forecasting_dataset.py',
    )
    parser.add_argument(
        '--counters',
        nargs='*',
        default=[],
        help=f'Up to {MAX_PROGRAMMABLE_COUNTERS} programmable non-L2 counters (per-core column names, e.g. L1_CACHE_MISS)',
    )
    parser.add_argument(
        '--l2-counters',
        nargs='*',
        default=[],
        help=f'Up to {MAX_L2_COUNTERS} L2 counters (GLOBAL_ prefix optional, e.g. L2_CACHE_MISS)',
    )
    parser.add_argument(
        '--pairwise-ratios',
        action='store_true',
        help='Also add (non-L2 counter) / (L2 counter) ratios, in the spirit of L1_BY_L2',
    )
    parser.add_argument(
        '--include-duration-features',
        action='store_true',
        help="Also give the model the previous job's CPU_CYCLES and utilization. Off by default: "
        'the goal is to capture the task profile from counters, and both are near-restatements '
        'of the previous duration (cycles ~ frequency x duration).',
    )
    parser.add_argument(
        '--protocols',
        nargs='+',
        choices=PROTOCOLS,
        default=None,
        help='Evaluation protocols (default: temporal and leave_thread_out, plus leave_taskset_out '
        'when more than one dataset is given)',
    )
    parser.add_argument('--n-folds', type=int, default=5, help='Number of time blocks for the temporal protocol')


def load_datasets(dataset_paths: list[str]) -> pd.DataFrame:
    frames = []

    for index, path in enumerate(dataset_paths):
        df = pd.read_csv(path)

        required = ['core', 'thread', 'run_id', TARGET_COLUMN, OWN_UTILIZATION_COLUMN, *FIXED_COUNTERS]
        missing = [column for column in required if column not in df.columns]
        if missing:
            raise ValueError(
                f'{path} is missing required columns {missing}. Models expect the output of '
                f'generate_forecasting_dataset.py (which adds {TARGET_COLUMN}).'
            )

        df['taskset'] = index
        frames.append(df)
        print(f'Taskset {index}: {path} ({len(df)} rows)')

    df = pd.concat(frames, ignore_index=True)

    df['group'] = df['taskset'].astype(str) + ':' + df['core'].astype(str) + ':' + df['thread'].astype(str)

    return df


def prepare_dataset(df: pd.DataFrame) -> pd.DataFrame:
    n_before = len(df)

    valid = (
        (df['CPU_CYCLES'] > 0)
        & (df['INSTRUCTIONS_RETIRED'] > 0)
        & df[TARGET_COLUMN].notna()
        & df[OWN_UTILIZATION_COLUMN].notna()
    )
    df = df[valid].reset_index(drop=True)  # type: ignore

    print(f'Dropped {n_before - len(df)} row(s) with missing target or zero/missing fixed counters.')

    return df


def resolve_counters(df: pd.DataFrame, counters: list[str], l2_counters: list[str]) -> tuple[list[str], list[str]]:
    if len(counters) > MAX_PROGRAMMABLE_COUNTERS:
        raise ValueError(f'At most {MAX_PROGRAMMABLE_COUNTERS} programmable counters allowed, got {len(counters)}.')
    if len(l2_counters) > MAX_L2_COUNTERS:
        raise ValueError(f'At most {MAX_L2_COUNTERS} L2 counters allowed, got {len(l2_counters)}.')

    for counter in counters:
        if counter in FIXED_COUNTERS:
            raise ValueError(f'{counter} is a fixed counter and is always available; do not list it in --counters.')
        if counter.startswith(GLOBAL_PREFIX):
            raise ValueError(f'{counter} is an L2 (GLOBAL) counter; pass it via --l2-counters.')

    l2_counters = [c if c.startswith(GLOBAL_PREFIX) else GLOBAL_PREFIX + c for c in l2_counters]

    missing = [c for c in [*counters, *l2_counters] if c not in df.columns]
    if missing:
        raise ValueError(f'Counters not found in the dataset: {missing}')

    return counters, l2_counters


def safe_ratio(numerator: pd.Series, denominator: pd.Series) -> pd.Series:
    with np.errstate(divide='ignore', invalid='ignore'):
        result = numerator / denominator

    return result.replace([np.inf, -np.inf], np.nan)


def build_features(
    df: pd.DataFrame,
    counters: list[str],
    l2_counters: list[str],
    pairwise_ratios: bool,
    include_duration: bool,
) -> pd.DataFrame:
    instructions = df['INSTRUCTIONS_RETIRED']
    cycles = df['CPU_CYCLES']

    features = pd.DataFrame(index=df.index)
    features['IPC'] = safe_ratio(instructions, cycles)  # type: ignore

    for counter in [*counters, *l2_counters]:
        features[f'{counter}_PER_CYCLE'] = safe_ratio(df[counter], cycles)  # type: ignore
        features[f'{counter}_PER_INSTR'] = safe_ratio(df[counter], instructions)  # type: ignore

    if pairwise_ratios:
        for counter in counters:
            for l2_counter in l2_counters:
                features[f'{counter}_BY_{l2_counter}'] = safe_ratio(df[counter], df[l2_counter])  # type: ignore

    if include_duration:
        features['CPU_CYCLES'] = cycles
        features[OWN_UTILIZATION_COLUMN] = df[OWN_UTILIZATION_COLUMN]

    return features


def preprocessing_steps() -> list[tuple[str, object]]:
    return [
        ('imputer', SimpleImputer(strategy='median', keep_empty_features=True)),
        ('scaler', StandardScaler()),
    ]


def generate_splits(df: pd.DataFrame, protocol: str, n_folds: int) -> Iterator[tuple[str, np.ndarray, np.ndarray]]:
    n_rows = len(df)
    all_indices = np.arange(n_rows)
    group_indices = df.groupby('group').indices

    if protocol == 'temporal':
        block = np.zeros(n_rows, dtype=int)
        run_ids = df['run_id'].values

        for indices in group_indices.values():
            ordered = indices[np.argsort(run_ids[indices])]  # type: ignore
            block[ordered] = (np.arange(len(ordered)) * n_folds) // len(ordered)

        for fold in range(1, n_folds):
            yield f'block_{fold}', np.where(block < fold)[0], np.where(block == fold)[0]

    elif protocol == 'leave_thread_out':
        for group, indices in group_indices.items():
            yield f'thread_{group}', np.setdiff1d(all_indices, indices), indices

    elif protocol == 'leave_taskset_out':
        for taskset in sorted(df['taskset'].unique()):
            test = np.where(df['taskset'].values == taskset)[0]
            yield f'taskset_{taskset}', np.setdiff1d(all_indices, test), test

    else:
        raise ValueError(f'Unknown protocol: {protocol}')


def compute_metrics(y_true: np.ndarray, y_pred: np.ndarray) -> dict[str, float]:
    mae = mean_absolute_error(y_true, y_pred)

    return {
        'mae': float(mae),
        'rmse': float(np.sqrt(mean_squared_error(y_true, y_pred))),
        'r2': float(r2_score(y_true, y_pred)) if len(y_true) > 1 else float('nan'),
        'rel_mae': float(mae / np.mean(np.abs(y_true))),
    }


def evaluate_protocol(
    df: pd.DataFrame,
    X: pd.DataFrame,
    y: pd.Series,
    protocol: str,
    models: dict[str, ModelFactory],
    n_folds: int,
) -> list[dict]:
    rows = []
    truths = []
    predictions: dict[str, list[np.ndarray]] = {name: [] for name in [BASELINE_NAME, *models]}

    for fold_name, train_idx, test_idx in generate_splits(df, protocol, n_folds):
        y_train, y_test = y.iloc[train_idx].values, y.iloc[test_idx].values
        truths.append(y_test)

        fold_predictions = {BASELINE_NAME: df[OWN_UTILIZATION_COLUMN].iloc[test_idx].values}

        for name, factory in models.items():
            model = factory()
            model.fit(X.iloc[train_idx], y_train)  # type: ignore
            fold_predictions[name] = model.predict(X.iloc[test_idx])  # type: ignore

        for name, prediction in fold_predictions.items():
            predictions[name].append(prediction)
            rows.append(
                {
                    'protocol': protocol,
                    'model': name,
                    'fold': fold_name,
                    'n_test': len(y_test),
                    **compute_metrics(y_test, prediction),
                }
            )

    y_all = np.concatenate(truths)
    for name, fold_predictions_list in predictions.items():
        rows.append(
            {
                'protocol': protocol,
                'model': name,
                'fold': 'ALL',
                'n_test': len(y_all),
                **compute_metrics(y_all, np.concatenate(fold_predictions_list)),
            }
        )

    return rows


def write_results(
    output_directory: str,
    script_name: str,
    config_lines: list[str],
    results: pd.DataFrame,
    descriptions: dict[str, str],
) -> None:
    makedirs(output_directory, exist_ok=True)

    results.to_csv(join(output_directory, f'{script_name}_folds.csv'), index=False)

    pooled = results[results['fold'] == 'ALL']

    path = join(output_directory, f'{script_name}_results.txt')
    with open(path, 'w', encoding='utf-8') as file:
        file.write(f'Results -- {script_name}\n')
        file.write('=' * 78 + '\n')
        file.write('\n'.join(config_lines) + '\n\n')
        file.write(
            'Metrics are pooled over all test rows of a protocol (per-fold numbers are in the CSV).\n'
            f'{BASELINE_NAME} predicts next utilization = current utilization; a model has to beat it\n'
            'on mae/rmse to justify the counters. Pooled r2 is inflated by between-task differences,\n'
            'so prefer mae/rel_mae when comparing against the baseline.\n\n'
        )

        for protocol in pooled['protocol'].unique():  # type: ignore
            table = pooled[pooled['protocol'] == protocol][['model', 'n_test', 'mae', 'rmse', 'r2', 'rel_mae']]
            file.write(f'--- {protocol} ---\n')
            file.write(table.to_string(index=False, float_format=lambda value: f'{value:.4f}') + '\n\n')  # type: ignore

        for name, description in descriptions.items():
            file.write(f'--- {name} (fitted on all data) ---\n')
            file.write(description + '\n\n')

    print(f'Wrote {path}')


def run_experiment(
    args: Namespace,
    script_name: str,
    models: dict[str, ModelFactory],
    describe_model: Callable[[str, object, list[str]], str] | None = None,
) -> None:
    df = load_datasets(args.dataset_paths)
    counters, l2_counters = resolve_counters(df, args.counters, args.l2_counters)
    df = prepare_dataset(df)

    X = build_features(df, counters, l2_counters, args.pairwise_ratios, args.include_duration_features)
    y = df[TARGET_COLUMN]

    n_tasksets = df['taskset'].nunique()
    protocols = args.protocols or ['temporal', 'leave_thread_out'] + (['leave_taskset_out'] if n_tasksets > 1 else [])
    if 'leave_taskset_out' in protocols and n_tasksets < 2:
        raise ValueError('leave_taskset_out needs at least two datasets.')

    config_lines = [
        f'Fixed counters:        {FIXED_COUNTERS}',
        f'Programmable counters: {counters}',
        f'L2 counters:           {l2_counters}',
        f'Features ({X.shape[1]}):         {list(X.columns)}',
        f'Rows: {len(df)} across {n_tasksets} taskset(s), {df["group"].nunique()} thread(s)',
        f'Protocols: {protocols} (temporal folds: {args.n_folds})',
    ]
    print('\n'.join(config_lines))

    rows = []
    for protocol in protocols:
        print(f'\n=== {protocol} ===')
        rows.extend(evaluate_protocol(df, X, y, protocol, models, args.n_folds))  # type: ignore

    results = pd.DataFrame(rows)

    descriptions = {}
    if describe_model is not None:
        for name, factory in models.items():
            fitted = factory()
            fitted.fit(X, y.values)  # type: ignore
            descriptions[name] = describe_model(name, fitted, list(X.columns))

    write_results(args.output_directory, script_name, config_lines, results, descriptions)
