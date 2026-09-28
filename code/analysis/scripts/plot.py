import argparse
import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from json import load
from pandas import Series

from definitions import (
    DataOrigin,
    Dataset,
    L2CachePMUEvent,
    MonitorData,
    PMUEvent,
    SystemEvent,
)


def plot(
    title: str,
    out_dir: str,
    file_name: str,
    x_values: tuple,
    y_values_list: list[tuple],
    y_labels: tuple,
) -> None:
    fig, ax = plt.subplots(figsize=(16, 8), dpi=300)
    ax.set_title(title)
    ax.set_xlabel('Time (seconds)')
    ax.set_ylabel('Data Value')

    for i, y_values in enumerate(y_values_list):
        ax.plot(
            x_values,
            y_values,
            marker='.',
            markersize=2,
            linewidth=0.8,
            label=y_labels[i],
        )

    ax.grid(True, which='major', axis='x', linestyle='-', alpha=0.6)
    ax.grid(True, which='minor', axis='x', linestyle='--', alpha=0.4)
    ax.grid(True, which='major', axis='y', linestyle='--', alpha=0.5)

    ax.legend()
    plt.tight_layout()
    output_path = os.path.join(out_dir, file_name)
    plt.savefig(output_path, format='pdf', bbox_inches='tight')
    plt.close(fig)


def get_monitor_data(
    dataset: Dataset,
    data_origin: DataOrigin,
    event: SystemEvent | PMUEvent | L2CachePMUEvent,
) -> MonitorData | None:
    for monitor_data in dataset.data[data_origin]:
        if monitor_data.event == event:
            return monitor_data

    return None


def plot_running_threads(dataset: Dataset, out_dir: str) -> None:
    core_origins = [origin for origin in dataset.data.keys() if origin != DataOrigin.GLOBAL]

    if not core_origins:
        return

    core_data = {}
    unique_threads = set()

    for origin in core_origins:
        for monitor_data in dataset.data[origin]:
            event_name = getattr(monitor_data.event, 'value', str(monitor_data.event))
            if event_name == 'RUNNING_THREAD':
                core_data[origin] = monitor_data.data
                for _, tid in monitor_data.data:
                    unique_threads.add(tid)
                break

    if not core_data:
        return

    unique_threads = sorted(list(unique_threads))
    colors = plt.cm.tab10(np.linspace(0, 1, max(1, len(unique_threads))))
    color_map = {tid: colors[i] for i, tid in enumerate(unique_threads)}

    task_address_map = {}
    if hasattr(dataset, 'task_timings') and dataset.task_timings:
        task_address_map = {
            task['address']: task['task']
            for task in dataset.task_timings
            if isinstance(task, dict) and 'address' in task
        }

    fig, ax = plt.subplots(figsize=(16, 8), dpi=300)
    ax.set_title('Thread Execution Timeline (RUNNING_THREAD)')
    ax.set_xlabel('Time (seconds)')
    ax.set_ylabel('CPU Core')

    y_ticks = []
    y_labels = []

    sorted_origins = sorted(core_data.keys(), key=lambda o: str(o.name))

    for i, origin in enumerate(sorted_origins):
        thread_data = core_data[origin]
        if not thread_data:
            continue

        x_vals = [t / 1_000_000.0 for t, _ in thread_data]
        y_vals = [tid for _, tid in thread_data]

        y_base = i * 10
        y_ticks.append(y_base + 4)
        y_labels.append(str(origin.name).replace('_', ' ').title())

        blocks = []
        facecolors = []

        if len(x_vals) > 0:
            current_start = x_vals[0]
            current_tid = y_vals[0]

            for j in range(1, len(x_vals)):
                next_tid = y_vals[j]

                if next_tid != current_tid:
                    duration = x_vals[j] - current_start
                    blocks.append((current_start, duration))
                    facecolors.append(color_map[current_tid])

                    current_start = x_vals[j]
                    current_tid = next_tid

            last_duration = (x_vals[-1] - x_vals[-2]) if len(x_vals) > 1 else 0.001
            total_end = x_vals[-1] + last_duration
            blocks.append((current_start, total_end - current_start))
            facecolors.append(color_map[current_tid])

        ax.broken_barh(blocks, (y_base, 8), facecolors=facecolors, linewidth=0)

    ax.set_yticks(y_ticks)
    ax.set_yticklabels(y_labels)

    ax.grid(True, which='major', axis='x', linestyle='-', alpha=0.6)
    ax.grid(True, which='minor', axis='x', linestyle='--', alpha=0.4)

    legend_patches = []
    for tid in unique_threads:
        task_idx = task_address_map.get(tid)
        if task_idx is not None:
            label_str = f'Task [{task_idx}] (0x{tid:x})'
        else:
            label_str = f'Idle / Unknown (0x{tid:x})'

        legend_patches.append(mpatches.Patch(color=color_map[tid], label=label_str))

    ax.legend(handles=legend_patches, loc='upper right', bbox_to_anchor=(1.15, 1))

    plt.tight_layout()
    output_path = os.path.join(out_dir, 'plot_RUNNING_THREAD_timeline.pdf')
    plt.savefig(output_path, format='pdf', bbox_inches='tight')
    plt.close(fig)


def plot_data(dataset: Dataset, out_dir: str):
    os.makedirs(out_dir, exist_ok=True)

    plot_running_threads(dataset, out_dir)

    for monitor_data in dataset.data.get(DataOrigin.GLOBAL, []):
        event = monitor_data.event.value
        x_values, y_values = zip(*monitor_data.data)

        plot(
            f'Benchmark Data - {event} - Cumulative',
            out_dir,
            f'plot_{event}_cumulative.pdf',
            x_values,
            [y_values],
            ('Global',),
        )

        rates = tuple([float('nan')] + [current - previous for previous, current in zip(y_values[:-1], y_values[1:])])

        plot(
            f'Benchmark Data - {event} - Rate',
            out_dir,
            f'plot_{event}_rate.pdf',
            x_values,
            [rates],
            ('Global',),
        )

        series = Series(rates)
        moving_averages = tuple(series.rolling(window=34).mean().tolist())

        plot(
            f'Benchmark Data - {event} - Moving average',
            out_dir,
            f'plot_{event}_moving_average.pdf',
            x_values,
            [moving_averages],
            ('Global',),
        )

    for monitor_data in dataset.data.get(DataOrigin.CORE_1, []):
        event = monitor_data.event

        if getattr(event, 'value', str(event)) == 'RUNNING_THREAD':
            continue

        x_values_all, y_values_1_all = zip(*monitor_data.data)

        core2_data = get_monitor_data(dataset, DataOrigin.CORE_2, event)
        core3_data = get_monitor_data(dataset, DataOrigin.CORE_3, event)

        y_values_2_all = list(zip(*core2_data.data))[1] if core2_data else []
        y_values_3_all = list(zip(*core3_data.data))[1] if core3_data else []

        min_len = min(len(x_values_all), len(y_values_2_all), len(y_values_3_all))

        if min_len == 0:
            continue

        x_values = x_values_all[:min_len]
        y_values_1 = y_values_1_all[:min_len]
        y_values_2 = y_values_2_all[:min_len]
        y_values_3 = y_values_3_all[:min_len]

        event_name = event.value

        plot(
            f'Benchmark Data - {event_name} - Cumulative',
            out_dir,
            f'plot_{event_name}_cumulative.pdf',
            x_values,
            [y_values_1, y_values_2, y_values_3],
            ('Core 1', 'Core 2', 'Core 3'),
        )

        rates_1 = tuple(
            [float('nan')] + [current - previous for previous, current in zip(y_values_1[:-1], y_values_1[1:])]
        )

        rates_2 = tuple(
            [float('nan')] + [current - previous for previous, current in zip(y_values_2[:-1], y_values_2[1:])]
        )

        rates_3 = tuple(
            [float('nan')] + [current - previous for previous, current in zip(y_values_3[:-1], y_values_3[1:])]
        )

        plot(
            f'Benchmark Data - {event_name} - Rate',
            out_dir,
            f'plot_{event_name}_rate.pdf',
            x_values,
            [rates_1, rates_2, rates_3],
            ('Core 1', 'Core 2', 'Core 3'),
        )

        series_1 = Series(rates_1)
        series_2 = Series(rates_2)
        series_3 = Series(rates_3)

        moving_averages_1 = tuple(series_1.rolling(window=34).mean().tolist())
        moving_averages_2 = tuple(series_2.rolling(window=34).mean().tolist())
        moving_averages_3 = tuple(series_3.rolling(window=34).mean().tolist())

        plot(
            f'Benchmark Data - {event_name} - Moving average',
            out_dir,
            f'plot_{event_name}_moving_average.pdf',
            x_values,
            [moving_averages_1, moving_averages_2, moving_averages_3],
            ('Core 1', 'Core 2', 'Core 3'),
        )


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Plot EPOS parsed log data as PDFs.')
    parser.add_argument('json_file', help='Path to the parsed JSON data file')
    parser.add_argument(
        '--outdir',
        default='.',
        help='Directory to save the PDF plots (default: current directory)',
    )
    args = parser.parse_args()

    with open(args.json_file, 'r', encoding='utf-8') as file:
        dataset = Dataset(**load(file))

    plot_data(dataset, args.outdir)
