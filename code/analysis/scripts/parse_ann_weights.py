import sys
import os
import re


def parse_fann_network(file_path):
    if not os.path.exists(file_path):
        print(f"Error: File '{file_path}' not found.")
        sys.exit(1)

    with open(file_path, 'r') as f:
        content = f.read()

    # Dictionary to hold scalar configuration values
    config = {}
    keys = [
        'num_layers',
        'learning_rate',
        'connection_rate',
        'network_type',
        'learning_momentum',
        'train_error_function',
        'train_stop_function',
        'bit_fail_limit',
    ]

    # Extract configs regardless of line number
    for key in keys:
        match = re.search(rf'^{key}=([^\n]+)', content, re.MULTILINE)
        if match:
            config[key] = match.group(1).strip()

    # Extract layer sizes
    layer_sizes_match = re.search(r'^layer_sizes=([^\n]+)', content, re.MULTILINE)
    layer_sizes = layer_sizes_match.group(1).strip().split() if layer_sizes_match else []

    # Extract neurons and connections, handling the extended FANN 2.1 definitions
    neurons_match = re.search(r'^neurons.*?=(.*)', content, re.MULTILINE)
    neurons_str = neurons_match.group(1).strip() if neurons_match else ''

    connections_match = re.search(r'^connections.*?=(.*)', content, re.MULTILINE)
    connections_str = connections_match.group(1).strip() if connections_match else ''

    return config, layer_sizes, neurons_str, connections_str


def generate_header(config, layer_sizes, neurons_str, connections_str):
    output = []

    # Defines
    output.append(f'    static const unsigned int NUM_LAYERS_CONFIG           = {config.get("num_layers", "0")};')
    output.append(f'    static constexpr float    LEARNING_RATE_CONFIG        = {config.get("learning_rate", "0.0")};')
    output.append(
        f'    static constexpr float    CONNECTION_RATE_CONFIG      = {config.get("connection_rate", "0.0")};'
    )
    output.append(f'    static const unsigned int NETWORK_TYPE_CONFIG         = {config.get("network_type", "0")};')
    output.append(
        f'    static constexpr float    LEARNING_MOMENTUM_CONFIG    = {config.get("learning_momentum", "0.0")};'
    )
    output.append(
        '    static const unsigned int TRAINING_ALGORITHM_CONFIG   = 0; // 0 = INCREMENTAL - for online training support'
    )
    output.append(
        f'    static const unsigned int TRAIN_ERROR_FUNCTION_CONFIG = {config.get("train_error_function", "0")};'
    )
    output.append(
        f'    static const unsigned int TRAIN_STOP_FUNCTION_CONFIG  = {config.get("train_stop_function", "0")};'
    )
    output.append(f'    static constexpr double   BIT_FAIL_LIMIT_CONFIG       = {config.get("bit_fail_limit", "0.0")};')

    # Layers Array
    sizes_str = ', '.join(layer_sizes)
    output.append(f'\n        const unsigned int layers_sizes[] = {{ {sizes_str} }};')

    # Neurons Array
    output.append('\n        const float neurons_config[][3] = {')
    neuron_tuples = re.findall(r'\(([^)]+)\)', neurons_str)
    neuron_lines = [f'            {{{t}}}' for t in neuron_tuples]
    output.append(',\n'.join(neuron_lines))
    output.append('        };')

    # Connections Array
    output.append('\n        const float weights_config[][3] = {')
    conn_tuples = re.findall(r'\(([^)]+)\)', connections_str)
    conn_lines = [f'            {{{t}}}' for t in conn_tuples]
    output.append(',\n'.join(conn_lines))
    output.append('        };')

    return '\n'.join(output)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Please provide the name of the .net file to be converted.')
        sys.exit(1)

    file_name = sys.argv[1]

    # Auto-append .net if the user forgets it
    if not file_name.endswith('.net'):
        file_name += '.net'

    config, layer_sizes, neurons, connections = parse_fann_network(file_name)
    result = generate_header(config, layer_sizes, neurons, connections)

    print(result)
