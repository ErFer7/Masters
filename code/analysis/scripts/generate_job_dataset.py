from argparse import ArgumentParser
from datetime import datetime
from json import load
from os import sep
from os.path import join

import pandas as pd

from definitions.data_models import DataStream, SimplePeriodicDataset, Taskset
from definitions.data_origins import DataOrigin
from definitions.events import SystemEvent

NON_CUMULATIVE_EVENTS = {
    SystemEvent.RUNNING_THREAD,
    SystemEvent.JOB_UTILIZATION,
}


def get_stream(streams: list[DataStream], event) -> DataStream:
    return next(s for s in streams if s.event == event)


def identify_idle_thread(streams: list[DataStream], taskset: Taskset) -> int:
    running_thread_stream = get_stream(streams, SystemEvent.RUNNING_THREAD)

    known_threads = {task.thread for task in taskset.tasks.values()}
    observed_threads = set(running_thread_stream.data)

    idle_candidates = observed_threads - known_threads

    if len(idle_candidates) == 1:
        return idle_candidates.pop()  # type: ignore

    if len(idle_candidates) == 0:
        raise ValueError('Could not identify an idle thread: every observed value matches a known task.')

    fallback = running_thread_stream.data[0]
    print(f'WARNING: ambiguous idle candidates {idle_candidates}, falling back to first sample {fallback}.')
    return fallback  # type: ignore


def compute_run_ids(thread_values: tuple) -> list[int]:
    run_ids = [0] * len(thread_values)

    for i in range(1, len(thread_values)):
        run_ids[i] = run_ids[i - 1] + (1 if thread_values[i] != thread_values[i - 1] else 0)

    return run_ids


def find_run_boundaries(run_ids: list[int]) -> list[tuple[int, int]]:
    """Returns (start_index, end_index) pairs, end exclusive, one per contiguous run."""

    boundaries = []
    run_start = 0

    for i in range(1, len(run_ids) + 1):
        if i == len(run_ids) or run_ids[i] != run_ids[run_start]:
            boundaries.append((run_start, i))
            run_start = i

    return boundaries


def sum_range(values: tuple, start: int, end: int, skip_first: bool) -> int | float | None:
    """Sums values[start:end]. If skip_first, the sample at `start` is excluded -- this is
    the boundary sample of a run, whose delta is None/unattributable and must not enter
    the sum. Returns None if no valid samples remain in range.

    `end` is clamped to len(values): different DataOrigins can end up with slightly
    different sample counts (see the length-mismatch check in generate_job_dataset),
    so a run's index range computed from one origin's length may overrun another's."""

    end = min(end, len(values))
    start = min(start, end)

    sample_range = range(start + 1, end) if skip_first else range(start, end)
    valid = [values[j] for j in sample_range if values[j] is not None]

    return sum(valid) if valid else None


def aggregate_core_jobs(
    streams: list[DataStream], taskset: Taskset, global_streams: list[DataStream], core_name: str
) -> list[dict]:
    running_thread = get_stream(streams, SystemEvent.RUNNING_THREAD)
    utilization = get_stream(streams, SystemEvent.JOB_UTILIZATION)

    thread_values = running_thread.data
    run_ids = compute_run_ids(thread_values)  # type: ignore
    boundaries = find_run_boundaries(run_ids)

    idle_thread = identify_idle_thread(streams, taskset)
    thread_to_benchmark = {task.thread: task.benchmark for task in taskset.tasks.values()}

    cumulative_streams = [s for s in streams if s.event not in NON_CUMULATIVE_EVENTS]

    rows = []

    for run_id, (start, end) in enumerate(boundaries):
        thread = thread_values[start]
        is_idle = thread == idle_thread

        row: dict = {
            'core': core_name,
            'run_id': run_id,
            'thread': thread,
            'is_idle': is_idle,
            'benchmark': 'IDLE' if is_idle else thread_to_benchmark.get(thread, 'UNKNOWN'),
            'start_index': start,
            'end_index': end,
            'n_samples': end - start,
            # The first sample of every run is a boundary sample (delta spans the
            # previous thread/idle -> this one and can't be attributed). It's excluded
            # from every sum below, so n_valid_samples is usually n_samples - 1.
            'n_valid_samples': max(0, (end - start) - 1),
            # JOB_UTILIZATION is a point value replicated across the whole run, not a
            # delta -- take it as-is rather than summing it.
            'utilization': utilization.data[start],
        }

        for stream in cumulative_streams:
            row[str(stream.event)] = sum_range(stream.data, start, end, skip_first=True)

        # Global/shared counters have no run concept of their own -- sum over the same
        # positional window as this core's run. This is still an approximation: if other
        # cores were active during this window, their activity is mixed into this sum
        # (see contention discussion). start is included here (not skipped) since it is
        # a valid GLOBAL sample, just not a valid sample for *this core's* run.
        for stream in global_streams:
            row[f'GLOBAL_{stream.event}'] = sum_range(stream.data, start, end, skip_first=False)

        rows.append(row)

    return rows


def check_stream_lengths(dataset: SimplePeriodicDataset) -> None:
    """Different DataOrigins can end up with slightly different sample counts (collection
    start/stop jitter). A difference of a sample or two is expected and handled by the
    clamping in sum_range; a larger gap suggests something is actually wrong upstream
    (dropped samples, a stalled collector, etc.) and is worth investigating before trusting
    the aggregated output."""

    lengths = {}

    for data_origin, streams in dataset.data.items():
        stream_lengths = {str(s.event): len(s.data) for s in streams}
        lengths[str(data_origin)] = stream_lengths

        unique_lengths = set(stream_lengths.values())
        if len(unique_lengths) > 1:
            print(f'WARNING: streams within {data_origin} disagree on length: {stream_lengths}')

    origin_lengths = {origin: next(iter(lens.values())) for origin, lens in lengths.items()}
    max_len, min_len = max(origin_lengths.values()), min(origin_lengths.values())

    if max_len - min_len > 2:
        print(f'WARNING: sample counts differ by more than 2 across origins: {origin_lengths}')
    elif max_len != min_len:
        print(f'NOTE: minor sample count mismatch across origins (likely start/stop jitter): {origin_lengths}')


def generate_job_dataset(dataset: SimplePeriodicDataset) -> pd.DataFrame:
    check_stream_lengths(dataset)

    global_streams = dataset.data[DataOrigin.GLOBAL]

    all_rows = []

    for data_origin, streams in dataset.data.items():
        if data_origin == DataOrigin.GLOBAL:
            continue

        all_rows.extend(aggregate_core_jobs(streams, dataset.taskset, global_streams, str(data_origin)))  # type: ignore

    return pd.DataFrame(all_rows)


def main(input_path: str, output_directory: str) -> None:
    with open(input_path, 'r', encoding='utf-8') as file:
        dataset = SimplePeriodicDataset(**load(file))

    df = generate_job_dataset(dataset)

    input_file_base_name_parts = input_path.split(sep)[-1].split('-')

    timestamp = datetime.now().strftime('%Y-%m-%d-%H-%M')
    tag = input_file_base_name_parts[5]

    collection_info = '-'.join(input_file_base_name_parts[6:-1])

    ending, _ = input_file_base_name_parts[-1].split('.')

    file_name = f'{timestamp}-{tag}-{collection_info}-{ending}-jobs.csv'

    df.to_csv(join(output_directory, file_name), index=False)


if __name__ == '__main__':
    parser = ArgumentParser(description='Aggregate a rate dataset into one row per job iteration')
    parser.add_argument('input_path', help='Path to a rate dataset')
    parser.add_argument('output_directory', help='Directory to save the per-job CSV')

    args = parser.parse_args()

    main(args.input_path, args.output_directory)
