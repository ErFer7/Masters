import itertools
import subprocess
import re
import time
import os
import multiprocessing
from concurrent.futures import ProcessPoolExecutor, as_completed

# ================= Configuration =================
CPP_BINARY = '../ann/train_gridsearch'

TRAIN_DATA = '../data/datasets/ann/taskset-1-reworked.data'
TEST_DATA = '../data/datasets/ann/taskset-2-reworked.data'
OUT_MODEL = './models/gridsearch_model.net'
CORE_FILES = [
    '../data/datasets/ann/taskset-2-reworked_CORE_1.data',
    '../data/datasets/ann/taskset-2-reworked_CORE_2.data',
    '../data/datasets/ann/taskset-2-reworked_CORE_3.data',
]

# Hyperparameter search space
EPOCHS = [1, 5, 10, 100, 1000, 5000]
OFFLINE_LR = [0.01, 0.05, 0.1, 0.2, 0.35, 0.5]
ONLINE_LR = [0.5, 0.7, 1.0, 1.25, 1.5, 1.75, 2]
HIDDEN_LAYERS = [0, 1, 2, 3]
NEURONS = [1, 2, 3, 4]

TARGET_METRIC = 'real_accuracy'
# =================================================


def build_grid():
    configs = []
    for e in EPOCHS:
        for off_lr in OFFLINE_LR:
            for on_lr in ONLINE_LR:
                for num_layers in HIDDEN_LAYERS:
                    if num_layers == 0:
                        configs.append({'epochs': e, 'off_lr': off_lr, 'on_lr': on_lr, 'layers': []})
                    else:
                        for combo in itertools.product(NEURONS, repeat=num_layers):
                            configs.append({'epochs': e, 'off_lr': off_lr, 'on_lr': on_lr, 'layers': list(combo)})
    return configs


def evaluate_config(config):
    """Worker function to execute a single configuration."""
    unique_model_name = f'temp_model_{os.getpid()}.net'

    cmd = [CPP_BINARY, str(config['epochs']), str(config['off_lr']), str(config['on_lr']), str(len(config['layers']))]
    cmd.extend([str(n) for n in config['layers']])
    cmd.extend([TRAIN_DATA, TEST_DATA, unique_model_name])
    cmd.extend(CORE_FILES)

    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=False)

        match_pattern = r'Accuracy after online adaptation \(Core \d+\): ([0-9.]+), error count: (\d+), retrains required: (\d+), Real accuracy: ([0-9.]+)'
        metrics = re.findall(match_pattern, result.stdout)

        if metrics:
            if TARGET_METRIC == 'online_accuracy':
                vals = [float(m[0]) for m in metrics]
            elif TARGET_METRIC == 'error_count':
                vals = [float(m[1]) for m in metrics]
            elif TARGET_METRIC == 'retrains_required':
                vals = [float(m[2]) for m in metrics]
            elif TARGET_METRIC == 'real_accuracy':
                vals = [float(m[3]) for m in metrics]
            else:
                return config, None, f'Invalid TARGET_METRIC selected: {TARGET_METRIC}'

            avg_val = sum(vals) / len(vals)

            # Sanity check (only applies if we are measuring an accuracy percentage)
            if 'accuracy' in TARGET_METRIC and not (0.0 <= avg_val <= 1.0):
                return config, None, f'Out of bounds accuracy calculated: {avg_val}'

            return config, avg_val, None
        else:
            error_msg = result.stderr if result.stderr else result.stdout
            return config, None, error_msg
    except Exception as e:
        return config, None, str(e)
    finally:
        if os.path.exists(unique_model_name):
            try:
                os.remove(unique_model_name)
            except OSError:
                pass


def main():
    configs = build_grid()
    total_runs = len(configs)
    print(f'Total configurations to test: {total_runs}')
    print(f'Optimizing for: {TARGET_METRIC.upper()}')

    # Determine whether we are trying to maximize or minimize the score
    is_minimizing = TARGET_METRIC in ['error_count', 'retrains_required']
    best_score = float('inf') if is_minimizing else -1.0

    best_config = None
    runs_completed = 0
    start_time = time.time()

    max_workers = max(1, os.cpu_count() - 1)
    print(f'Starting parallel execution with {max_workers} workers...')

    with ProcessPoolExecutor(max_workers=max_workers) as executor:
        futures = {executor.submit(evaluate_config, config): config for config in configs}

        for future in as_completed(futures):
            runs_completed += 1
            config, score, error_msg = future.result()

            if score is not None:
                is_new_best = (score < best_score) if is_minimizing else (score > best_score)

                if is_new_best:
                    best_score = score
                    best_config = config
                    print(
                        f'\n[NEW BEST] {TARGET_METRIC}: {score:.5f} | Epochs: {config["epochs"]} | Off-LR: {config["off_lr"]} | On-LR: {config["on_lr"]} | Layers: {config["layers"]}'
                    )

            elif error_msg and runs_completed == 1:
                print(f'\n[DIAGNOSTIC FIRST FAILURE] The C++ binary failed. Output:\n{error_msg}')

            elapsed = time.time() - start_time
            rate = runs_completed / elapsed if elapsed > 0 else 0

            if runs_completed % 50 == 0:
                print(
                    f'Progress: {runs_completed}/{total_runs} ({(runs_completed / total_runs) * 100:.1f}%) - {rate:.1f} runs/sec - last {TARGET_METRIC}: {score:.5f}'
                )

    print('\n' + '=' * 40)
    print('GRID SEARCH COMPLETE')
    print('=' * 40)
    if best_config:
        print(f'Best {TARGET_METRIC} : {best_score:.5f}')
        print(f'Best Epochs        : {best_config["epochs"]}')
        print(f'Best Offline LR    : {best_config["off_lr"]}')
        print(f'Best Online LR     : {best_config["on_lr"]}')
        print(f'Best Layer Topology: {best_config["layers"]}')
    else:
        print('No valid configurations completed successfully.')


if __name__ == '__main__':
    multiprocessing.set_start_method('spawn')
    main()
