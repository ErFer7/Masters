from argparse import ArgumentParser
from datetime import datetime
from json import load
from pathlib import Path
from re import search
from os import sep
from os.path import join

from deepdiff import DeepDiff
from definitions.data_models import PeriodicDataset, RunData, Taskset
from definitions.data_origins import DataOrigin
from definitions.events import PMUEvent, PMUEventVariance, SystemEvent


def get_run_test_duration(runs: list[RunData]) -> int:
    test_duration = runs[0].test_duration

    for run in runs[1:]:
        if run.test_duration != test_duration:
            print(f'WARNING: There are different test durations! -> {run.test_duration} != {test_duration}')

    return test_duration


def get_run_rng_seed(runs: list[RunData]) -> int:
    rng_seed = runs[0].rng_seed

    for run in runs[1:]:
        if run.rng_seed != rng_seed:
            print(f'WARNING: There are different RNG seeds! -> {run.rng_seed} != {rng_seed}')

    return rng_seed


def get_run_taskset(runs: list[RunData]) -> Taskset:
    taskset = runs[0].taskset

    for task_index in taskset.tasks:
        taskset.tasks[task_index].task_timing = None

    for run in runs[1:]:
        for task_index in run.taskset.tasks:
            run.taskset.tasks[task_index].task_timing = None

        diff = DeepDiff(run.taskset.model_dump(), taskset.model_dump())

        if diff:
            print(f'WARNING: There are different tasksets! -> {diff}')

    return taskset


def merge(runs: list[RunData]) -> PeriodicDataset:
    periodic_dataset = PeriodicDataset()

    periodic_dataset.test_duration = get_run_test_duration(runs)
    periodic_dataset.rng_seed = get_run_rng_seed(runs)
    periodic_dataset.taskset = get_run_taskset(runs)

    periodic_dataset.data[DataOrigin.GLOBAL] = {}
    periodic_dataset.data[DataOrigin.CORE_1] = {}
    periodic_dataset.data[DataOrigin.CORE_2] = {}
    periodic_dataset.data[DataOrigin.CORE_3] = {}

    timestamps = {
        DataOrigin.GLOBAL: [],
        DataOrigin.CORE_1: [],
        DataOrigin.CORE_2: [],
        DataOrigin.CORE_3: [],
    }

    for benchmark in runs:
        for core, data in benchmark.data.items():
            if core == 0:
                for monitor_data in data:
                    event = monitor_data.event  # type: ignore

                    if (
                        not isinstance(event, PMUEvent)
                        and not isinstance(event, PMUEventVariance)
                        and event not in periodic_dataset.data[DataOrigin.GLOBAL]
                    ):
                        periodic_dataset.data[DataOrigin.GLOBAL][event] = monitor_data  # type: ignore
                        timestamps[DataOrigin.GLOBAL].append(list(zip(*monitor_data.data))[0])  # type: ignore
            else:
                data_origin = list(DataOrigin)[core]

                for monitor_data in data:
                    event = monitor_data.event  # type: ignore

                    if event not in periodic_dataset.data[data_origin]:
                        periodic_dataset.data[data_origin][event] = monitor_data  # type: ignore
                        timestamps[data_origin].append(list(zip(*monitor_data.data))[0])  # type: ignore

    merged_timestamps = {
        DataOrigin.GLOBAL: [],
        DataOrigin.CORE_1: [],
        DataOrigin.CORE_2: [],
        DataOrigin.CORE_3: [],
    }

    for data_origin, timestamp_lists in timestamps.items():
        timestamp_average_calc = []
        timestamp_length = len(timestamp_lists[0])

        for timestamp_list in timestamp_lists[1:]:
            timestamp_list_length = len(timestamp_list)

            if timestamp_list_length != timestamp_length:
                print(
                    f'WARNING: There are different collection list lengths! -> {timestamp_list_length} != {timestamp_length}'
                )

            if timestamp_list_length < timestamp_length:
                timestamp_length = timestamp_list_length

        for i in range(timestamp_length):
            timestamp_average_calc.append([0, 0])

            for timestamp_list in timestamp_lists:
                if i >= len(timestamp_list):
                    continue

                timestamp_average_calc[i][0] += timestamp_list[i]
                timestamp_average_calc[i][1] += 1

        for sum, count in timestamp_average_calc:
            merged_timestamps[data_origin].append(sum // count)

    for timestamps in merged_timestamps.values():
        first = timestamps[0]

        for i in range(len(timestamps)):
            timestamps[i] -= first  # type: ignore

    for data_origin, data in periodic_dataset.data.items():
        data_cap_length = len(merged_timestamps[data_origin])

        for _, monitor_data in data.items():  # type: ignore
            previous_monitor_data_length = len(monitor_data.data)
            monitor_data.data = monitor_data.data[:data_cap_length]
            monitor_data_length = len(monitor_data.data)

            print(
                f'Monitor data of event {monitor_data.event.value} from {data_origin.value} capped from {previous_monitor_data_length} to {monitor_data_length}'
            )

            for i in range(len(monitor_data.data)):
                monitor_data.data[i] = (
                    merged_timestamps[data_origin][i],
                    monitor_data.data[i][1],
                )

        periodic_dataset.data[data_origin] = list(data.values())  # type: ignore

    periodic_dataset.collection_duration = (
        periodic_dataset.data[DataOrigin.GLOBAL][0].data[-1][0] - periodic_dataset.data[DataOrigin.GLOBAL][0].data[0][0]  # type: ignore
    )

    return periodic_dataset


def fix_utilization(periodic_dataset: PeriodicDataset) -> PeriodicDataset:
    for data_origin in DataOrigin:
        if data_origin == DataOrigin.GLOBAL:
            continue

        running_thread = None
        job_utilization = None

        for monitor_data in periodic_dataset.data[data_origin]:
            if monitor_data.event == SystemEvent.RUNNING_THREAD:  # type: ignore
                running_thread = monitor_data
            elif monitor_data.event == SystemEvent.JOB_UTILIZATION:  # type: ignore
                job_utilization = monitor_data

            if running_thread is not None and job_utilization is not None:
                break

        idle = running_thread.data[0][1]  # type: ignore

        threads_indices = {idle: [0, 0]}
        current_thread = idle

        for i, (_, thread) in enumerate(running_thread.data):  # type: ignore
            if thread != current_thread:
                threads_indices[current_thread][1] = i - 1  # close the window we're LEAVING

                if thread == idle:
                    pass
                elif thread not in threads_indices:
                    threads_indices[thread] = [i, i]
                else:
                    utilization = job_utilization.data[i][1]  # type: ignore

                    for j in range(threads_indices[thread][0], threads_indices[thread][1] + 1):
                        job_utilization.data[j] = (  # type: ignore
                            job_utilization.data[j][0],  # type: ignore
                            utilization,
                        )

                threads_indices[thread][0] = i
                current_thread = thread

    return periodic_dataset


def main(input_directory: str, output_directory: str) -> None:
    runs = []
    directory = Path(input_directory)

    # Sort by run number
    file_paths = sorted(directory.glob('*.json'), key=lambda x: int(search(r'run-(\d+)-', x.name).group(1)))  # type: ignore

    for benchmark_file_path in file_paths:
        with open(benchmark_file_path, 'r', encoding='utf-8') as file:
            runs.append(RunData(**load(file)))

    dataset = merge(runs)
    fixed_dataset = fix_utilization(dataset)
    json_str = fixed_dataset.model_dump_json(indent=4)

    input_file_base_name_parts = str(file_paths[0]).split(sep)[-1].split('-')

    timestamp = datetime.now().strftime('%Y-%m-%d-%H-%M')
    tag = input_file_base_name_parts[5]

    run_tag_index = input_file_base_name_parts.index('run')

    collection_info = '-'.join(input_file_base_name_parts[6:run_tag_index])

    frequency = input_file_base_name_parts[-2]
    duration_and_extension = input_file_base_name_parts[-1]

    file_name = f'{timestamp}-{tag}-{collection_info}-{frequency}-{duration_and_extension}'

    with open(join(output_directory, file_name), 'w+', encoding='utf-8') as file:
        file.write(json_str)


if __name__ == '__main__':
    parser = ArgumentParser(description='Merge structured runs into a single dataset')
    parser.add_argument('input_directory', help='Path to a directory to load run files')
    parser.add_argument('output_directory', help='Path to a directory to save the dataset')

    args = parser.parse_args()

    main(args.input_directory, args.output_directory)
