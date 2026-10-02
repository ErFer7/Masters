from argparse import ArgumentParser
from datetime import datetime
from json import load
from os import sep
from os.path import join

from definitions.data_models import MonitorData, PeriodicDataset
from definitions.data_origins import DataOrigin
from definitions.events import (
    L2CachePMUEvent,
    L2CachePMUVirtualEvent,
    PMUEvent,
    PMUVirtualEvent,
)


def calculate_ipc(instructions_retired: MonitorData, cycles: MonitorData) -> MonitorData:
    ipc = MonitorData(event=PMUVirtualEvent.IPC)

    timestamps = map(lambda x: x[0], instructions_retired.data)
    instructions_retired_values = map(lambda x: x[1], instructions_retired.data)
    cycles_values = map(lambda x: x[1], cycles.data)

    ipc.data = list(zip(timestamps, map(lambda x: x[0] / x[1], zip(instructions_retired_values, cycles_values))))

    return ipc


def calculate_cpi(cycles: MonitorData, instructions_retired: MonitorData) -> MonitorData:
    cpi = MonitorData(event=PMUVirtualEvent.CPI)

    timestamps = map(lambda x: x[0], instructions_retired.data)
    cycles_values = map(lambda x: x[1], cycles.data)
    instructions_retired_values = map(lambda x: x[1], instructions_retired.data)

    cpi.data = list(zip(timestamps, map(lambda x: x[0] / x[1], zip(cycles_values, instructions_retired_values))))

    return cpi


def calculate_l1_cmpi(l1_cache_misses: MonitorData, instructions_retired: MonitorData) -> MonitorData:
    l1_cmpi = MonitorData(event=PMUVirtualEvent.L1_CMPI)

    timestamps = map(lambda x: x[0], instructions_retired.data)
    l1_cache_misses_values = map(lambda x: x[1], l1_cache_misses.data)
    instructions_retired_values = map(lambda x: x[1], instructions_retired.data)

    l1_cmpi.data = list(
        zip(timestamps, map(lambda x: x[0] / x[1], zip(l1_cache_misses_values, instructions_retired_values)))
    )

    return l1_cmpi


def calculate_l1_by_l2(l1_cache_misses: MonitorData, l2_cache_misses: MonitorData) -> MonitorData:
    l1_by_l2 = MonitorData(event=L2CachePMUVirtualEvent.L1_BY_L2)

    timestamps = map(lambda x: x[0], l1_cache_misses.data)
    l1_cache_misses_values = map(lambda x: x[1], l1_cache_misses.data)
    l2_cache_misses_values = map(lambda x: x[1], l2_cache_misses.data)

    l1_by_l2.data = list(
        zip(timestamps, map(lambda x: x[0] / x[1], zip(l1_cache_misses_values, l2_cache_misses_values)))
    )

    return l1_by_l2


def calculate_l2_cmpi(l2_cache_misses: MonitorData, instructions_retired: MonitorData) -> MonitorData:
    l2_cmpi = MonitorData(event=L2CachePMUVirtualEvent.L2_CMPI)

    timestamps = map(lambda x: x[0], instructions_retired.data)
    l1_cache_misses_values = map(lambda x: x[1], l2_cache_misses.data)
    instructions_retired_values = map(lambda x: x[1], instructions_retired.data)

    l2_cmpi.data = list(
        zip(timestamps, map(lambda x: x[0] / x[1], zip(l1_cache_misses_values, instructions_retired_values)))
    )

    return l2_cmpi


def calculate_virtual_metrics(dataset: PeriodicDataset) -> PeriodicDataset:
    l2_cache_misses = None

    for data_origin, data in dataset.data.items():
        if data_origin == DataOrigin.GLOBAL:
            for monitor_data in data:
                if monitor_data.event == L2CachePMUEvent.L2_CACHE_MISS:  # type: ignore
                    l2_cache_misses = monitor_data
                    break
            continue

        instructions_retired = None
        cycles = None
        l1_cache_misses = None

        for monitor_data in data:
            if monitor_data.event == PMUEvent.INSTRUCTIONS_RETIRED:  # type: ignore
                instructions_retired = monitor_data
            elif monitor_data.event == PMUEvent.CPU_CYCLES:  # type: ignore
                cycles = monitor_data
            elif monitor_data.event == PMUEvent.L1_CACHE_MISS:  # type: ignore
                l1_cache_misses = monitor_data

            if instructions_retired is not None and cycles is not None and l1_cache_misses is not None:
                break

        ipc = calculate_ipc(instructions_retired, cycles)  # type: ignore
        cpi = calculate_cpi(cycles, instructions_retired)  # type: ignore
        l1_cmpi = calculate_l1_cmpi(l1_cache_misses, instructions_retired)  # type: ignore
        l1_by_l2 = calculate_l1_by_l2(l1_cache_misses, l2_cache_misses)  # type: ignore
        l2_cmpi = calculate_l2_cmpi(l2_cache_misses, instructions_retired)  # type: ignore

        dataset.data[data_origin].append(ipc)  # type: ignore
        dataset.data[data_origin].append(cpi)  # type: ignore
        dataset.data[data_origin].append(l1_cmpi)  # type: ignore
        dataset.data[data_origin].append(l1_by_l2)  # type: ignore
        dataset.data[data_origin].append(l2_cmpi)  # type: ignore

    return dataset


def main(input_path: str, output_directory: str) -> None:
    with open(input_path, 'r', encoding='utf-8') as file:
        dataset = PeriodicDataset(**load(file))

    dataset = calculate_virtual_metrics(dataset)

    json_str = dataset.model_dump_json(indent=4)

    input_file_base_name_parts = input_path.split(sep)[-1].split('-')

    timestamp = datetime.now().strftime('%Y-%m-%d-%H-%M')
    tag = input_file_base_name_parts[5]

    collection_info = '-'.join(input_file_base_name_parts[6:-1])

    duration, _ = input_file_base_name_parts[-1].split('.')

    file_name = f'{timestamp}-{tag}-{collection_info}-{duration}-virtual.json'

    with open(join(output_directory, file_name), 'w+', encoding='utf-8') as file:
        file.write(json_str)


if __name__ == '__main__':
    parser = ArgumentParser(description='Generate a dataset with virtual metrics')
    parser.add_argument('input_path', help='Path to a periodic dataset')
    parser.add_argument('output_directory', help='Directory to save the periodic dataset with virtual metrics')

    args = parser.parse_args()

    main(args.input_path, args.output_directory)
