from argparse import ArgumentParser
from datetime import datetime
from os import sep
from os.path import join

import pandas as pd


def shift_target_to_next_job(df: pd.DataFrame, target_column: str) -> pd.DataFrame:
    df = df.sort_values(['core', 'thread', 'run_id']).reset_index(drop=True)

    next_target_column = f'next_{target_column}'
    df[next_target_column] = df.groupby(['core', 'thread'])[target_column].shift(-1)

    n_before = len(df)
    df = df[df[next_target_column].notna()].reset_index(drop=True)
    n_dropped = n_before - len(df)

    print(
        f'Dropped {n_dropped} row(s) with no following job for their thread '
        f'(the last observed iteration of each thread has no "next" target).'
    )

    return df


def main(input_path: str, output_directory: str, target_column: str) -> None:
    df = pd.read_csv(input_path)

    df = shift_target_to_next_job(df, target_column)

    input_file_base_name_parts = input_path.split(sep)[-1].split('-')

    timestamp = datetime.now().strftime('%Y-%m-%d-%H-%M')
    tag = input_file_base_name_parts[5] if len(input_file_base_name_parts) > 5 else 'data'

    collection_info = '-'.join(input_file_base_name_parts[6:-1])

    ending, _ = input_file_base_name_parts[-1].split('.')

    file_name = f'{timestamp}-{tag}-{collection_info}-{ending}-forecast.csv'

    df.to_csv(join(output_directory, file_name), index=False)


if __name__ == '__main__':
    parser = ArgumentParser(
        description='Shift the target to the next job iteration of the same thread, for inter-job forecasting'
    )
    parser.add_argument('input_path', help='Path to a per-job CSV (virtual metrics added, idle dropped)')
    parser.add_argument('output_directory', help='Directory to save the shifted-target CSV')
    parser.add_argument(
        '--target-column', default='utilization', help='Name of the column to shift forward (default: utilization)'
    )

    args = parser.parse_args()

    main(args.input_path, args.output_directory, args.target_column)
