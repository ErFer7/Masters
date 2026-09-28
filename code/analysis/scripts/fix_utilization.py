from sys import argv
from json import load

from definitions import (
    DataOrigin,
    Dataset,
    SystemEvent,
)


def fix_utilization(dataset: Dataset) -> Dataset:
    for data_origin in DataOrigin:
        if data_origin == DataOrigin.GLOBAL:
            continue

        running_thread = None
        job_utilization = None

        for monitor_data in dataset.data[data_origin]:
            if monitor_data.event == SystemEvent.RUNNING_THREAD:
                running_thread = monitor_data
            elif monitor_data.event == SystemEvent.JOB_UTILIZATION:
                job_utilization = monitor_data

            if running_thread is not None and job_utilization is not None:
                break

        idle = running_thread.data[0][1]  # type: ignore

        threads_indices = {idle: [0, 0]}

        current_thread = idle
        last_thread = idle

        for i, (_, thread) in enumerate(running_thread.data):  # type: ignore
            if thread != current_thread:
                threads_indices[last_thread][1] = i - 1

                if thread not in threads_indices:
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
                last_thread = running_thread.data[i - 1][1]  # type: ignore

    return dataset


def main(source_path: str, destination_path: str) -> None:
    with open(source_path, 'r', encoding='utf-8') as file:
        dataset = Dataset(**load(file))

    dataset = fix_utilization(dataset)
    json_str = dataset.model_dump_json(indent=4)

    with open(destination_path, 'w+', encoding='utf-8') as file:
        file.write(json_str)


if __name__ == '__main__':
    main(argv[1], argv[2])
