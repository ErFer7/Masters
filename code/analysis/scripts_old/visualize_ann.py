import re
import sys
import plotly.graph_objects as go
import numpy as np


def visualize_fann_network(file_path, prune_percentile=25):
    # 1. Read the FANN file
    with open(file_path, 'r') as file:
        content = file.read()

    # 2. Extract layer sizes
    layer_sizes_match = re.search(r'layer_sizes=(.*)', content)
    if not layer_sizes_match:
        print('Could not find layer_sizes in the file.')
        return

    layer_sizes = [int(x) for x in layer_sizes_match.group(1).strip().split()]

    # 3. Extract neurons to find their input counts
    neuron_matches = re.findall(r'\(\s*(\d+),\s*\d+,\s*[-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?\)', content)

    # 4. Extract explicit connections
    connection_matches = re.findall(r'\(\s*(\d+),\s*([-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)\)', content)

    # 5. Build Node Tracking and Coordinates
    node_x = []
    node_y = []
    node_text = []

    neuron_coords = {}
    hidden_neurons = []  # Track IDs of non-bias hidden neurons for pruning

    current_neuron_id = 0
    max_layer_size = max(layer_sizes)

    for layer_idx, num_neurons in enumerate(layer_sizes):
        y_offset = (max_layer_size - num_neurons) / 2.0

        for n_idx in range(num_neurons):
            is_bias = n_idx == num_neurons - 1
            is_hidden = 0 < layer_idx < len(layer_sizes) - 1

            x = layer_idx * 1.2
            y = max_layer_size - (y_offset + n_idx)

            node_x.append(x)
            node_y.append(y)

            label = (
                f'L{layer_idx} Bias (ID: {current_neuron_id})'
                if is_bias
                else f'L{layer_idx} N{n_idx} (ID: {current_neuron_id})'
            )
            node_text.append(label)
            neuron_coords[current_neuron_id] = (x, y, is_bias, is_hidden)

            if is_hidden and not is_bias:
                hidden_neurons.append(current_neuron_id)

            current_neuron_id += 1

    # 6. Map Edges and Calculate Neuron Relevance
    weights = [float(w) for _, w in connection_matches]
    max_abs_weight = max(abs(w) for w in weights) if weights else 1.0

    traces = []
    mid_x = []
    mid_y = []
    hover_texts = []

    conn_idx = 0
    current_target_id = 0

    # Initialize relevance dictionary for all neurons
    neuron_relevance = {i: 0.0 for i in range(current_neuron_id)}

    for num_inputs_str in neuron_matches:
        num_inputs = int(num_inputs_str)

        for _ in range(num_inputs):
            if conn_idx < len(connection_matches):
                source_id_str, weight_str = connection_matches[conn_idx]
                source_id = int(source_id_str)
                weight_val = float(weight_str)

                # Accumulate L1 relevance (sum of absolute weights)
                abs_w = abs(weight_val)
                neuron_relevance[source_id] += abs_w
                neuron_relevance[current_target_id] += abs_w

                if source_id in neuron_coords and current_target_id in neuron_coords:
                    x0, y0, _, _ = neuron_coords[source_id]
                    x1, y1, _, _ = neuron_coords[current_target_id]

                    alpha = max(0.05, abs_w / max_abs_weight)

                    if weight_val >= 0:
                        edge_color = f'rgba(44, 160, 44, {alpha})'
                    else:
                        edge_color = f'rgba(214, 39, 40, {alpha})'

                    traces.append(
                        go.Scatter(
                            x=[x0, x1, None],
                            y=[y0, y1, None],
                            line=dict(width=1.5, color=edge_color),
                            hoverinfo='none',
                            mode='lines',
                            showlegend=False,
                        )
                    )

                    mid_x.append((x0 + x1) / 2)
                    mid_y.append((y0 + y1) / 2)
                    hover_texts.append(f'Weight: {weight_val:.6f}')

                conn_idx += 1
        current_target_id += 1

    # 7. Identify Neurons to Prune
    # Calculate threshold based on the specified percentile among hidden neurons
    hidden_relevances = [neuron_relevance[nid] for nid in hidden_neurons]

    prune_threshold = 0
    if hidden_relevances:
        prune_threshold = np.percentile(hidden_relevances, prune_percentile)
        neurons_to_prune = [nid for nid in hidden_neurons if neuron_relevance[nid] <= prune_threshold]
    else:
        neurons_to_prune = []

    # Print relevance metrics to terminal
    print('\n--- Neuron Relevance & Pruning Analysis ---')
    print(f'Pruning Threshold ({prune_percentile}th percentile): {prune_threshold:.4f}')
    print(f'{"Neuron ID":<12} | {"Layer":<10} | {"Relevance Score":<18} | {"Status"}')
    print('-' * 65)

    for nid, (x, y, is_bias, is_hidden) in neuron_coords.items():
        if is_hidden and not is_bias:
            layer_str = node_text[nid].split()[0]
            score = neuron_relevance[nid]
            status = 'PRUNE (Candidate)' if nid in neurons_to_prune else 'KEEP'
            print(f'{nid:<12} | {layer_str:<10} | {score:<18.4f} | {status}')

    # 8. Set Node Colors based on Pruning Status
    node_colors = []
    for nid in range(current_neuron_id):
        _, _, is_bias, _ = neuron_coords[nid]
        if nid in neurons_to_prune:
            node_colors.append('#d62728')  # Red for pruned
        elif is_bias:
            node_colors.append('#ff7f0e')  # Orange for bias
        else:
            node_colors.append('#1f78b4')  # Blue for regular nodes

    # 9. Create Plotly Traces for Interactivity and Nodes
    traces.append(
        go.Scatter(
            x=mid_x,
            y=mid_y,
            mode='markers',
            marker=dict(size=8, color='rgba(0,0,0,0)'),
            text=hover_texts,
            hoverinfo='text',
            showlegend=False,
        )
    )

    traces.append(
        go.Scatter(
            x=node_x,
            y=node_y,
            mode='markers+text',
            textposition='bottom center',
            hoverinfo='text',
            marker=dict(
                showscale=False,
                color=node_colors,
                size=16,
                line_width=1.5,
                line=dict(color='#000'),
            ),
            text=node_text,
            showlegend=False,
        )
    )

    # 10. Render Figure
    fig = go.Figure(
        data=traces,
        layout=go.Layout(
            title=dict(text='FANN Architecture (Red nodes = Pruning Candidates)', font=dict(size=16)),
            hovermode='closest',
            margin=dict(b=20, l=5, r=5, t=40),
            plot_bgcolor='white',
            xaxis=dict(showgrid=False, zeroline=False, showticklabels=False),
            yaxis=dict(showgrid=False, zeroline=False, showticklabels=False),
        ),
    )

    fig.show()


if __name__ == '__main__':
    if len(sys.argv) > 1:
        # You can adjust the pruning percentile (default is bottom 25%)
        visualize_fann_network(sys.argv[1], prune_percentile=25)
    else:
        print('Please provide the path to the .net file.')
