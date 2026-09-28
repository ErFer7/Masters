import argparse
import matplotlib.pyplot as plt
import numpy as np
import re
import os


# Function to convert mAh and Volts to Joules
def to_joules(mah, volts):
    return mah * volts * 3.6


# Function to extract data from a single taskset text file
def parse_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    # Regex to extract the numbers for Voltage and Consumption
    regular_match = re.search(
        r'Regular:.*?Voltage:\s*([\d.]+)\s*V.*?Consumption:\s*([\d.]+)\s*mAh', content, re.DOTALL | re.IGNORECASE
    )
    opt_match = re.search(
        r'Optimized:.*?Voltage:\s*([\d.]+)\s*V.*?Consumption:\s*([\d.]+)\s*mAh', content, re.DOTALL | re.IGNORECASE
    )

    if not regular_match or not opt_match:
        raise ValueError(f'Could not parse expected format in {filepath}')

    reg_v, reg_mah = float(regular_match.group(1)), float(regular_match.group(2))
    opt_v, opt_mah = float(opt_match.group(1)), float(opt_match.group(2))

    return to_joules(reg_mah, reg_v), to_joules(opt_mah, opt_v)


def main():
    # Set up command-line arguments
    parser = argparse.ArgumentParser(description='Plot energy consumption from taskset files.')
    parser.add_argument(
        '-i',
        '--inputs',
        nargs='+',
        required=True,
        help='Paths to the input text files (e.g., -i task1.txt task2.txt task3.txt)',
    )
    parser.add_argument(
        '-o', '--outdir', type=str, default='.', help='Directory to save the plot (default: current directory)'
    )
    args = parser.parse_args()

    # Create output directory if it doesn't exist
    if not os.path.exists(args.outdir):
        os.makedirs(args.outdir)

    rtos_data = []
    optimized_data = []
    labels = []

    # Parse each input file dynamically
    for idx, filepath in enumerate(args.inputs):
        try:
            reg_j, opt_j = parse_file(filepath)
            print(f'TS{idx + 1}: Original: {reg_j:.3f}, Optimized: {opt_j:.3f}, Change: {(opt_j - reg_j) / reg_j:.2%}')
            rtos_data.append(reg_j)
            optimized_data.append(opt_j)
            # Use TSX abbreviation to save horizontal space and match tables
            labels.append(f'TS{idx + 1}')
        except Exception as e:
            print(f'Error processing {filepath}: {e}')
            return

    # Super compact height (1.8 inches), standard IEEE width (3.5 inches)
    fig, ax = plt.subplots(figsize=(3.5, 1.8))

    y = np.arange(len(labels))
    height = 0.35  # Bar thickness adjusted for horizontal layout

    # Create horizontal bars (barh instead of bar)
    rects1 = ax.barh(y - height / 2, rtos_data, height, label='Baseline', color='#666666', edgecolor='black')
    rects2 = ax.barh(y + height / 2, optimized_data, height, label='OL-EAMRTS', color='#CCCCCC', edgecolor='black')

    # Formatting - Increased font sizes and made labels bold for extreme readability
    ax.set_xlabel('Energy (Joules)', fontsize=10, fontweight='bold')
    ax.set_yticks(y)
    ax.set_yticklabels(labels, fontsize=10, fontweight='bold')
    ax.tick_params(axis='x', labelsize=11)

    # Invert the Y-axis so TS1 starts at the top and reads downward naturally
    ax.invert_yaxis()

    # Add vertical grid lines instead of horizontal
    ax.set_axisbelow(True)
    ax.xaxis.grid(True, color='#E5E5E5', linestyle='-', linewidth=0.5)

    # Legend - Pushed strictly ABOVE the plot with 2 columns to prevent overlap
    ax.legend(loc='lower center', bbox_to_anchor=(0.5, 1.05), ncol=2, frameon=False, fontsize=10)

    # Remove top and right spines
    ax.spines['top'].set_visible(False)
    ax.spines['right'].set_visible(False)
    ax.spines['left'].set_color('#333333')
    ax.spines['bottom'].set_color('#333333')

    # Minimal padding
    fig.tight_layout(pad=0.2)

    # Save the figure in both PNG and PDF with transparent=True
    output_pdf = os.path.join(args.outdir, 'energy_comparison.pdf')
    # bbox_inches='tight' guarantees the external legend isn't clipped out of the PDF bounds
    plt.savefig(output_pdf, bbox_inches='tight', transparent=True)
    print(f'Plots successfully saved to:\n- {output_pdf}')


if __name__ == '__main__':
    main()
