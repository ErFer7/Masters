import re
from sys import argv, exit
from json import load
from pathlib import Path

from definitions import (
    DataOrigin,
    Dataset,
    PMUEvent,
    RunData,
)


def merge(benchmarks: list[RunData]) -> Dataset:
    timestamps = {
        DataOrigin.GLOBAL: [],
        DataOrigin.CORE_1: [],
        DataOrigin.CORE_2: [],
        DataOrigin.CORE_3: [],
    }

    temp_dataset = {
        'data': {
            DataOrigin.GLOBAL: {},
            DataOrigin.CORE_1: {},
            DataOrigin.CORE_2: {},
            DataOrigin.CORE_3: {},
        }
    }

    dataset = {
        'data': {
            DataOrigin.GLOBAL: [],
            DataOrigin.CORE_1: [],
            DataOrigin.CORE_2: [],
            DataOrigin.CORE_3: [],
        }
    }

    for benchmark in benchmarks:
        for core, data in benchmark.data.items():
            if core == 0:
                for monitor_data in data:
                    event = monitor_data.event

                    if (
                        any(map(bool, list(zip(*monitor_data.data))[1]))
                        and not isinstance(event, PMUEvent)
                        and event not in temp_dataset['data'][DataOrigin.GLOBAL]
                    ):
                        temp_dataset['data'][DataOrigin.GLOBAL][event] = monitor_data
                        timestamps[DataOrigin.GLOBAL].append(list(zip(*monitor_data.data))[0])
            else:
                data_origin = list(DataOrigin)[core]

                for monitor_data in data:
                    event = monitor_data.event

                    if (
                        any(map(bool, list(zip(*monitor_data.data))[1]))
                        and event not in temp_dataset['data'][data_origin]
                    ):
                        temp_dataset['data'][data_origin][event] = monitor_data
                        timestamps[data_origin].append(list(zip(*monitor_data.data))[0])

    merged_timestamps = {
        DataOrigin.GLOBAL: [],
        DataOrigin.CORE_1: [],
        DataOrigin.CORE_2: [],
        DataOrigin.CORE_3: [],
    }

    for data_origin, timestamp_lists in timestamps.items():
        timestamp_average_calc = []
        max_len = 0

        for timestamp_list in timestamp_lists:
            timestamp_list_len = len(timestamp_list)

            if timestamp_list_len > max_len:
                max_len = timestamp_list_len

        for i in range(max_len):
            timestamp_average_calc.append([0, 0])

            for timestamp_list in timestamp_lists:
                if i >= len(timestamp_list):
                    continue

                timestamp_average_calc[i][0] += timestamp_list[i]
                timestamp_average_calc[i][1] += 1

        for sum, count in timestamp_average_calc:
            merged_timestamps[data_origin].append(sum // count)

    for data_origin, data in temp_dataset['data'].items():
        for _, monitor_data in data.items():
            for i in range(len(monitor_data.data)):
                monitor_data.data[i] = (
                    merged_timestamps[data_origin][i],
                    monitor_data.data[i][1],
                )

        dataset['data'][data_origin] = list(data.values())

    return Dataset(**dataset)


def get_sort_key(filepath: Path) -> list:
    numbers = re.findall(r'\d+', filepath.name)
    return [int(n) for n in numbers] if numbers else [filepath.name]


def main(data_dir: str, saving_name: str) -> None:
    benchmarks = []
    directory = Path(data_dir)

    file_paths = sorted(directory.glob('*.json'), key=get_sort_key)

    for benchmark_file_path in file_paths:
        with open(benchmark_file_path, 'r', encoding='utf-8') as file:
            benchmarks.append(RunData(**load(file)))

    if not benchmarks:
        print(f'No JSON files found in {data_dir}')
        return

    dataset = merge(benchmarks)
    json_str = dataset.model_dump_json(indent=4)

    with open(saving_name, 'w+', encoding='utf-8') as file:
        file.write(json_str)


if __name__ == '__main__':
    if len(argv) != 3:
        print('Usage: python script.py <data_directory> <output_file>')
        exit(1)

    main(argv[1], argv[2])
