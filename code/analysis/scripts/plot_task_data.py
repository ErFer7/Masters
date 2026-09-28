import argparse
import os
import numpy as np
import matplotlib.pyplot as plt
from json import load

from definitions import TASKSET_1, TASKSETS, TaskRateDataset

SELECTED_TASKSET = TASKSET_1

def plot_segmented(
    title: str,
    out_dir: str,
    file_name: str,
    x_values_list: list[tuple],
    y_values_list: list[tuple],
    y_labels: tuple,
) -> None:
    """Plots data using threshold segmentation to handle gaps in timestamps."""
    fig, ax = plt.subplots(figsize=(16, 8), dpi=300)
    ax.set_title(title)
    ax.set_xlabel('Time (seconds)')
    ax.set_ylabel('Data Value')
    ax.ticklabel_format(style='plain', axis='x')

    for i in range(len(y_values_list)):
        x = np.array(x_values_list[i]) / 1000000
        y = np.array(y_values_list[i])

        # Slice both x and y simultaneously to safely remove the first index
        x = x[1:]
        y = y[1:]
        label = y_labels[i]

        if len(x) == 0:
            continue

        # Calculate step and threshold per task
        if len(x) > 1:
            step = np.median(np.diff(x))
            if step == 0:
                step = 0.001
            threshold = step * 1.5
        else:
            threshold = 0

        # Plot faint connecting line to establish color and legend entry
        line = ax.plot(x, y, linestyle=':', alpha=0.4, label=label)
        color = line[0].get_color()

        # Segmenting logic
        if len(x) > 1:
            segment_x = [x[0]]
            segment_y = [y[0]]

            for j in range(len(x) - 1):
                if x[j + 1] - x[j] <= threshold:
                    segment_x.append(x[j + 1])
                    segment_y.append(y[j + 1])
                else:
                    if len(segment_x) > 1:
                        ax.plot(segment_x, segment_y, linestyle='-', color=color, alpha=0.9, linewidth=0.8)
                    else:
                        ax.plot(segment_x, segment_y, marker='.', color=color, alpha=0.9, markersize=2)

                    segment_x = [x[j + 1]]
                    segment_y = [y[j + 1]]

            # Plot final segment
            if len(segment_x) > 1:
                ax.plot(segment_x, segment_y, linestyle='-', color=color, alpha=0.9, linewidth=0.8)
            else:
                ax.plot(segment_x, segment_y, marker='.', color=color, alpha=0.9, markersize=2)
        else:
            ax.plot(x, y, marker='.', color=color, alpha=0.9, markersize=2)

    # Styling grids based on the provided plot function
    ax.grid(True, which='major', axis='x', linestyle='-', alpha=0.6)
    ax.grid(True, which='minor', axis='x', linestyle='--', alpha=0.4)
    ax.grid(True, which='major', axis='y', linestyle='--', alpha=0.5)

    ax.legend()
    plt.tight_layout()

    output_path = os.path.join(out_dir, file_name)
    plt.savefig(output_path, format='pdf', bbox_inches='tight')
    plt.close(fig)


def plot_data(dataset: TaskRateDataset, out_dir: str):
    os.makedirs(out_dir, exist_ok=True)

    selected_taskset = TASKSETS[dataset.taskset]

    # 1. Pre-fetch INSTRUCTIONS_RETIRED data to use as the denominator
    instr_retired_data = {}
    try:
        for task, monitor_data_list in dataset.data.items():  # type: ignore
            # Filter by string name to avoid strict Enum import dependencies
            instr_monitor = list(filter(lambda m: 'INSTRUCTIONS_RETIRED' in str(m.event), monitor_data_list))[0]
            instr_retired_data[task] = np.array(list(zip(*instr_monitor.data))[1])
    except IndexError:
        # If INSTRUCTIONS_RETIRED is missing from the dataset, we skip derived plotting
        instr_retired_data = None

    for monitor_data in dataset.data[0]:
        event = monitor_data.event.value
        x_values, y_values = zip(*monitor_data.data)

        x_values_list = [x_values]
        y_values_list = [y_values]
        labels = [selected_taskset.tasks[0].task]

        for task, monitor_data_list in dataset.data.items():  # type: ignore
            if task == 0:
                continue

            task_monitor_data = list(filter(lambda m: m.event.value == event or m.event == event, monitor_data_list))[0]  # type: ignore
            t_x, t_y = zip(*task_monitor_data.data)
            x_values_list.append(t_x)
            y_values_list.append(t_y)
            labels.append(selected_taskset.tasks[task].task)

        # --- Standard Plot ---
        plot_segmented(
            title=f'{event}',
            out_dir=out_dir,
            file_name=f'plot_{event}.pdf',
            x_values_list=x_values_list,
            y_values_list=y_values_list,
            y_labels=tuple(labels),
        )

        # --- Derived Plot: L2 Event / INSTRUCTIONS_RETIRED ---
        if type(monitor_data.event).__name__ == 'L2CachePMUEvent' and instr_retired_data is not None:
            derived_y_values_list = []
            tasks_in_order = [0] + [t for t, _ in dataset.data.items() if t != 0]

            for i, task_id in enumerate(tasks_in_order):
                y_l2 = np.array(y_values_list[i])
                y_instr = instr_retired_data.get(task_id)

                if y_instr is not None:
                    derived_y = np.divide(y_l2, y_instr, out=np.zeros_like(y_l2, dtype=float), where=y_instr != 0)
                    derived_y_values_list.append(tuple(derived_y))
                else:
                    derived_y_values_list.append(y_values_list[i])

            plot_segmented(
                title=f'{event} per Instruction Retired',
                out_dir=out_dir,
                file_name=f'plot_{event}_per_instruction.pdf',
                x_values_list=x_values_list,
                y_values_list=derived_y_values_list,
                y_labels=tuple(labels),
            )

    # --- Custom Plot: PREDICTED_JOB_UTILIZATION vs JOB_UTILIZATION ---
    pred_event_name = 'PREDICTED_JOB_UTILIZATION'
    util_event_name = 'JOB_UTILIZATION'

    paired_data = []

    for task_id, monitor_data_list in dataset.data.items():  # type: ignore
        pred_monitors = list(filter(lambda m: pred_event_name in str(m.event), monitor_data_list))
        util_monitors = list(filter(lambda m: util_event_name in str(m.event), monitor_data_list))

        if pred_monitors and util_monitors:
            task_name = selected_taskset.tasks[task_id].task
            x_pred, y_pred = zip(*pred_monitors[0].data)
            x_util, y_util = zip(*util_monitors[0].data)
            paired_data.append(
                {'task_name': task_name, 'x_pred': x_pred, 'y_pred': y_pred, 'x_util': x_util, 'y_util': y_util}
            )

    if paired_data:
        fig, ax = plt.subplots(figsize=(16, 8), dpi=300)
        ax.set_title(f'{pred_event_name} vs {util_event_name}')
        ax.set_xlabel('Time (seconds)')
        ax.set_ylabel('Utilization Value')
        ax.ticklabel_format(style='plain', axis='x')

        # Use tab10 colormap to guarantee distinct colors for different tasks
        cmap = plt.get_cmap('tab10')

        for i, data in enumerate(paired_data):
            color = cmap(i % 10)
            task_name = data['task_name']

            # Inline function to duplicate segmentation logic but allow custom styling
            def plot_with_gaps(x_raw, y_raw, label, linestyle, alpha, linewidth):
                x = np.array(x_raw) / 1000000
                y = np.array(y_raw)
                x, y = x[1:], y[1:]

                if len(x) == 0:
                    return

                step = np.median(np.diff(x)) if len(x) > 1 else 0
                step = 0.001 if step == 0 else step
                threshold = step * 1.5 if len(x) > 1 else 0

                # Create an empty plot line to generate a clean legend handle
                ax.plot([], [], color=color, linestyle=linestyle, label=label, alpha=alpha, linewidth=linewidth)

                if len(x) > 1:
                    segment_x, segment_y = [x[0]], [y[0]]
                    for j in range(len(x) - 1):
                        if x[j + 1] - x[j] <= threshold:
                            segment_x.append(x[j + 1])
                            segment_y.append(y[j + 1])
                        else:
                            if len(segment_x) > 1:
                                ax.plot(
                                    segment_x,
                                    segment_y,
                                    linestyle=linestyle,
                                    color=color,
                                    alpha=alpha,
                                    linewidth=linewidth,
                                )
                            else:
                                ax.plot(segment_x, segment_y, marker='.', color=color, alpha=alpha, markersize=2)
                            segment_x, segment_y = [x[j + 1]], [y[j + 1]]

                    if len(segment_x) > 1:
                        ax.plot(
                            segment_x, segment_y, linestyle=linestyle, color=color, alpha=alpha, linewidth=linewidth
                        )
                    else:
                        ax.plot(segment_x, segment_y, marker='.', color=color, alpha=alpha, markersize=2)
                else:
                    ax.plot(x, y, marker='.', color=color, alpha=alpha, markersize=2)

            # Actual Utilization -> Solid line, slightly thicker
            plot_with_gaps(
                data['x_util'], data['y_util'], f'{task_name} (Actual)', linestyle='-', alpha=0.9, linewidth=1.5
            )
            # Predicted Utilization -> Dashed line, slightly thinner
            plot_with_gaps(
                data['x_pred'], data['y_pred'], f'{task_name} (Predicted)', linestyle='--', alpha=0.8, linewidth=1.0
            )

        # Re-apply standard grids
        ax.grid(True, which='major', axis='x', linestyle='-', alpha=0.6)
        ax.grid(True, which='minor', axis='x', linestyle='--', alpha=0.4)
        ax.grid(True, which='major', axis='y', linestyle='--', alpha=0.5)

        ax.legend()
        plt.tight_layout()

        output_path = os.path.join(out_dir, 'plot_predicted_vs_actual_utilization.pdf')
        plt.savefig(output_path, format='pdf', bbox_inches='tight')
        plt.close(fig)


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
        dataset = TaskRateDataset(**load(file))

    plot_data(dataset, args.outdir)
