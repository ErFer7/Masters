"""
PMU Counter Metrics per Job Execution.

For each job execution (consecutive samples where the same non-idle thread is
running), computes two metrics per PMU counter:

  rate  = last_sample - first_sample          (total activity)
  ss    = sum of squared consecutive deltas   (burstiness)

Generates one normalized plot per counter showing both metrics against
utilization, mirroring the style of plot_normalized_growth().
"""

from sys import argv
from json import load
from os.path import join

import numpy as np
import pandas as pd
import matplotlib

matplotlib.use('QtAgg')
import matplotlib.pyplot as plt

from definitions import (
    TASKSET_1,
    DataOrigin,
    Dataset,
    SystemEvent,
    PMUEvent,
    Task,
    L2CachePMUEvent,
    PMU_LABELS,
    L2_PMU_LABELS,
)

# ── Configuration ──────────────────────────────────────────────────────────────
SELECTED_TASKSET = TASKSET_1
FILTER_TASK = None
FILTER_ORIGIN = None
INCLUDE_IDLE = True
# ──────────────────────────────────────────────────────────────────────────────


def get_readable_label(label_name: str) -> str:
    if label_name in PMU_LABELS:
        return PMU_LABELS[label_name]
    if label_name.startswith('IR_'):
        base_event = label_name[3:]
        if base_event in L2_PMU_LABELS:
            return f'{L2_PMU_LABELS[base_event]} (per IR)'
    if label_name in L2_PMU_LABELS:
        return L2_PMU_LABELS[label_name]
    return label_name


# ── Core computation ───────────────────────────────────────────────────────────


def compute_job_metrics(dataset: Dataset) -> pd.DataFrame:
    """
    Walk the time-series sample by sample, grouping consecutive samples where
    the same non-idle thread is running into a single job execution.

    For each job execution segment [i, j):
      - rate  = raw[j-1] - raw[i]          one integer per counter
      - ss    = Σ (raw[k] - raw[k-1])²     burstiness, also one integer

    The idle base offset cancels out in `rate` (last - first).  For `ss` we
    work on raw deltas; any constant idle contribution per sample cancels in
    the differences.

    Returns one DataFrame row per job execution.
    """
    excluded = {SystemEvent.RUNNING_THREAD, SystemEvent.JOB_UTILIZATION}
    rows = []

    for data_origin, data in dataset.data.items():
        if data_origin == DataOrigin.GLOBAL:
            continue

        # ── Locate anchor series ───────────────────────────────────────────
        running_threads_md = None
        utilization_md = None
        for md in data:
            if md.event == SystemEvent.RUNNING_THREAD:
                running_threads_md = md
            elif md.event == SystemEvent.JOB_UTILIZATION:
                utilization_md = md

        if running_threads_md is None:
            continue

        n = len(running_threads_md.data)
        idle = running_threads_md.data[0][1]

        # ── Raw cumulative PMU values (not rate-converted) ─────────────────
        pmu_raw: dict = {}
        for md in data + dataset.data.get(DataOrigin.GLOBAL, []):
            if md.event in excluded:
                continue
            if isinstance(md.event, (PMUEvent, L2CachePMUEvent)):
                pmu_raw[md.event] = [v for _, v in md.data]

        util_vals = [v for _, v in utilization_md.data] if utilization_md else [0.0] * n

        # ── Thread → task mapping (first-seen order, same as original) ─────
        thread_task_map: dict = {}
        task_index = 0

        # ── Segment walk ───────────────────────────────────────────────────
        i = 0
        global_job_idx = 0

        while i < n:
            thread = running_threads_md.data[i][1]

            # --- NEW IDLE FILTER LOGIC ---
            if not INCLUDE_IDLE and thread == idle:
                i += 1
                continue

            # Register thread → task on first encounter
            if thread not in thread_task_map:
                if thread == idle:
                    # Assign a string label and do not consume a task_index
                    thread_task_map[thread] = 'Idle'
                else:
                    try:
                        task = SELECTED_TASKSET.origin_map[data_origin][task_index].task
                    except (KeyError, IndexError):
                        task = None
                    thread_task_map[thread] = task
                    task_index += 1

            task = thread_task_map[thread]

            # Advance j to the end of this thread's consecutive run
            j = i + 1
            while j < n and running_threads_md.data[j][1] == thread:
                j += 1

            # Need ≥2 samples to compute deltas
            if j - i >= 2:
                # Utilization reported at job end
                util = util_vals[j - 1] / 1_000_000

                # Safely extract task name whether it's an Enum (real task) or String (Idle)
                task_name = task.value if hasattr(task, 'value') else (task if task else 'unknown')

                row: dict = {
                    'job_idx': global_job_idx,
                    'task': task_name,
                    'thread': thread,
                    'core': data_origin.value,
                    'timestamp': running_threads_md.data[j - 1][0],
                    'utilization': util,
                }

                for event, raw in pmu_raw.items():
                    segment = raw[i:j]

                    # Guard against counter overflow (wrap-around produces a
                    # large negative delta; clamp to 0)
                    deltas = [max(0, segment[k] - segment[k - 1]) for k in range(1, len(segment))]

                    n_d = len(deltas)
                    rate = max(0, segment[-1] - segment[0])
                    ss = sum(d * d for d in deltas)

                    mean_d = rate / n_d if n_d > 0 else 0.0
                    variance = (ss / n_d - mean_d**2) if n_d > 0 else 0.0

                    if n_d >= 2 and rate > 0:
                        q = (n_d * ss - rate * rate) / (rate * rate * (n_d - 1))
                        q = min(1.0, max(0.0, q))
                    else:
                        q = 0.0

                    row[f'{event.value}_rate'] = rate
                    row[f'{event.value}_var'] = variance
                    row[f'{event.value}_q'] = q

                rows.append(row)
                global_job_idx += 1

            i = j

    return pd.DataFrame(rows)


# ── Plotting ───────────────────────────────────────────────────────────────────


def _normalize(s: pd.Series) -> pd.Series:
    mn, mx = s.min(), s.max()
    if mn == mx:
        return pd.Series([0.5] * len(s), index=s.index)
    return (s - mn) / (mx - mn)


def plot_counter_metrics(df: pd.DataFrame, plot_directory: str) -> None:
    """
    One figure per PMU counter.  Each figure overlays three normalized series:

      Utilization  — black  solid    (prediction target)
      Rate         — blue   dashed   (total activity per job)
      SS Deltas    — red    dotted   (burstiness per job)

    Mirrors the style of plot_normalized_growth() in the original script.
    """
    # Dynamically detect X-axis thread boundaries (for labelling)
    cores = df['core'].values
    core_changes = [0]
    for idx in range(1, len(cores)):
        if cores[idx] != cores[idx - 1]:
            core_changes.append(idx)
    core_changes.append(len(cores))

    tick_positions = []
    tick_labels = []
    for k in range(len(core_changes) - 1):
        start = core_changes[k]
        end = core_changes[k + 1]
        tick_positions.append((start + end) // 2)
        core_val = str(cores[start]).split('_')[1] if '_' in str(cores[start]) else str(cores[start])
        tick_labels.append(f'T{core_val}')

    rate_cols = [c for c in df.columns if c.endswith('_rate')]

    for rate_col in rate_cols:
        event_key = rate_col[:-5]  # strip '_rate'
        var_col = f'{event_key}_var'
        q_col = f'{event_key}_q'

        if var_col not in df.columns:
            continue

        readable = get_readable_label(event_key)
        print(f'  Plotting: {readable}')

        x = df.index

        util_norm = _normalize(df['utilization'])
        rate_norm = _normalize(df[rate_col])
        var_norm = _normalize(df[var_col])
        q_vals = df[q_col] if q_col in df.columns else None

        fig, ax = plt.subplots(figsize=(1600 / 300, 900 / 300), dpi=300)

        ax.plot(x, util_norm, color='black', linewidth=1.5, label='Utilization')
        ax.plot(x, rate_norm, color='steelblue', linewidth=1.0, linestyle='--', label=r'Rate ($\Delta$)')
        ax.plot(x, var_norm, color='tomato', linewidth=1.0, linestyle=':', label=r'Var($\delta$)  [norm]')

        if q_vals is not None:
            ax.plot(
                x,
                q_vals,
                color='seagreen',
                linewidth=1.2,
                linestyle='-.',
                label=r'$q = \mathrm{CV}^2/(n{-}1)$  [0=uniform, 1=burst]',
            )

        ax.set_ylabel('Normalized value  /  q', fontsize=14)

        ax.set_xticks(tick_positions)
        ax.set_xticklabels(tick_labels, fontsize=16)
        ax.tick_params(axis='x', which='both', bottom=False, top=False)

        ax.legend(
            loc='upper center',
            bbox_to_anchor=(0.5, -0.15),
            ncol=4,
            fontsize=11,
            frameon=False,
        )

        ax.grid(True, linestyle='--', alpha=0.4)
        for spine in ['top', 'right']:
            ax.spines[spine].set_visible(False)

        safe_name = event_key.replace('/', '_').replace(' ', '_')
        plt.savefig(
            join(plot_directory, f'counter_metrics_{safe_name}.pdf'),
            bbox_inches='tight',
        )
        plt.close()


def plot_interactive_metrics(df: pd.DataFrame, plot_directory: str) -> None:
    """
    Creates an interactive HTML plot using true microseconds for the X-axis.
    Includes granular dropdown toggles for Rate, Variance, and Shape Q.
    """
    try:
        import plotly.graph_objects as go
        import plotly.colors as pcolors
    except ImportError:
        print('  [!] Plotly is not installed. Skipping interactive plot.')
        return

    print('  Generating interactive Plotly timeline...')
    fig = go.Figure()

    # 1. Pre-normalize globally and setup the true microsecond timeline
    df_norm = df.copy()

    # Sort to ensure timeline strictly moves forward
    df_norm = df_norm.sort_values(['core', 'timestamp']).reset_index(drop=True)
    df_norm['timestamp_us'] = df_norm['timestamp'] / 1000.0
    df_norm['utilization_norm'] = _normalize(df_norm['utilization'])

    rate_cols = [c for c in df_norm.columns if c.endswith('_rate')]
    pmu_events = [c[:-5] for c in rate_cols]

    # Update normalization to look for the new _var columns
    for c in rate_cols:
        df_norm[f'{c}_norm'] = _normalize(df_norm[c])
        var_col = f'{c[:-5]}_var'
        if var_col in df_norm.columns:
            df_norm[f'{var_col}_norm'] = _normalize(df_norm[var_col])

    # 2. Setup tracking for the granular dropdown menus
    trace_visibility_map = {'util': []}
    for event in pmu_events:
        trace_visibility_map[f'{event}_rate'] = []
        trace_visibility_map[f'{event}_var'] = []
        trace_visibility_map[f'{event}_q'] = []

    trace_idx = 0
    task_core_groups = df_norm.groupby(['task', 'core'])
    colors = pcolors.qualitative.Plotly + pcolors.qualitative.Alphabet

    # 3. Add traces per task/core combo
    for i, ((task, core), group_df) in enumerate(task_core_groups):
        task_df = group_df.sort_values('timestamp_us')

        color = colors[i % len(colors)]
        customdata = task_df[['task', 'core']].fillna('Unknown').values
        group_name = f'{task} ({core})'

        # Utilization (Solid Line + Markers)
        fig.add_trace(
            go.Scatter(
                x=task_df['timestamp_us'],
                y=task_df['utilization_norm'],
                mode='lines+markers',
                name=f'{group_name} Util',
                legendgroup=group_name,
                line=dict(color=color, width=2.5),
                marker=dict(size=4),
                customdata=customdata,
                hovertemplate='<b>Task:</b> %{customdata[0]}<br><b>Core:</b> %{customdata[1]}<br><b>Time (µs):</b> %{x:.1f}<br><b>Util:</b> %{y:.2f}<extra></extra>',
            )
        )
        trace_visibility_map['util'].append(trace_idx)
        trace_idx += 1

        # PMU Metrics for this task/core combo
        for event_key in pmu_events:
            readable = get_readable_label(event_key)

            # Rate (Dashed)
            fig.add_trace(
                go.Scatter(
                    x=task_df['timestamp_us'],
                    y=task_df[f'{event_key}_rate_norm'],
                    mode='lines+markers',
                    name=f'{readable} Rate',
                    legendgroup=group_name,
                    line=dict(color=color, dash='dash', width=1.5),
                    marker=dict(size=3),
                    visible=False,
                    customdata=customdata,
                    hovertemplate='<b>Task:</b> %{customdata[0]}<br><b>Core:</b> %{customdata[1]}<br><b>Rate:</b> %{y:.2f}<extra></extra>',
                )
            )
            trace_visibility_map[f'{event_key}_rate'].append(trace_idx)
            trace_idx += 1

            # Variance (Dotted)
            var_col = f'{event_key}_var'
            if var_col in df_norm.columns:
                fig.add_trace(
                    go.Scatter(
                        x=task_df['timestamp_us'],
                        y=task_df[f'{var_col}_norm'],
                        mode='lines+markers',
                        name=f'{readable} Var',
                        legendgroup=group_name,
                        line=dict(color=color, dash='dot', width=1.5),
                        marker=dict(size=3),
                        visible=False,
                        customdata=customdata,
                        hovertemplate='<b>Task:</b> %{customdata[0]}<br><b>Core:</b> %{customdata[1]}<br><b>Var:</b> %{y:.2f}<extra></extra>',
                    )
                )
                trace_visibility_map[f'{event_key}_var'].append(trace_idx)
                trace_idx += 1

            # Shape Q (Dash-Dot)
            q_col = f'{event_key}_q'
            if q_col in df_norm.columns:
                fig.add_trace(
                    go.Scatter(
                        x=task_df['timestamp_us'],
                        y=task_df[q_col],
                        mode='lines+markers',
                        name=f'{readable} Q',
                        legendgroup=group_name,
                        line=dict(color=color, dash='dashdot', width=1),
                        marker=dict(size=3),
                        visible=False,
                        customdata=customdata,
                        hovertemplate='<b>Task:</b> %{customdata[0]}<br><b>Core:</b> %{customdata[1]}<br><b>Q:</b> %{y:.2f}<extra></extra>',
                    )
                )
                trace_visibility_map[f'{event_key}_q'].append(trace_idx)
                trace_idx += 1

    # 4. Create Dropdown Menu Buttons
    num_traces = trace_idx
    buttons = []

    # Default View: Utilization Only
    vis_default = [False] * num_traces
    for idx in trace_visibility_map['util']:
        vis_default[idx] = True

    buttons.append(
        dict(
            label='-- Utilization Only --',
            method='update',
            args=[{'visible': vis_default}, {'title': 'Task Utilization Timeline'}],
        )
    )

    # View for each PMU event
    for event_key in pmu_events:
        readable = get_readable_label(event_key)

        # 1. Option: ALL Metrics for this Counter
        vis_all = list(vis_default)
        for idx in trace_visibility_map[f'{event_key}_rate']:
            vis_all[idx] = True
        for idx in trace_visibility_map[f'{event_key}_var']:
            vis_all[idx] = True
        for idx in trace_visibility_map[f'{event_key}_q']:
            vis_all[idx] = True

        buttons.append(
            dict(
                label=f'{readable} (All)',
                method='update',
                args=[{'visible': vis_all}, {'title': f'Utilization vs {readable} (All Metrics)'}],
            )
        )

        # 2. Option: Rate Only
        vis_rate = list(vis_default)
        for idx in trace_visibility_map[f'{event_key}_rate']:
            vis_rate[idx] = True
        buttons.append(
            dict(
                label=f' ├ Rate Only',
                method='update',
                args=[{'visible': vis_rate}, {'title': f'Utilization vs {readable} (Rate)'}],
            )
        )

        # 3. Option: Variance Only
        vis_var = list(vis_default)
        for idx in trace_visibility_map[f'{event_key}_var']:
            vis_var[idx] = True
        buttons.append(
            dict(
                label=f' ├ Variance Only',
                method='update',
                args=[{'visible': vis_var}, {'title': f'Utilization vs {readable} (Variance)'}],
            )
        )

        # 4. Option: Shape Q Only
        vis_q = list(vis_default)
        for idx in trace_visibility_map[f'{event_key}_q']:
            vis_q[idx] = True
        buttons.append(
            dict(
                label=f' └ Shape Q Only',
                method='update',
                args=[{'visible': vis_q}, {'title': f'Utilization vs {readable} (Shape Q)'}],
            )
        )

    # 5. Finalize Layout
    fig.update_layout(
        updatemenus=[
            dict(
                active=0,
                buttons=buttons,
                x=1.0,
                y=1.15,
                xanchor='right',
                yanchor='top',
                showactive=True,
            )
        ],
        title='Task Utilization Timeline',
        xaxis_title='Execution Timestamp (µs)',
        yaxis_title='Normalized Value / Shape Q',
        hovermode='closest',
        template='plotly_white',
        legend=dict(
            title='<b>Task & Core (Click to Toggle)</b>',
            orientation='v',
            yanchor='top',
            y=1.0,
            xanchor='left',
            x=1.02,
            tracegroupgap=2,
        ),
    )

    out_file = join(plot_directory, 'interactive_metrics.html')
    fig.write_html(out_file)
    print(f'  --> Saved interactive timeline plot to: {out_file}')


# ── Entry point ────────────────────────────────────────────────────────────────


def process(dataset_path: str, plot_directory: str) -> None:
    with open(dataset_path, 'r', encoding='utf-8') as f:
        dataset = Dataset(**load(f))

    print('Computing per-job rate and SS metrics...')
    df = compute_job_metrics(dataset)

    print(f'  Total job executions found : {len(df)}')
    if 'task' in df.columns:
        print(f'  Tasks found               : {df["task"].unique().tolist()}')

    if FILTER_TASK is not None:
        print(f'  Filtering for task        : {FILTER_TASK.value}')
        df = df[df['task'].str.contains(FILTER_TASK.value, case=False, na=False)]
        print(f'  Executions after filter   : {len(df)}')

    if FILTER_ORIGIN is not None:
        print(f'  Filtering for origin      : {FILTER_ORIGIN.value}')
        df = df[df['core'].str.contains(FILTER_ORIGIN.value, case=False, na=False)]
        print(f'  Executions after filter   : {len(df)}')

    df = df.sort_values(['core', 'timestamp']).reset_index(drop=True)

    print(f'\nDataFrame shape: {df.shape}')
    print(df[['task', 'core', 'utilization']].head())

    print('\nGenerating per-counter plots...')
    plot_counter_metrics(df, plot_directory)

    plot_interactive_metrics(df, plot_directory)
    print('Done.')


if __name__ == '__main__':
    process(argv[1], argv[2])
