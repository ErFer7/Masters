import argparse
from json import load
from math import inf, log1p, sqrt
from random import shuffle

from definitions import (
    TASKSETS,
    AuxiliaryEvents,
    DataOrigin,
    PMUEvent,
    L2CachePMUEventVariance,
    PMUEventVariance,
    L2CachePMUEvent,
    SystemEvent,
    TaskRateDataset,
    MonitorData,
)

TARGET_FEATURE = SystemEvent.JOB_UTILIZATION
NON_ACCUMULATING_FEATURES = (SystemEvent.CPU_CLOCK,)
IGNORED_FEATURES = (
    SystemEvent.DEADLINE_MISSES,
    SystemEvent.CORE,
    # PMUEventVariance.CPU_CYCLES_VARIANCE,
    # PMUEventVariance.INSTRUCTIONS_RETIRED_VARIANCE,
    # PMUEventVariance.L1_CACHE_MISS_VARIANCE,
    # PMUEventVariance.BRANCH_DIRECTION_MISPREDICTION_VARIANCE,
    L2CachePMUEventVariance.L2_INNER_ACQUIREBLOCK_HIT_L2_VARIANCE,
    L2CachePMUEventVariance.L2_INNER_RELEASEDATA_TTON_VARIANCE,
    L2CachePMUEventVariance.L2_CACHE_MISS_VARIANCE,
    L2CachePMUEventVariance.L2_DEMAND_MISS_HIT_MSHR_ALLOC_HINT_VARIANCE,
    L2CachePMUEventVariance.L2_INNER_PROBEBLOCK_TON_STORE_MISS_VARIANCE
)
OUTLIER_THRESHOLD = 1e15
MIN_CPU_CLOCK = 375999999
MAX_CPU_CLOCK = 1503999999
MIN_SAFE_CPU_CLOCK = 501333333
NEXT_FREQUENCY = {MAX_CPU_CLOCK: 751999999, 751999999: 501333333, 501333333: MIN_CPU_CLOCK}


def group_by_hyperperiod(
    task_data: list[MonitorData], hyperperiod: int
) -> dict[
    int, dict[SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance, list[int | float]]
]:
    hyperperiod_timeline: dict[
        int,
        dict[SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance, list[int | float]],
    ] = {}

    for monitor_data in task_data:
        event = monitor_data.event

        if event in IGNORED_FEATURES:
            continue

        for ts, value in monitor_data.data:
            if value >= OUTLIER_THRESHOLD:
                print(f'Outlier detected in event {event.name} with timestamp {ts} and value {value}')
                value = OUTLIER_THRESHOLD

            hyperperiod_index = int(ts) // hyperperiod

            if hyperperiod_index not in hyperperiod_timeline:
                hyperperiod_timeline[hyperperiod_index] = {}

            if event not in hyperperiod_timeline[hyperperiod_index]:
                hyperperiod_timeline[hyperperiod_index][event] = []

            hyperperiod_timeline[hyperperiod_index][event].append(value)

    return hyperperiod_timeline


def calculate_utilization(
    hyperperiod_timeline: dict[
        int,
        dict[SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance, list[int | float]],
    ],
    period: int,
) -> dict[
    int, dict[SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance, list[int | float]]
]:
    for hyperperiod_index, event_data in hyperperiod_timeline.items():
        hyperperiod_timeline[hyperperiod_index][SystemEvent.JOB_UTILIZATION] = list(
            map(lambda x: x / period, event_data[SystemEvent.JOB_UTILIZATION])
        )

    return hyperperiod_timeline


def calculate_hyperperiod_average(
    hyperperiod_timeline: dict[
        int,
        dict[SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance, list[int | float]],
    ],
) -> dict[int, dict[SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance, float]]:
    for hyperperiod_index, event_data in hyperperiod_timeline.items():
        for event, data in event_data.items():
            hyperperiod_timeline[hyperperiod_index][event] = sum(data) / len(data)  # type: ignore

    return hyperperiod_timeline  # type: ignore


# Counters (PMU/L2 events and their variances) are non-negative and heavy-tailed: mostly
# near zero with occasional bursts. log1p compresses that tail before z-scoring so a handful
# of large-but-legitimate values don't dominate the mean/std the way they dominate a min/max.
# JOB_UTILIZATION is the (unnormalized) target/current-utilization feature and is left alone,
# same as before. CPU_CLOCK is a near-uniform, quantized DVFS level, not a heavy-tailed
# counter, so it's z-scored directly without the log1p step.
LOG_TRANSFORM_EXCLUDED_EVENTS = (SystemEvent.JOB_UTILIZATION, SystemEvent.CPU_CLOCK)


def transform_value(
    event: SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance, value: float
) -> float:
    if event in LOG_TRANSFORM_EXCLUDED_EVENTS:
        return value
    return log1p(max(value, 0.0))


def z_score_normalized(
    value: float,
    event: SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance,
    mean: float,
    std: float,
    clip_sigma: float = 3.0,
) -> float:
    transformed = transform_value(event, value)

    if std <= 1e-9:
        z = 0.0
    else:
        z = (transformed - mean) / std

    z = max(-clip_sigma, min(clip_sigma, z))

    # Rescale from [-clip_sigma, clip_sigma] to [0, 1] so the FANN input range matches
    # what the old min-max normalization produced.
    return (z + clip_sigma) / (2 * clip_sigma)


FeatureDict = dict[
    SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance | AuxiliaryEvents, float
]


def load_task_dataset(source_path: str) -> tuple[TaskRateDataset, list[list[FeatureDict]]]:
    """Load a source JSON file and build the per-task, per-hyperperiod averaged feature
    series, WITHOUT normalizing. This is the shared unnormalized representation used both
    for computing normalization bounds and for producing the final FANN output."""
    with open(source_path, 'r', encoding='utf-8') as file:
        dataset = TaskRateDataset(**load(file))

    selected_taskset = TASKSETS[dataset.taskset]

    # TODO: Implement the core based separation
    task_dataset: list[list[FeatureDict]] = []

    for task in range(len(selected_taskset.tasks)):
        task_dataset.append([])

        hyperperiod_timeline = group_by_hyperperiod(dataset.data[task], selected_taskset.hyperperiod)
        calculate_utilization(hyperperiod_timeline, selected_taskset.tasks[task].period)
        calculate_hyperperiod_average(hyperperiod_timeline)

        for event_data in hyperperiod_timeline.values():
            task_dataset[task].append(event_data)  # type: ignore

        task_dataset[task] = list(
            filter(lambda series: series[SystemEvent.CPU_CLOCK] > MIN_SAFE_CPU_CLOCK, task_dataset[task])
        )

    return dataset, task_dataset


class RunningSum:
    __slots__ = ('sum', 'sum_sq', 'count')

    def __init__(self) -> None:
        self.sum = 0.0
        self.sum_sq = 0.0
        self.count = 0


def update_stats(
    task_dataset: list[list[FeatureDict]],
    stats: dict[SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance, RunningSum],
) -> None:
    """Fold the (log1p-transformed, where applicable) values found in task_dataset into the
    running sum/sum-of-squares accumulators used to compute mean/std per feature. Call this
    once per dataset file that should contribute to the normalization statistics."""
    for task_series in task_dataset:
        for data in task_series:
            for event, value in data.items():
                if event == SystemEvent.JOB_UTILIZATION:
                    continue

                if event not in stats:
                    stats[event] = RunningSum()

                transformed = transform_value(event, value)
                stats[event].sum += transformed
                stats[event].sum_sq += transformed * transformed
                stats[event].count += 1


def finalize_stats(
    stats: dict[SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance, RunningSum],
) -> tuple[
    dict[SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance, float],
    dict[SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance, float],
]:
    """Turn accumulated sum/sum-of-squares into per-feature mean/std."""
    mean_values: dict[SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance, float] = {}
    std_values: dict[SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance, float] = {}

    for event, running in stats.items():
        mean = running.sum / running.count
        variance = max(running.sum_sq / running.count - mean * mean, 0.0)
        mean_values[event] = mean
        std_values[event] = sqrt(variance)

    return mean_values, std_values


def main() -> None:
    parser = argparse.ArgumentParser(description='Convert TaskRateDataset to FANN input format.')
    parser.add_argument('source_path', help='Path to the source JSON dataset to convert')
    parser.add_argument('destination_path', help='Path to the destination FANN file')
    parser.add_argument('--merged', action='store_true', help='Output all tasks unified in a single dataset')
    parser.add_argument('--shuffled', action='store_true', help='Randomly shuffle the generated test data')
    parser.add_argument(
        '--calibration',
        nargs='*',
        default=[],
        metavar='JSON_PATH',
        help=(
            'Additional source JSON dataset paths whose values are folded into the '
            'normalization statistics (mean/std), without being converted themselves. Use '
            'this to compute global stats across multiple datasets (e.g. pass the test set '
            'path when generating the train file, and vice versa) instead of per-file local '
            'stats.'
        ),
    )
    parser.add_argument(
        '--average-target',
        action='store_true',
        help=(
            "Use each task's whole-dataset average JOB_UTILIZATION as the prediction target "
            "for every row of that task, instead of the next hyperperiod's utilization. "
            'The average is a fixed statistic over the entire task series (not a moving '
            'average) and every row for that task gets the same target value. Useful as a '
            'naive baseline to compare the trained model against.'
        ),
    )

    args = parser.parse_args()

    dataset, task_dataset = load_task_dataset(args.source_path)
    selected_taskset = TASKSETS[dataset.taskset]

    stats: dict[SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance, RunningSum] = {}

    # Always include the file we're converting.
    update_stats(task_dataset, stats)

    # Fold in any additional calibration files so mean/std reflect the union of datasets,
    # not just the one being converted right now.
    for calibration_path in args.calibration:
        _, calibration_task_dataset = load_task_dataset(calibration_path)
        update_stats(calibration_task_dataset, stats)

    mean_values, std_values = finalize_stats(stats)

    # NOTE: Maybe add the period later
    for task in range(len(selected_taskset.tasks)):
        for i, data in enumerate(task_dataset[task]):
            for event, value in data.items():
                if event != SystemEvent.JOB_UTILIZATION:
                    task_dataset[task][i][event] = z_score_normalized(
                        value, event, mean_values[event], std_values[event]
                    )

                # print(f'Event: {event.name}: {value} -> {task_dataset[task][i][event]}')
            # print()

        if args.average_target:
            # Fixed statistic over the whole task series (not a moving/windowed average) —
            # every row for this task gets the same target value. No "next" sample is
            # needed, so unlike the default mode, the last row doesn't need to be dropped.
            task_average_utilization = sum(data[SystemEvent.JOB_UTILIZATION] for data in task_dataset[task]) / len(
                task_dataset[task]
            )

            for i in range(len(task_dataset[task])):
                task_dataset[task][i][AuxiliaryEvents.NEXT_JOB_UTILIZATION] = task_average_utilization
        else:
            for i in range(len(task_dataset[task]) - 1):
                task_dataset[task][i][AuxiliaryEvents.NEXT_JOB_UTILIZATION] = task_dataset[task][i + 1][
                    SystemEvent.JOB_UTILIZATION
                ]

            task_dataset[task].pop()

    if args.merged:
        hyperperiod_series = []

        for task in range(len(selected_taskset.tasks)):
            hyperperiod_series += task_dataset[task]

        if args.shuffled:
            shuffle(hyperperiod_series)

        ann_dataset = f'{len(hyperperiod_series)} {len(hyperperiod_series[0].keys()) - 1} 1'

        for data in hyperperiod_series:
            line = '\n'

            for i, (event, value) in enumerate(data.items()):
                if event != AuxiliaryEvents.NEXT_JOB_UTILIZATION:
                    line += f'{value:.04f}' + ' ' if i < len(data.values()) - 1 else ''

            ann_dataset += line
            ann_dataset += f'\n{data[AuxiliaryEvents.NEXT_JOB_UTILIZATION]:.04f}'

        with open(args.destination_path, 'w+', encoding='utf-8') as file:
            file.write(ann_dataset)
    else:
        cores_hyperperiod_series = {DataOrigin.CORE_1: [], DataOrigin.CORE_2: [], DataOrigin.CORE_3: []}

        for task in range(len(selected_taskset.tasks)):
            cores_hyperperiod_series[selected_taskset.assigned_core_map[task]] += task_dataset[task]

        for core in cores_hyperperiod_series:
            if args.shuffled:
                shuffle(cores_hyperperiod_series[core])

            ann_dataset = f'{len(cores_hyperperiod_series[core])} {len(cores_hyperperiod_series[core][0].keys()) - 1} 1'

            for data in cores_hyperperiod_series[core]:
                line = '\n'

                for i, (event, value) in enumerate(data.items()):
                    if event != AuxiliaryEvents.NEXT_JOB_UTILIZATION:
                        line += f'{value:.04f}' + ' ' if i < len(data.values()) - 1 else ''

                ann_dataset += line
                ann_dataset += f'\n{data[AuxiliaryEvents.NEXT_JOB_UTILIZATION]:.04f}'

            destination_path = args.destination_path[:-5] + f'_{core.value}' + '.data'

            with open(destination_path, 'w+', encoding='utf-8') as file:
                file.write(ann_dataset)


if __name__ == '__main__':
    main()
