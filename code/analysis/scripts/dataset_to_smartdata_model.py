import argparse
import json
import csv
import os
from collections import defaultdict

UNIT_MAPPING = {
    'CPU_CLOCK': {
        'si_quantity': 'frequency',
        'unit': 2224175396,
    },
    'JOB_UTILIZATION': {
        'si_quantity': 'time',
        'unit': 2224183588,
    },
    'default': {
        'si_quantity': 'count',
        'unit': 2224179492,
    },
}


def convert_to_smartdata(input_file: str, output_dir: str, epoch_s: float | None = None, limit_s: float | None = None):
    # Ensure output directory exists
    os.makedirs(output_dir, exist_ok=True)

    try:
        with open(input_file, 'r') as f:
            dataset = json.load(f)
    except FileNotFoundError:
        print(f"Error: Could not find the file '{input_file}'")
        return

    time_series = defaultdict(dict)
    events_seen = set()

    # Calculate time shift offset and limit threshold in microseconds
    time_offset_us = int(epoch_s * 1_000_000) if epoch_s is not None else 0
    limit_us = int(limit_s * 1_000_000) if limit_s is not None else None

    # ==========================================
    # 1. Extract and align data by timestamp
    # ==========================================
    for task_id, event_list in dataset.get('data', {}).items():
        numeric_task_id = int(task_id)

        # Pass 1: Detect Deadline Miss Anomaly for this specific task
        deadline_offset = 0
        for event_obj in event_list:
            if event_obj.get('event') == 'DEADLINE_MISSES':
                data_points = event_obj.get('data', [])
                if len(data_points) > 0 and data_points[0][1] == 1:
                    deadline_offset = 1
                break

        # Pass 2: Extract data and apply corrections/limits
        for event_obj in event_list:
            event_name = event_obj.get('event')
            events_seen.add(event_name)

            for timestamp, value in event_obj.get('data', []):
                # Check against the experiment time limit (in microseconds)
                if limit_us is not None and timestamp > limit_us:
                    continue

                shifted_timestamp = timestamp + time_offset_us

                # Apply the anomaly offset if this is a deadline miss
                if event_name == 'DEADLINE_MISSES':
                    value = max(0, value - deadline_offset)

                time_series[shifted_timestamp][event_name] = value

                # INJECT THE TASK ID!
                time_series[shifted_timestamp]['TASK_ID'] = numeric_task_id
                events_seen.add('TASK_ID')

    # Sort timestamps chronologically and events alphabetically
    sorted_timestamps = sorted(time_series.keys())
    event_columns = sorted(list(events_seen))

    # ==========================================
    # 2. Write to CSV in the Wide Format
    # ==========================================
    csv_filepath = os.path.join(output_dir, 'input.csv')
    with open(csv_filepath, mode='w', newline='') as csv_file:
        csv_writer = csv.writer(csv_file)

        # Header: timestamp, EVENT_1, EVENT_2, ...
        header = ['timestamp'] + event_columns
        csv_writer.writerow(header)

        row_count = 0
        for t in sorted_timestamps:
            row = [t]
            for event in event_columns:
                val = time_series[t].get(event, '')
                row.append(val)

            csv_writer.writerow(row)
            row_count += 1

    print(f"Successfully pivoted {row_count} chronological rows to '{csv_filepath}'")

    # ==========================================
    # 3. Generate the MultiUnit SmartData Model
    # ==========================================
    variables = []
    for event_name in event_columns:
        semantics = event_name.replace('_', ' ').title()

        # Apply specific SI Units if defined, otherwise use default
        mapping = UNIT_MAPPING.get(event_name, UNIT_MAPPING['default'])

        variables.append(
            {
                'name': event_name,
                'semantics': semantics,
                'si_quantity': mapping['si_quantity'],
                'unit': mapping['unit'],
                'dev': 0,
                'acquisition_mode': {'type': 'OLD'},
            }
        )

    # Sort explicitly to be absolutely bulletproof against the C++ ordering bug
    variables.sort(key=lambda x: x['name'])

    t0 = sorted_timestamps[0] if sorted_timestamps else 0
    tf = sorted_timestamps[-1] if sorted_timestamps else 0

    # Build the final model with the MultiUnit bit set (Bit 29 -> 536870912)
    smartdata_model = {
        'Model': {
            'version': '1.1',
            'x': 745051.448,
            'y': 6944572.972,
            'z': 8.0,
            'r': 1,
            't0': t0,
            'tf': tf,
            'elements': variables,
        }
    }

    json_filepath = os.path.join(output_dir, 'smartdata-model.json')
    with open(json_filepath, 'w') as json_file:
        json.dump(smartdata_model, json_file, indent=4)

    print(f"Successfully generated MultiUnit schema for {len(variables)} variables to '{json_filepath}'")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Convert RTOS JSON to SmartData MultiUnit Model & CSV')
    parser.add_argument('input_file', type=str, help='Path to the input JSON dataset')
    parser.add_argument('output_dir', type=str, help='Directory to save the generated files')
    parser.add_argument(
        '--epoch',
        type=float,
        default=None,
        help='Optional Unix epoch timestamp (in SECONDS) to shift the data to real-time. '
        'The script automatically converts this to microseconds.',
    )
    parser.add_argument('--limit', type=float, default=None, help='Limit the extraction strictly to this many seconds')

    args = parser.parse_args()
    convert_to_smartdata(args.input_file, args.output_dir, args.epoch, args.limit)
