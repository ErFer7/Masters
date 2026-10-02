from argparse import ArgumentParser
from datetime import datetime
from os import sep
from os.path import join

import pandas as pd


def drop_idle(df: pd.DataFrame) -> pd.DataFrame:
    n_before = len(df)
    df = df[~df['is_idle']].reset_index(drop=True)
    n_dropped = n_before - len(df)

    print(f'Dropped {n_dropped} idle rows out of {n_before} ({n_dropped / n_before:.1%}).')

    return df


def main(input_path: str, output_directory: str) -> None:
    df = pd.read_csv(input_path)

    df = drop_idle(df)

    input_file_base_name_parts = input_path.split(sep)[-1].split('-')

    timestamp = datetime.now().strftime('%Y-%m-%d-%H-%M')
    tag = input_file_base_name_parts[5] if len(input_file_base_name_parts) > 5 else 'data'

    collection_info = '-'.join(input_file_base_name_parts[6:-1])

    ending, _ = input_file_base_name_parts[-1].split('.')

    file_name = f'{timestamp}-{tag}-{collection_info}-{ending}-no-idle.csv'

    df.to_csv(join(output_directory, file_name), index=False)


if __name__ == '__main__':
    parser = ArgumentParser(description='Drop idle rows from a per-job dataset')
    parser.add_argument('input_path', help='Path to a per-job CSV (with virtual metrics already added)')
    parser.add_argument('output_directory', help='Directory to save the filtered CSV')

    args = parser.parse_args()

    main(args.input_path, args.output_directory)
