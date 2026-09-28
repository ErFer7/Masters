import argparse
import json
import os
import matplotlib.pyplot as plt
import matplotlib.transforms as mtransforms
from collections import defaultdict

from definitions import SystemEvent, TaskRateDataset, TASKSET_1

SELECTED_TASKSET = TASKSET_1
HYPERPERIOD = TASKSET_1.hyperperiod
OUTLIER_THRESHOLD = 1e15


def plot_core_utilization(dataset_path: str, output_dir: str, limit_seconds: float | None = None) -> None:
    with open(dataset_path, 'r', encoding='utf-8') as file:
        dataset = TaskRateDataset(**json.load(file))

    core_utilization = defaultdict(lambda: defaultdict(float))
    core_pred_utilization = defaultdict(lambda: defaultdict(float))
    global_freq = {}

    # Track core allocation per task per window for migrations
    task_cores = defaultdict(dict)
    all_windows_set = set()

    for task_id, monitor_list in dataset.data.items():
        # Look up the task's period to determine how many executions make up one hyperperiod
        task_info = SELECTED_TASKSET.tasks[int(task_id)]
        jobs_per_window = HYPERPERIOD // task_info.period

        windowed_utilization = defaultdict(float)
        windowed_pred_utilization = defaultdict(float)
        windowed_core = {}

        for monitor_data in monitor_list:
            event = monitor_data.event

            if event not in (
                SystemEvent.JOB_UTILIZATION,
                SystemEvent.PREDICTED_JOB_UTILIZATION,
                SystemEvent.CORE,
                SystemEvent.CPU_CLOCK,
            ):
                continue

            if not monitor_data.data:
                continue

            # Anchor the sequence to real time using the very first timestamp
            first_ts = monitor_data.data[0][0]
            base_window = int(first_ts) // HYPERPERIOD

            for i, (ts, value) in enumerate(monitor_data.data):
                if value >= OUTLIER_THRESHOLD:
                    continue

                if event == SystemEvent.CPU_CLOCK:
                    # System events use strict timestamps to stay perfectly globally synced
                    window = int(ts) // HYPERPERIOD
                else:
                    # Task events use the anchored sequence to mathematically prevent time-shift overlap
                    window = base_window + (i // jobs_per_window)

                all_windows_set.add(window)

                if event == SystemEvent.JOB_UTILIZATION:
                    windowed_utilization[window] += value
                elif event == SystemEvent.PREDICTED_JOB_UTILIZATION:
                    windowed_pred_utilization[window] = value
                elif event == SystemEvent.CORE:
                    if window not in windowed_core:
                        windowed_core[window] = value
                elif event == SystemEvent.CPU_CLOCK:
                    if window not in global_freq:
                        global_freq[window] = value

        for window, util in windowed_utilization.items():
            core_id = windowed_core.get(window)
            if core_id is not None:
                core_utilization[core_id][window] += (util / HYPERPERIOD) * 100.0

        for window, pred_util in windowed_pred_utilization.items():
            core_id = windowed_core.get(window)
            if core_id is not None:
                core_pred_utilization[core_id][window] += (pred_util / HYPERPERIOD) * 100.0

        for window, core_id in windowed_core.items():
            task_cores[task_id][window] = core_id

    # --- Migration Detection ---
    all_windows = sorted(list(all_windows_set))
    prev_allocation = {}
    migrations = []
    migration_id = 1

    for w in all_windows:
        curr_allocation = {}
        for t_id in dataset.data.keys():
            if w in task_cores[t_id]:
                curr_allocation[t_id] = task_cores[t_id][w]
            elif t_id in prev_allocation:
                curr_allocation[t_id] = prev_allocation[t_id]

        if prev_allocation:
            migrating_tasks = []
            for t_id, c_id in curr_allocation.items():
                if t_id in prev_allocation and prev_allocation[t_id] != c_id:
                    migrating_tasks.append(t_id)

            swaps = []
            singles = []
            skip_tasks = set()

            for i, t1 in enumerate(migrating_tasks):
                if t1 in skip_tasks:
                    continue
                is_swap = False
                for j in range(i + 1, len(migrating_tasks)):
                    t2 = migrating_tasks[j]
                    if t2 in skip_tasks:
                        continue

                    if prev_allocation[t1] == curr_allocation[t2] and prev_allocation[t2] == curr_allocation[t1]:
                        swaps.append((t1, t2))
                        skip_tasks.add(t1)
                        skip_tasks.add(t2)
                        is_swap = True
                        break

                if not is_swap:
                    singles.append(t1)

            time_sec = w * (HYPERPERIOD / 1_000_000.0)

            for t1, t2 in swaps:
                migrations.append(
                    {
                        'id': migration_id,
                        'time': time_sec,
                        'desc': f'Swap Task {t1} (Core {prev_allocation[t1]}) and Task {t2} (Core {prev_allocation[t2]})',
                    }
                )
                migration_id += 1

            for t in singles:
                migrations.append(
                    {
                        'id': migration_id,
                        'time': time_sec,
                        'desc': f'Single: Task {t} from Core {prev_allocation[t]} to Core {curr_allocation[t]}',
                    }
                )
                migration_id += 1

        prev_allocation = curr_allocation.copy()

    # Print migrations to console for your table
    print('--- Detected Migrations ---')
    for mig in migrations:
        if limit_seconds is not None and mig['time'] > limit_seconds:
            continue
        print(f'[{mig["id"]}] Time: {mig["time"]:.2f}s | {mig["desc"]}')
    print('---------------------------')

    # --- DVFS Actuation Accuracy ---
    all_cores = sorted(set(core_utilization.keys()).union(core_pred_utilization.keys()))
    print('--- DVFS Actuation Predictions ---')
    freq_sorted_windows = sorted(global_freq.keys())
    for i in range(1, len(freq_sorted_windows)):
        prev_w = freq_sorted_windows[i - 1]
        curr_w = freq_sorted_windows[i]

        # Only evaluate on DVFS decrease
        if global_freq[curr_w] < global_freq[prev_w]:
            time_sec = curr_w * (HYPERPERIOD / 1_000_000.0)
            if limit_seconds is not None and time_sec > limit_seconds:
                continue

            print(
                f'\nActuation at Time: {time_sec:.2f}s | Freq: {global_freq[prev_w] / 1e9:.3f}GHz -> {global_freq[curr_w] / 1e9:.3f}GHz'
            )

            act_errors = []
            act_accuracies = []

            for core_id in all_cores:
                # The prediction is logged in the window the actuation is performed
                pred = core_pred_utilization.get(core_id, {}).get(curr_w, 0.0)
                # The actual is extracted from the next clean hyperperiod
                actual = core_utilization.get(core_id, {}).get(curr_w + 1, 0.0)

                if pred > 0 or actual > 0:
                    diff = abs(actual - pred)
                    accuracy = 100.0 - (diff / actual * 100.0) if actual > 0 else 0.0

                    act_errors.append(diff)
                    act_accuracies.append(accuracy)

                    print(
                        f'  Core {core_id} -> Predicted: {pred:6.2f}% | Actual: {actual:6.2f}% | Abs Error: {diff:5.2f}% | Accuracy: {accuracy:6.2f}%'
                    )

            # Print the average across active cores
            if act_errors:
                avg_err = sum(act_errors) / len(act_errors)
                avg_acc = sum(act_accuracies) / len(act_accuracies)
                print(f'  >> AVERAGE  -> Abs Error: {avg_err:5.2f}% | Accuracy: {avg_acc:6.2f}%')

    print('----------------------------------\n')

    # --- Plotting ---
    fig, ax1 = plt.subplots(figsize=(4, 2.2))
    ax2 = ax1.twinx()

    time_scale_to_seconds = HYPERPERIOD / 1_000_000.0

    # CPU Frequency
    freq_time_seconds = []
    frequencies_ghz = []

    for w in freq_sorted_windows:
        t_sec = w * time_scale_to_seconds
        if limit_seconds is not None and t_sec > limit_seconds:
            break
        freq_time_seconds.append(t_sec)
        frequencies_ghz.append(global_freq[w] / 1_000_000_000.0)

    if freq_time_seconds:
        bar_width = time_scale_to_seconds * 0.9
        ax1.bar(
            freq_time_seconds,
            frequencies_ghz,
            width=bar_width,
            align='edge',
            color='#E0E0E0',  # very light gray — stays in background, no hatching
            edgecolor='none',
            label='Frequency',
        )

    # Actual and Predicted Utilization
    # Each core gets a unique (linestyle, linewidth, marker) combo so they are
    # distinguishable even when printed in pure black-and-white.
    #   Actual    → continuous line + filled marker
    #   Predicted → markers only (no line), hollow marker of the same shape
    core_styles = [
        # (linestyle, linewidth, marker, markersize)
        ('-', 1.2, 'o', 3.0),
        ('--', 1.2, 's', 3.0),
        ('-.', 1.2, '^', 3.5),
        (':', 1.5, 'D', 3.0),
    ]

    for idx, core_id in enumerate(all_cores):
        ls, lw, mk, ms = core_styles[idx % len(core_styles)]

        # Actual Utilization — solid line with filled markers
        timeline = core_utilization.get(core_id, {})
        sorted_windows = sorted(timeline.keys())
        time_seconds = []
        utilizations = []

        for w in sorted_windows:
            t_sec = w * time_scale_to_seconds
            if limit_seconds is not None and t_sec > limit_seconds:
                break
            time_seconds.append(t_sec)
            utilizations.append(timeline[w])

        ax2.plot(
            time_seconds,
            utilizations,
            marker=mk,
            markersize=ms,
            markerfacecolor='black',
            markeredgecolor='black',
            linewidth=lw,
            linestyle=ls,
            color='black',
            label=f'Core {core_id} Actual',
        )

        # Predicted Utilization — markers only, hollow, so they stand out
        # clearly against the lines without adding another overlapping line.
        pred_timeline = core_pred_utilization.get(core_id, {})
        pred_sorted_windows = sorted(pred_timeline.keys())
        pred_time_seconds = []
        pred_utilizations = []

        for w in pred_sorted_windows:
            t_sec = w * time_scale_to_seconds
            if limit_seconds is not None and t_sec > limit_seconds:
                break
            pred_time_seconds.append(t_sec)
            pred_utilizations.append(pred_timeline[w])

        if pred_time_seconds:
            ax2.plot(
                pred_time_seconds,
                pred_utilizations,
                marker=mk,
                markersize=ms + 0.5,
                markeredgewidth=0.9,
                markerfacecolor='white',  # hollow = predicted
                markeredgecolor='black',
                linestyle='None',  # no line — avoids doubling up on the actual line
                color='black',
                label=f'Core {core_id} Predicted',
            )

    # Migration Indicators
    top_line_y = 1.05
    trans = mtransforms.blended_transform_factory(ax1.transData, ax1.transAxes)

    x_min = 0
    x_max = limit_seconds if limit_seconds is not None else max(freq_time_seconds) if freq_time_seconds else 0

    # Draw horizontal dashed line
    ax1.plot(
        [x_min, x_max],
        [top_line_y, top_line_y],
        color='black',
        linestyle='--',
        linewidth=0.8,
        transform=trans,
        clip_on=False,
    )

    # Draw migration squares on the line
    for mig in migrations:
        if limit_seconds is not None and mig['time'] > limit_seconds:
            continue
        ax1.plot(mig['time'], top_line_y, marker='s', markersize=2, color='black', transform=trans, clip_on=False)

    # Formatting and labels
    ax1.set_xlabel('Time (seconds)', fontsize=8)

    ax1.set_ylabel('Frequency (GHz)', fontsize=8)
    ax1.tick_params(axis='y', labelsize=8)

    ax2.set_ylabel('Utilization (%)', fontsize=8)
    ax2.tick_params(axis='y', labelsize=8)

    # Restrict utilization axis to 150% max and draw faint gray line at 100% (changed from red)
    ax2.set_ylim(bottom=0, top=150)
    ax2.axhline(y=100, color='gray', alpha=0.5, linestyle='--', linewidth=0.5)

    ax1.tick_params(axis='x', labelsize=8)

    # lines_1, labels_1 = ax1.get_legend_handles_labels()
    # lines_2, labels_2 = ax2.get_legend_handles_labels()
    #
    # # Place legend outside below the plot, expanding into multiple columns
    # ax1.legend(
    #     lines_1 + lines_2, labels_1 + labels_2, fontsize=6, loc='upper center', bbox_to_anchor=(0.5, -0.25), ncol=3
    # )

    ax1.grid(True, linestyle='--', linewidth=0.5)

    if limit_seconds is not None:
        ax1.set_xlim(left=0, right=limit_seconds)

    # fig.tight_layout could sometimes squish the legend if we place it outside.
    # Using pad to ensure enough space.
    fig.tight_layout(pad=0.5)

    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, 'core_utilization.pdf')
    # bbox_inches='tight' guarantees the external legend and the top line won't get cut off
    plt.savefig(output_path, format='pdf', bbox_inches='tight')
    plt.close()


def main() -> None:
    parser = argparse.ArgumentParser(description='Plot normalized core utilization and migrations in grayscale.')
    parser.add_argument('dataset_path', help='Path to the source JSON dataset')
    parser.add_argument('output_dir', help='Directory to save the generated plot')
    parser.add_argument('--limit', type=float, default=None, help='Limit the plot strictly to this many seconds')

    args = parser.parse_args()

    plot_core_utilization(args.dataset_path, args.output_dir, args.limit)


if __name__ == '__main__':
    main()
