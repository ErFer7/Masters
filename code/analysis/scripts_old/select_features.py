"""
Data processing.
"""

from copy import deepcopy
from sys import argv
from json import load
from os.path import join

import numpy as np
from numpy import average
import pandas as pd
from pandas import DataFrame
import matplotlib

matplotlib.use('QtAgg')

import matplotlib.pyplot as plt
from termcolor import colored
from sklearn.linear_model import LassoCV
from info_gain import info_gain

from definitions import (
    TASKSET_1,
    WCET,
    DataOrigin,
    Dataset,
    MonitorData,
    SystemEvent,
    PMUEvent,
    Task,
    TaskData,
    L2CachePMUEvent,
    PMU_LABELS,
    L2_PMU_LABELS,
)

NORMALIZE_BY_IR = False
SELECTED_TASKSET = TASKSET_1
FILTER_TASK = Task.DISPARITY
FILTER_ORIGIN = None


def convert_cumulative_to_rate(dataset: Dataset) -> None:
    excluded_events = (SystemEvent.RUNNING_THREAD, SystemEvent.JOB_UTILIZATION)

    for _, data in dataset.data.items():
        for monitor_data in data:
            if monitor_data.event in excluded_events:
                continue

            timestamps, values = zip(*monitor_data.data)
            data_rate = [0] + [current - previous for previous, current in zip(values[:-1], values[1:])]

            print(f'{monitor_data.event.value} average rate: {average(data_rate):.3f}')

            if any(map(lambda value: value < 0, data_rate)):
                print(colored('Overflow detected!', 'red'))

            for i, data_rate_value in enumerate(data_rate):
                monitor_data.data[i] = (timestamps[i], data_rate_value)


def select_data_per_thread(dataset: Dataset) -> list[TaskData]:
    processed_threads = []

    for data_origin, data in dataset.data.items():
        if data_origin == DataOrigin.GLOBAL:
            continue

        threads = {}
        running_threads = None

        for monitor_data in data:
            if monitor_data.event == SystemEvent.RUNNING_THREAD:
                running_threads = monitor_data
                break

        idle = running_threads.data[0][1]  # type: ignore
        threads_index = 0
        thread_IR = {}

        # --- NEW LOGIC: Calculate Idle Averages for PMU Counters ---
        idle_averages = {}
        idle_counts = {}

        for monitor_data in data:
            event = monitor_data.event

            # We only want to calculate idle baselines for PMU counters
            if not isinstance(event, (PMUEvent,)):
                continue

            for i, (_, value) in enumerate(monitor_data.data):
                thread = running_threads.data[i][1]  # type: ignore
                if thread == idle:
                    if event not in idle_averages:
                        idle_averages[event] = 0.0
                        idle_counts[event] = 0
                    idle_averages[event] += value
                    idle_counts[event] += 1

        for event in idle_averages:
            if idle_counts[event] > 0:
                idle_averages[event] /= idle_counts[event]
        # -----------------------------------------------------------

        for monitor_data in data + dataset.data[DataOrigin.GLOBAL]:
            event = monitor_data.event

            if monitor_data.event == SystemEvent.RUNNING_THREAD:
                continue

            for i, (timestamp, value) in enumerate(monitor_data.data):
                thread = running_threads.data[i][1]  # type: ignore

                # --- NEW LOGIC: Subtract Idle Average ---
                if isinstance(event, (PMUEvent, L2CachePMUEvent)) and event in idle_averages:
                    # Enforce a minimum of 0.0 to avoid negative event rates
                    adjusted_value = max(0.0, value - idle_averages[event])
                else:
                    adjusted_value = value
                # ----------------------------------------

                if monitor_data.event == PMUEvent.INSTRUCTIONS_RETIRED:
                    if thread not in thread_IR:
                        thread_IR[thread] = []
                    thread_IR[thread].append(adjusted_value)

                if thread == idle:
                    continue

                if thread not in threads:
                    threads[thread] = {
                        'task': SELECTED_TASKSET.origin_map[data_origin][threads_index].task,
                        'address': thread,
                        'core': data_origin.value,
                        'data': {},
                    }
                    threads_index += 1

                if event not in threads[thread]['data']:
                    threads[thread]['data'][event] = MonitorData(**{'event': event, 'data': []})

                threads[thread]['data'][event].data.append((timestamp, adjusted_value))

        if NORMALIZE_BY_IR:
            for thread in threads:
                events = deepcopy(threads[thread]['data'])
                for e, _ in events.items():
                    print(e)
                    if isinstance(e, L2CachePMUEvent):
                        event = 'IR_' + e.value
                        if event not in threads[thread]['data']:
                            threads[thread]['data'][event] = MonitorData(**{'event': event, 'data': []})

                        for i, value in enumerate(threads[thread]['data'][e].data):
                            print(value[1])

                            # Safely compute ratio to prevent DivisionByZero
                            ir_val = float(thread_IR[thread][i])
                            normalized_val = float(value[1]) / ir_val if ir_val > 0 else 0.0

                            threads[thread]['data'][event].data.append((value[0], normalized_val))

        for _, thread in threads.items():
            processed_monitor_data = []

            for _, monitor_data in thread['data'].items():
                processed_monitor_data.append(monitor_data)

            thread['data'] = processed_monitor_data
            print(thread['task'])
            processed_threads.append(TaskData(**thread))

    return processed_threads


def flatten(task_data: list[TaskData]) -> DataFrame:
    events = [data.event.value for data in task_data[0].data]
    columns = ['task', 'address', 'core', 'timestamp'] + events
    flattened_data = []

    for task in task_data:
        task_name = task.task.value
        address = task.address
        core = task.core
        entry_count = len(task.data[0].data)

        for i in range(entry_count):
            row = [task_name, address, core, task.data[0].data[i][0]]

            for data in task.data:
                row.append(data.data[i][1])

            flattened_data.append(row)

    return DataFrame(flattened_data, columns=columns)  # type: ignore


def sortSecond(val):
    return val[1]


def info_gain_ratio_calc(Xs, Y, names):
    file = open('../data/results/info_gain_ratio.out', 'w')
    corr = []
    print(len(Xs), len(Y))
    for col in range(len(Xs)):
        if max(Xs[col]) > 0:
            # ig  = info_gain.info_gain(Xs[col], Y)
            # iv  = info_gain.intrinsic_value(Xs[col], Y)
            igr = info_gain.info_gain_ratio(Xs[col], Y)
            corr.append([names[col], igr])
            file.write(names[col] + ': igr= ' + str(igr) + '\n')
        else:
            file.write(names[col] + ': igr= 0\n')
    corr.sort(reverse=True, key=sortSecond)
    print(corr)
    file.write('=============== Ordered ================\n')
    rank_series = []
    name_series = []
    for i in range(len(corr)):
        if i < 11:
            rank_series.append(corr[i][1])
            name_series.append(corr[i][0])
        file.write(str(corr[i]) + '\n')
    file.close()

    rank_series = pd.Series(rank_series[:10])
    plt.figure(figsize=(6, 8))
    ax = rank_series.plot(kind='barh')
    ax.set_title('Information Gain Ratio')
    ax.set_xlabel('Gain Ratio')
    ax.set_ylabel('Feature')
    ax.set_yticklabels(name_series[:10])
    # add_value_labels(ax)
    plt.gca().invert_yaxis()
    plt.show()
    return corr


def discretize_quantile(data, bins_num: int = 10):
    return pd.qcut(data, q=bins_num, labels=False, duplicates='drop')


def normalize_data(dataframe: DataFrame) -> None:
    job_utilization_event = SystemEvent.JOB_UTILIZATION.value

    if job_utilization_event in dataframe.columns:
        dataframe[job_utilization_event] = dataframe[job_utilization_event] / 1000000

    columns_to_exclude = [
        'task',
        'address',
        'core',
        'timestamp',
        job_utilization_event,
        SystemEvent.CPU_CLOCK.value,
        SystemEvent.CPU_VOLTAGE.value,
    ]

    feature_columns = [column for column in dataframe.columns if column not in columns_to_exclude]

    for column in feature_columns:
        column_min = dataframe[column].min()
        column_max = dataframe[column].max()

        if column_min == column_max:
            print(f'Warning: Column {column} has constant value. Setting to 0.5.')
            dataframe[column] = 0.5
        else:
            dataframe[column] = (dataframe[column] - column_min) / (column_max - column_min)

    if job_utilization_event in dataframe.columns:
        utilization = dataframe[job_utilization_event]

        for column in feature_columns:
            dataframe[column] = dataframe[column] * utilization


def run_feature_selection(
    dataframe: pd.DataFrame, plot_directory: str, target_column: str = SystemEvent.JOB_UTILIZATION.value
) -> None:
    numeric_dataframe = dataframe.select_dtypes(include=['number'])

    columns_to_drop = (
        'timestamp',
        'address',
        SystemEvent.CPU_CLOCK.value,
        SystemEvent.CPU_VOLTAGE.value,
        target_column,
    )

    feature_columns = [column for column in numeric_dataframe.columns if column not in columns_to_drop]

    features = numeric_dataframe[feature_columns].values
    target = numeric_dataframe[target_column].values

    print('\n--- Pearson Correlation ---')
    target_correlation = numeric_dataframe[feature_columns].corrwith(
        other=numeric_dataframe[target_column], method='pearson'
    )

    target_correlation = target_correlation.abs().sort_values(ascending=False)
    print(target_correlation)

    plt.figure(figsize=(6, 8))
    target_correlation.head(10).sort_values().plot(kind='barh', title='Pearson Correlation Coefficient')
    plt.xlabel('Correlation')
    plt.show()

    print('\n--- LassoCV Embedded Method ---')
    lasso = LassoCV(cv=5, max_iter=1000000, tol=1e-3)
    lasso.fit(features, target)

    lasso_coefficients = pd.Series(np.abs(lasso.coef_), index=feature_columns).sort_values(ascending=False)
    lasso_coefficients = lasso_coefficients[lasso_coefficients > 0]

    l_min = lasso_coefficients.min()
    l_max = lasso_coefficients.max()
    if l_min != l_max:
        lasso_coefficients = (lasso_coefficients - l_min) / (l_max - l_min)
    else:
        lasso_coefficients[:] = 1.0

    print(lasso_coefficients.head(10))  # type: ignore
    print(f'Best alpha: {lasso.alpha_}')
    print(f'Score: {lasso.score(features, target)}')

    if not lasso_coefficients.empty:  # type: ignore
        plt.figure(figsize=(6, 8))
        lasso_coefficients.head(10).sort_values().plot(kind='barh', title='Lasso CV Feature Rank')  # type: ignore
        plt.xlabel('Coefficient Magnitude')
        plt.show()

    X_info = []
    print(len(features.T))
    for i, f in enumerate(features.T):
        X_info.append(discretize_quantile(f))
    Y_info = discretize_quantile(target, 2)

    gain = info_gain_ratio_calc(X_info, Y_info, feature_columns)

    # 1. Convert lists of (name, value) tuples into dictionaries
    pcc_dict = dict(target_correlation)
    gain_dict = dict(gain)
    lasso_dict = dict(lasso_coefficients)

    # 2. Get unique list of all feature names
    all_features = set(pcc_dict.keys()) | set(gain_dict.keys()) | set(lasso_dict.keys())

    # 3. Align, Sum, and Bundle
    data_bundled = []
    for name in all_features:
        p_val = pcc_dict.get(name, 0)
        g_val = gain_dict.get(name, 0)
        l_val = lasso_dict.get(name, 0)

        total_score = 0.25 * p_val + 0.75 * l_val
        data_bundled.append((total_score, p_val, g_val, l_val, name))

    top_data = sorted(data_bundled, key=lambda x: x[0], reverse=True)[:20]

    # 5. Unpack for plotting
    (_, sorted_pcc, sorted_gain, sorted_lasso, sorted_labels) = zip(*top_data)

    selected_pmu_fixed = []
    selected_pmu_programmable = []
    selected_pmu_l2 = []
    highly_correlated = set()

    pmu_fixed = (PMUEvent.CPU_CYCLES.value, PMUEvent.INSTRUCTIONS_RETIRED.value)
    pmu_programmable = tuple((
        event.value for event in PMUEvent if event != PMUEvent.CPU_CYCLES and event != PMUEvent.INSTRUCTIONS_RETIRED
    ))
    pmu_l2 = tuple((event.value for event in L2CachePMUEvent))

    for i, label in enumerate(sorted_labels):
        print('-------------------', label, '-------------------------')
        print(f'PCC: {sorted_pcc[i]:.2f}, LassoCV: {sorted_lasso[i]:.2f}')

        f_correlation = numeric_dataframe[[column for column in numeric_dataframe.columns if column != label]].corrwith(
            other=numeric_dataframe[label], method='pearson'
        )

        f_correlation = f_correlation.abs().sort_values(ascending=False)

        if label not in highly_correlated:
            if label in pmu_fixed and len(selected_pmu_fixed) < 2:
                selected_pmu_fixed.append(label)
            elif label in pmu_programmable and len(selected_pmu_programmable) < 2:
                selected_pmu_programmable.append(label)
            elif label in pmu_l2 and len(selected_pmu_l2) < 6:
                selected_pmu_l2.append(label)

        for col, val in f_correlation.items():
            print(f'Column: {col} | Correlation: {val}')

            if val < 0.85:
                break

            highly_correlated.add(col)

    print('Selected:')
    print('Fixed: ' + ', '.join(selected_pmu_fixed))
    print('Programmable: ' + ', '.join(selected_pmu_programmable))
    print('L2: ' + ', '.join(selected_pmu_l2))

    def get_readable_label(label_name):
        if label_name in PMU_LABELS:
            return PMU_LABELS[label_name]

        # Handle L2 events normalized by Instructions Retired
        if label_name.startswith('IR_'):
            base_event = label_name[3:]
            if base_event in L2_PMU_LABELS:
                return f'{L2_PMU_LABELS[base_event]} (per IR)'

        if label_name in L2_PMU_LABELS:
            return L2_PMU_LABELS[label_name]

        return label_name

    readable_labels = [get_readable_label(name) for name in sorted_labels]

    # 6. Setup Plot
    # Increased figsize width from 10 to 12 to accommodate the longer, readable labels
    fig, ax = plt.subplots(figsize=(3.5, 6))
    y = np.arange(len(readable_labels))
    height = 0.2

    ax.barh(y + height, sorted_pcc, height, label='PCC', color='black')
    # ax.barh(y, sorted_gain, height, label='Information Gain', color='dimgray')
    ax.barh(y - height, sorted_lasso, height, label='Lasso CV Rank', color='silver')

    # 7. Styling
    ax.set_yticks(y)
    ax.set_yticklabels(readable_labels)
    ax.invert_yaxis()  # Highest summed score at the top
    ax.set_xlim(0, 1.0)
    ax.set_xticks([0.0, 0.25, 0.50, 0.75, 1.0])

    # Grid and Spines
    ax.xaxis.grid(True, linestyle='-', alpha=0.7)
    for spine in ['top', 'right', 'bottom']:
        ax.spines[spine].set_visible(False)

    # Legend at the top
    ax.legend(loc='upper center', bbox_to_anchor=(0.5, 1.08), ncol=3, frameon=False)

    plt.savefig(join(plot_directory, 'utilization_correlation.pdf'), format='pdf', bbox_inches='tight')
    plt.show()

    # print('\n--- RFE (Linear Regression) ---')
    # estimator = LinearRegression()
    # rfe = RFE(estimator, n_features_to_select=6)
    # rfe.fit(features, target)
    #
    # rfe_ranking = pd.Series(rfe.ranking_, index=feature_columns).sort_values()
    # print(f'Selected Features (Support): {np.array(feature_columns)[rfe.support_]}')
    # print('\nFeature Ranking:')
    # print(rfe_ranking.head(10))


def plot_utilization(dataframe: pd.DataFrame) -> None:
    utilization_column = SystemEvent.JOB_UTILIZATION.value

    plt.figure(figsize=(12, 6))

    for (task_name, _, core), group_data in dataframe.groupby(['task', 'address', 'core']):  # type: ignore
        group_data = group_data.sort_values(by='timestamp')

        x = group_data['timestamp'].values
        y = group_data[utilization_column].values

        if len(x) == 0:
            continue

        line = plt.plot(x, y, linestyle=':', alpha=0.4, label=f'{task_name} (Core {core})')  # type: ignore
        color = line[0].get_color()

        if len(x) > 1:
            step = np.median(np.diff(x))  # type: ignore
            if step == 0:
                step = 0.001
            threshold = step * 1.5

            segment_x = [x[0]]
            segment_y = [y[0]]

            for i in range(len(x) - 1):
                if x[i + 1] - x[i] <= threshold:
                    segment_x.append(x[i + 1])
                    segment_y.append(y[i + 1])
                else:
                    if len(segment_x) > 1:
                        plt.plot(segment_x, segment_y, linestyle='-', color=color, alpha=0.9, linewidth=2)
                    else:
                        plt.plot(segment_x, segment_y, marker='o', color=color, alpha=0.9)

                    segment_x = [x[i + 1]]
                    segment_y = [y[i + 1]]

            if len(segment_x) > 1:
                plt.plot(segment_x, segment_y, linestyle='-', color=color, alpha=0.9, linewidth=2)
            else:
                plt.plot(segment_x, segment_y, marker='o', color=color, alpha=0.9)
        else:
            plt.plot(x, y, marker='o', color=color, alpha=0.9)  # type: ignore

    plt.title('Normalized Job Utilization Over Time')
    plt.xlabel('Timestamp')
    plt.ylabel('Normalized Job Utilization')
    plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
    plt.grid(True, linestyle='--', alpha=0.6)
    plt.tight_layout()
    plt.show()


def plot_normalized_growth(dataframe: pd.DataFrame, plot_directory: str) -> None:
    utilization_column = SystemEvent.JOB_UTILIZATION.value

    # Helper to map raw feature names to human-readable ones for the legend
    def get_readable_label(label_name):
        if label_name in PMU_LABELS:
            return PMU_LABELS[label_name]
        if str(label_name).startswith('IR_'):
            base_event = str(label_name)[3:]
            if base_event in L2_PMU_LABELS:
                return f'{L2_PMU_LABELS[base_event]} (per IR)'
        if label_name in L2_PMU_LABELS:
            return L2_PMU_LABELS[label_name]
        return str(label_name)

    # Sort by core (thread) and then timestamp to concatenate thread execution sequentially
    df_sorted = dataframe.sort_values(by=['core', 'timestamp']).reset_index(drop=True)

    # Identify numerical feature columns
    columns_to_exclude = ['task', 'address', 'core', 'timestamp', utilization_column]
    feature_columns = [
        col
        for col in df_sorted.columns
        if col not in columns_to_exclude and pd.api.types.is_numeric_dtype(df_sorted[col])
    ]

    # Dynamically calculate thread boundaries for the X-axis
    cores = df_sorted['core'].values
    core_changes = [0]
    for i in range(1, len(cores)):
        if cores[i] != cores[i - 1]:
            core_changes.append(i)
    core_changes.append(len(cores))

    tick_positions = []
    tick_labels = []
    for i in range(len(core_changes) - 1):
        start = core_changes[i]
        end = core_changes[i + 1]
        center = (start + end) // 2
        tick_positions.append(center)

        # Format the tick label as T<core> (e.g., T2, T3)
        core_val = cores[start].split('_')[1]
        tick_labels.append(f'T{core_val}')

    # Generate a plot for each feature against Thread Usage
    for feature in feature_columns:
        feature_label = get_readable_label(feature)
        print(f'Generating normalized growth plot for: {feature_label}...')

        plt.figure(figsize=(1600 / 300, 800 / 300), dpi=300)

        # Plot the feature (in gray) and the utilization (in black)
        plt.plot(df_sorted.index, df_sorted[feature], color='gray', label=feature_label)
        plt.plot(df_sorted.index, df_sorted[utilization_column], color='black', label='Thread Usage')

        plt.ylabel('Normalized growth', fontsize=14)

        # Place legend under the plot, split into 2 columns if needed
        plt.legend(loc='upper center', bbox_to_anchor=(0.5, -0.15), ncol=2, fontsize=12)

        # Apply the dynamic X-axis labels centered on each thread segment
        ax = plt.gca()
        ax.set_xticks(tick_positions)
        ax.set_xticklabels(tick_labels, fontsize=16)

        # Remove tick markers for a cleaner look
        ax.tick_params(axis='x', which='both', bottom=False, top=False)

        safe_feature_name = str(feature).replace('/', '_').replace(' ', '_')

        plt.savefig(join(plot_directory, f'normalized_growth_{safe_feature_name}.pdf'), bbox_inches='tight')
        plt.close()


def process(dataset_path: str, plot_directory: str) -> None:
    with open(dataset_path, 'r', encoding='utf-8') as file:
        dataset = Dataset(**load(file))

    print('Converting cumulative values to rate of change values')
    convert_cumulative_to_rate(dataset)

    print('Selecting data per thread')
    task_data = select_data_per_thread(dataset)

    print('Flattening the data')
    dataframe = flatten(task_data)

    print(f'Dataframe: {dataframe}')

    if FILTER_TASK is not None:
        print(f'Filtering for {FILTER_TASK} tasks')
        dataframe = dataframe[dataframe['task'].str.contains(FILTER_TASK.value, case=False, na=False)]

    if FILTER_ORIGIN is not None:
        print(f'Filtering tasks from {FILTER_ORIGIN}')
        dataframe = dataframe[dataframe['core'].str.contains(FILTER_ORIGIN.value, case=False, na=False)]  # type: ignore

    print('Normalizing')
    normalize_data(dataframe)  # type: ignore

    print(f'Normalized dataframe: {dataframe}')

    print('Plotting normalized utilization per task')
    plot_utilization(dataframe)  # type: ignore

    print('Plotting normalized growth')
    plot_normalized_growth(dataframe, plot_directory)  # type: ignore

    print('Running feature selection')
    run_feature_selection(dataframe, plot_directory)  # type: ignore


if __name__ == '__main__':
    process(argv[1], argv[2])
