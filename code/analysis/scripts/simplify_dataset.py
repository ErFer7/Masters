from argparse import ArgumentParser
from datetime import datetime
from json import load
from os import sep
from os.path import join

from definitions.data_models import DataStream, MonitorData, PeriodicDataset, SimplePeriodicDataset
from definitions.data_origins import DataOrigin
from definitions.events import SystemEvent


def get_cpu_clock(cpu_clock_data: MonitorData) -> int:
    cpu_clock = cpu_clock_data.data[0][1]

    for _, value in cpu_clock_data.data:
        if value != cpu_clock:
            print(f'WARNING: There are different CPU clocks! -> {value} != {cpu_clock}')

    return cpu_clock  # type: ignore


def get_values(dataset: PeriodicDataset, sanitized_dataset: SimplePeriodicDataset) -> None:
    for data_origin, data in dataset.data.items():
        sanitized_dataset.data[data_origin] = []

        for monitor_data in data:
            if data_origin == DataOrigin.GLOBAL and (
                monitor_data.event == SystemEvent.CPU_CLOCK or monitor_data.event == SystemEvent.CPU_VOLTAGE  # type: ignore
            ):
                continue

            _, values = zip(*monitor_data.data)  # type: ignore

            sanitized_dataset.data[data_origin].append(DataStream(event=monitor_data.event, data=values))  # type: ignore


def sanitize(dataset: PeriodicDataset) -> SimplePeriodicDataset:
    sanitized_dataset = SimplePeriodicDataset(
        test_duration=dataset.test_duration,
        rng_seed=dataset.rng_seed,
        collection_duration=dataset.collection_duration,
        taskset=dataset.taskset,
    )

    cpu_clock_data = None

    for monitor_data in dataset.data[DataOrigin.GLOBAL]:
        if monitor_data.event == SystemEvent.CPU_CLOCK:  # type: ignore
            cpu_clock_data = monitor_data

    sanitized_dataset.cpu_clock = get_cpu_clock(cpu_clock_data)  # type: ignore

    get_values(dataset, sanitized_dataset)

    return sanitized_dataset


def main(input_path: str, output_directory: str) -> None:
    with open(input_path, 'r', encoding='utf-8') as file:
        dataset = PeriodicDataset(**load(file))

    dataset = sanitize(dataset)

    json_str = dataset.model_dump_json(indent=4)

    input_file_base_name_parts = input_path.split(sep)[-1].split('-')

    timestamp = datetime.now().strftime('%Y-%m-%d-%H-%M')
    tag = input_file_base_name_parts[5]

    collection_info = '-'.join(input_file_base_name_parts[6:-1])

    ending, _ = input_file_base_name_parts[-1].split('.')

    file_name = f'{timestamp}-{tag}-{collection_info}-{ending}-simplified.json'

    with open(join(output_directory, file_name), 'w+', encoding='utf-8') as file:
        file.write(json_str)


if __name__ == '__main__':
    parser = ArgumentParser(description='Generate a simplified dataset')
    parser.add_argument('input_path', help='Path to a periodic dataset')
    parser.add_argument('output_directory', help='Directory to save the simplified dataset')

    args = parser.parse_args()

    main(args.input_path, args.output_directory)
