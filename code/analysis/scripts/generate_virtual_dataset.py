from argparse import ArgumentParser
from datetime import datetime
from os import sep
from os.path import join
from typing import Callable

import numpy as np
import pandas as pd


def safe_ratio(numerator: pd.Series, denominator: pd.Series) -> pd.Series:
    """Elementwise division that turns divide-by-zero and 0/0 into NaN instead of inf/error.
    NaN (rather than 0) is the right default here: a 0/0 row usually means "no activity in
    this job iteration for this event", which is a missing signal, not a zero rate."""

    with np.errstate(divide='ignore', invalid='ignore'):
        result = numerator / denominator

    return result.replace([np.inf, -np.inf], np.nan)


# Each entry: name of the output column, the input columns it needs (checked before
# computing, so a missing/renamed event just skips this one metric with a warning instead
# of crashing the whole script), and the function that computes it from the dataframe.
#
# Ratios are computed on the per-job SUMMED raw counts (already summed across all samples
# of the job by generate_job_dataset.py) -- i.e. ratio of sums, not an average of per-sample
# ratios. This avoids biasing toward samples with small denominators, which a naive
# per-sample-then-average approach would do.
#
# NOTE: several event names below (the *_INTERLOCK and *_BUSY ones) are inferred from the
# sanitized data sample and may not match your actual events.py exactly -- verify the
# column names in your CSV and adjust REQUIRES/the lambda if a metric gets skipped.
METRIC_DEFINITIONS: list[dict] = [
    # --- Core efficiency / utilization proxies ---
    {
        'name': 'IPC',
        'requires': ['INSTRUCTIONS_RETIRED', 'CPU_CYCLES'],
        'compute': lambda df: safe_ratio(df['INSTRUCTIONS_RETIRED'], df['CPU_CYCLES']),
    },
    {
        'name': 'CPI',
        'requires': ['CPU_CYCLES', 'INSTRUCTIONS_RETIRED'],
        'compute': lambda df: safe_ratio(df['CPU_CYCLES'], df['INSTRUCTIONS_RETIRED']),
    },
    # --- Cache / memory-boundness proxies ---
    {
        'name': 'L1_CMPI',
        'requires': ['L1_CACHE_MISS', 'INSTRUCTIONS_RETIRED'],
        'compute': lambda df: safe_ratio(df['L1_CACHE_MISS'], df['INSTRUCTIONS_RETIRED']) * 1000,
    },
    {
        'name': 'ICACHE_MPKI',
        'requires': ['INSTRUCTION_CACHE_MISS', 'INSTRUCTIONS_RETIRED'],
        'compute': lambda df: safe_ratio(df['INSTRUCTION_CACHE_MISS'], df['INSTRUCTIONS_RETIRED']) * 1000,
    },
    {
        'name': 'DCACHE_MPKI',
        'requires': ['DATA_CACHE_MISS_OR_MEMORY_MAPPED_IO_ACCESS', 'INSTRUCTIONS_RETIRED'],
        'compute': lambda df: safe_ratio(df['DATA_CACHE_MISS_OR_MEMORY_MAPPED_IO_ACCESS'], df['INSTRUCTIONS_RETIRED'])
        * 1000,
    },
    {
        # Shared/global counter divided by a per-core count -- see the contention
        # discussion: interpret this as a system-pressure signal, not a pure per-task one.
        'name': 'L2_MPKI',
        'requires': ['GLOBAL_L2_CACHE_MISS', 'INSTRUCTIONS_RETIRED'],
        'compute': lambda df: safe_ratio(df['GLOBAL_L2_CACHE_MISS'], df['INSTRUCTIONS_RETIRED']) * 1000,
    },
    {
        'name': 'L1_BY_L2',
        'requires': ['L1_CACHE_MISS', 'GLOBAL_L2_CACHE_MISS'],
        'compute': lambda df: safe_ratio(df['L1_CACHE_MISS'], df['GLOBAL_L2_CACHE_MISS']),
    },
    # --- Stall / interlock cycle ratios (strong memory-boundness candidates if present) ---
    {
        'name': 'MEM_STALL_RATIO',
        'requires': ['LONG_LATENCY_INTERLOCK', 'CPU_CYCLES'],
        'compute': lambda df: safe_ratio(df['LONG_LATENCY_INTERLOCK'], df['CPU_CYCLES']),
    },
    {
        'name': 'ADDR_GEN_STALL_RATIO',
        'requires': ['ADDRESS_GENERATION_INTERLOCK', 'CPU_CYCLES'],
        'compute': lambda df: safe_ratio(df['ADDRESS_GENERATION_INTERLOCK'], df['CPU_CYCLES']),
    },
    {
        'name': 'FP_STALL_RATIO',
        'requires': ['FLOATING_POINT_INTERLOCK', 'CPU_CYCLES'],
        'compute': lambda df: safe_ratio(df['FLOATING_POINT_INTERLOCK'], df['CPU_CYCLES']),
    },
    {
        'name': 'ICACHE_BUSY_RATIO',
        'requires': ['INSTRUCTION_CACHE_ITIM_BUSY', 'CPU_CYCLES'],
        'compute': lambda df: safe_ratio(df['INSTRUCTION_CACHE_ITIM_BUSY'], df['CPU_CYCLES']),
    },
    {
        'name': 'DCACHE_BUSY_RATIO',
        'requires': ['DATA_CACHE_DTIM_BUSY', 'CPU_CYCLES'],
        'compute': lambda df: safe_ratio(df['DATA_CACHE_DTIM_BUSY'], df['CPU_CYCLES']),
    },
    # --- Branch behavior (control-flow proxy, not memory-boundness, but a useful control) ---
    {
        'name': 'BRANCH_MISS_RATE',
        'requires': [
            'BRANCH_DIRECTION_MISPREDICTION',
            'BRANCH_JUMP_TARGET_MISPREDICTION',
            'CONDITIONAL_BRANCHES_RETIRED',
            'JAL_INSTRUCTIONS_RETIRED',
            'JALR_INSTRUCTIONS_RETIRED',
        ],
        'compute': lambda df: safe_ratio(
            df['BRANCH_DIRECTION_MISPREDICTION'] + df['BRANCH_JUMP_TARGET_MISPREDICTION'],
            df['CONDITIONAL_BRANCHES_RETIRED'] + df['JAL_INSTRUCTIONS_RETIRED'] + df['JALR_INSTRUCTIONS_RETIRED'],
        ),
    },
    # --- Instruction mix (characterizes workload type, useful alongside the target) ---
    {
        'name': 'MEM_INSTR_FRACTION',
        'requires': [
            'INTEGER_LOAD_INSTRUCTIONS_RETIRED',
            'INTEGER_STORE_INSTRUCTIONS_RETIRED',
            'ATOMIC_MEMORY_INSTRUCTIONS_RETIRED',
            'FLOATING_POINT_LOAD_INSTRUCTION_RETIRED',
            'FLOATING_POINT_STORE_INSTRUCTION_RETIRED',
            'INSTRUCTIONS_RETIRED',
        ],
        'compute': lambda df: safe_ratio(
            df['INTEGER_LOAD_INSTRUCTIONS_RETIRED']
            + df['INTEGER_STORE_INSTRUCTIONS_RETIRED']
            + df['ATOMIC_MEMORY_INSTRUCTIONS_RETIRED']
            + df['FLOATING_POINT_LOAD_INSTRUCTION_RETIRED']
            + df['FLOATING_POINT_STORE_INSTRUCTION_RETIRED'],
            df['INSTRUCTIONS_RETIRED'],
        ),
    },
    {
        'name': 'LOAD_STORE_RATIO',
        'requires': ['INTEGER_LOAD_INSTRUCTIONS_RETIRED', 'INTEGER_STORE_INSTRUCTIONS_RETIRED'],
        'compute': lambda df: safe_ratio(
            df['INTEGER_LOAD_INSTRUCTIONS_RETIRED'], df['INTEGER_STORE_INSTRUCTIONS_RETIRED']
        ),
    },
]


def generate_virtual_metrics(df: pd.DataFrame, metric_definitions: list[dict] = METRIC_DEFINITIONS) -> pd.DataFrame:
    df = df.copy()

    for metric in metric_definitions:
        missing = [col for col in metric['requires'] if col not in df.columns]

        if missing:
            print(f"Skipping '{metric['name']}': missing columns {missing}")
            continue

        compute: Callable[[pd.DataFrame], pd.Series] = metric['compute']
        df[metric['name']] = compute(df)

    return df


def main(input_path: str, output_directory: str) -> None:
    df = pd.read_csv(input_path)

    df = generate_virtual_metrics(df)

    input_file_base_name_parts = input_path.split(sep)[-1].split('-')

    timestamp = datetime.now().strftime('%Y-%m-%d-%H-%M')
    tag = input_file_base_name_parts[5] if len(input_file_base_name_parts) > 5 else 'data'

    collection_info = '-'.join(input_file_base_name_parts[6:-1])

    ending, _ = input_file_base_name_parts[-1].split('.')

    file_name = f'{timestamp}-{tag}-{collection_info}-{ending}-virtual.csv'

    df.to_csv(join(output_directory, file_name), index=False)


if __name__ == '__main__':
    parser = ArgumentParser(description='Add virtual/ratio metrics to a per-job dataset')
    parser.add_argument('input_path', help='Path to a per-job CSV')
    parser.add_argument('output_directory', help='Directory to save the CSV with virtual metrics added')

    args = parser.parse_args()

    main(args.input_path, args.output_directory)
