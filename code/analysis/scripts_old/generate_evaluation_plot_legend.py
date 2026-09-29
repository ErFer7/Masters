import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
import matplotlib.patches as mpatches
import os


def generate_standalone_legend(output_dir: str):
    # Grayscale colors matching your main plot
    grayscale_colors = ['#000000', '#444444', '#888888', '#AAAAAA']
    all_cores = [1, 2, 3]  # Cores used in your task-sets

    legend_elements = []

    # 1. Frequency Bar
    legend_elements.append(mpatches.Patch(facecolor='#CCCCCC', alpha=0.6, label='Frequency'))

    # 2. Migrations Line
    legend_elements.append(Line2D([0], [0], color='black', linestyle='--', marker='s', markersize=2, label='Migration'))

    # 3. Core Actuals
    for idx, core_id in enumerate(all_cores):
        core_color = grayscale_colors[idx % len(grayscale_colors)]
        legend_elements.append(
            Line2D([0], [0], color=core_color, marker='o', markersize=2, linewidth=1, label=f'Core {core_id} Actual')
        )

    # 4. Core Predicteds
    for idx, core_id in enumerate(all_cores):
        core_color = grayscale_colors[idx % len(grayscale_colors)]
        legend_elements.append(
            Line2D(
                [0],
                [0],
                color=core_color,
                marker='x',
                markersize=4,
                linestyle='None',
                label=f'Core {core_id} Predicted',
            )
        )

    # Create a dummy figure (wide and very short)
    fig, ax = plt.subplots(figsize=(7, 0.5))
    ax.axis('off')  # Hide axes entirely

    # Render the legend spanning 4 columns
    legend = ax.legend(handles=legend_elements, loc='center', ncol=4, frameon=False, fontsize=8)

    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, 'shared_legend.pdf')

    # Save just the bounding box of the legend
    fig.savefig(output_path, format='pdf', bbox_inches='tight')
    plt.close(fig)
    print(f'Standalone legend saved to {output_path}')


if __name__ == '__main__':
    generate_standalone_legend('./figures')
