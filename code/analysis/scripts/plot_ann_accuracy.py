import argparse
import re
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import os
from os.path import join


def setup_ieee_style():
    """Configures matplotlib for standard IEEE double-column paper formatting."""
    plt.rcParams.update(
        {
            'font.family': 'serif',
            'font.serif': ['Times New Roman', 'Times', 'serif'],
            'font.size': 9,
            'axes.labelsize': 9,
            'axes.titlesize': 9,
            'xtick.labelsize': 8,
            'ytick.labelsize': 8,
            'legend.fontsize': 8,
            'figure.figsize': (3.5, 2.5),
            'figure.dpi': 300,
            'lines.linewidth': 1.0,
            'grid.linewidth': 0.5,
            'grid.alpha': 0.5,
        }
    )


def parse_online_learning(file_path: str) -> dict:
    cores_data = {}
    current_core = 'System Adaptation'

    with open(file_path, 'r') as f:
        for line in f:
            # Detect which core is being processed
            match_core = re.search(r'Retraining Core:\s+(.+)', line, re.IGNORECASE)
            if match_core:
                current_core = match_core.group(1).strip()
                if current_core not in cores_data:
                    cores_data[current_core] = {'error': [], 'type': []}
                continue

            # Parses RUN and TRN passes, capturing the error value
            # Matches: (RUN|TRN): Output [i]: ..., expected: ..., error: ...
            match_rt = re.search(
                r'(RUN|TRN):\s*Output\s*\[\d+\]:\s*[0-9\.eE\-]+,\s*expected:\s*[0-9\.eE\-]+,\s*error:\s*([0-9\.eE\-]+)',
                line,
                re.IGNORECASE,
            )
            if match_rt:
                if current_core not in cores_data:
                    cores_data[current_core] = {'error': [], 'type': []}

                pass_type = match_rt.group(1).upper()
                error_val = float(match_rt.group(2))

                cores_data[current_core]['type'].append(pass_type)
                cores_data[current_core]['error'].append(error_val)

    return cores_data


def generate_plots(log_path: str, plot_dir: str) -> None:
    setup_ieee_style()
    cores_data = parse_online_learning(log_path)

    for core, (_, data) in enumerate(cores_data.items()):
        if len(data['error']) == 0:
            continue

        fig, ax = plt.subplots()

        steps = range(len(data['error']))
        errors = data['error']
        types = data['type']

        # Plot continuous solid line for the error trend
        ax.plot(steps, errors, label='Error', color='#d62728', linestyle='-', linewidth=0.5, zorder=2)

        # Find contiguous TRN regions to shade
        trn_regions = []
        start = None
        for i, t in enumerate(types):
            if t == 'TRN' and start is None:
                start = i
            elif t == 'RUN' and start is not None:
                trn_regions.append((start, i - 1))
                start = None

        # Catch if the log ends during a TRN phase
        if start is not None:
            trn_regions.append((start, len(types) - 1))

        # Add shaded backgrounds for TRN regions
        for s, e in trn_regions:
            ax.axvspan(s - 0.5, e + 0.5, color='gray', alpha=0.75, lw=0, zorder=1)

        # Append a custom legend handle for the shaded region
        handles, labels = ax.get_legend_handles_labels()
        trn_patch = mpatches.Patch(color='gray', alpha=0.3, label='Training')
        handles.append(trn_patch)

        ax.set_xlabel('Step')
        ax.set_ylabel('Prediction Error')

        ax.set_title(f'Core {core + 1}')

        ax.legend(handles=handles, loc='best')
        ax.grid(True)

        plt.tight_layout()

        os.makedirs(plot_dir, exist_ok=True)
        saving_path = join(plot_dir, f'online_learning_error_core_{core + 1}.pdf')

        plt.savefig(saving_path, bbox_inches='tight')
        print(f'Saved plot: {saving_path}')

        plt.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Plot online learning error separating RUN and TRN passes.')
    parser.add_argument('log_path', type=str, help='Path to the log file.')
    parser.add_argument('plot_dir', type=str, help='Path to the plot directory.')

    args = parser.parse_args()
    generate_plots(args.log_path, args.plot_dir)
