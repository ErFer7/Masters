from argparse import ArgumentParser
from enum import Enum
from sys import argv

from scripts.definitions import RunData


class ParsingState(Enum):
    RUN_INFO = 0
    MONITOR_INFO = 1
    TASK_TIMINGS = 2
    DATA = 3
    END = 4


def parse_line(parsing_state: dict, line: str, parsed_data: dict) -> None:
    line = line.strip()

    match parsing_state['state']:
        case ParsingState.RUN_INFO:
            if line.startswith('>  Test duration'):
                parsed_data['test_duration'] = int(line.split()[-1])
            elif line.startswith('>  Selected taskset'):
                parsed_data['selected_taskset'] = int(line.split()[-1])
            elif line.startswith('Monitored events'):
                parsing_state['state'] = ParsingState.MONITOR_INFO
            elif line.startswith('Elapsed'):
                parsed_data['real_test_duration'] = int(line.split()[-1])
            elif '...............Threads Timing Behavior...............' in line:
                parsed_data['task_timings'] = []
                parsing_state['state'] = ParsingState.TASK_TIMINGS
        case ParsingState.MONITOR_INFO:
            if line.startswith('> Core'):
                line_parts = line.split()
                cpu = int(line_parts[-1][:1])
                parsing_state['current_cpu'] = cpu
                parsing_state['events'][cpu] = []
            elif line.startswith('> System events') or line.startswith('> PMU events'):
                pass
            elif line.startswith('Taskset initialization...'):
                parsing_state['state'] = ParsingState.RUN_INFO
                parsing_state['current_cpu'] = None
            else:
                line_parts = line.split()

                if len(line_parts) <= 1:
                    return

                event_type, event = line_parts[1].split('::')

                match event_type:
                    case 'System':
                        event = SystemEvent(event)
                    case 'PMU':
                        event = PMUEvent(event)
                    case 'L2_PMU':
                        event = L2CachePMUEvent(event)

                parsing_state['events'][parsing_state['current_cpu']].append(event)
        case ParsingState.TASK_TIMINGS:
            if line.startswith('Task ['):
                line_parts = line.split()
                index = int(line_parts[1].strip('[]:'))
                address = int(line_parts[2], 16)
                parsed_data['task_timings'].append({'task': index, 'address': address})
            elif line.startswith('>   Counted iterations'):
                parsed_data['task_timings'][-1]['job_iterations'] = int(line.split()[-1])
            elif line.startswith('>   Execution time'):
                parsed_data['task_timings'][-1]['execution_time'] = int(line.split()[-2])
            elif line.startswith('>   Job WCET'):
                parsed_data['task_timings'][-1]['job_wcet'] = int(line.split()[-2])
            elif line.startswith('>   Iteration WCET'):
                parsed_data['task_timings'][-1]['iteration_wcet'] = int(line.split()[-2])
            elif line.startswith('>   Average job runtime'):
                parsed_data['task_timings'][-1]['average_job_runtime'] = int(line.split()[-2])
            elif line.startswith('>   Average iteration runtime') or line.startswith('>   Agerage iteration runtime'):
                parsed_data['task_timings'][-1]['average_iteration_runtime'] = int(line.split()[-2])
            elif line.startswith('>   Iterations per job'):
                parsed_data['task_timings'][-1]['iteration_per_job'] = int(line.split()[-1])
            elif line.startswith('begin_data'):
                parsed_data['data'] = {}
                parsing_state['state'] = ParsingState.DATA
        case ParsingState.DATA:
            if line.startswith('CPU'):
                cpu = int(line[3:])
                parsed_data['data'][cpu] = []
                parsing_state['current_cpu'] = cpu
                parsing_state['current_event_index'] = None
            elif line.startswith('TS,'):
                current_cpu = parsing_state['current_cpu']
                current_event_index = parsing_state['current_event_index']

                if (
                    current_event_index is not None
                    and current_event_index != -1
                    and current_event_index + 1 >= len(parsing_state['events'][current_cpu])
                ):
                    parsing_state['current_event_index'] = -1
                    return

                if current_event_index is None:
                    parsing_state['current_event_index'] = 0
                else:
                    parsing_state['current_event_index'] += 1

                event = parsing_state['events'][current_cpu][parsing_state['current_event_index']]
                parsed_data['data'][current_cpu].append({'event': event, 'data': []})
            elif line.startswith('end_data'):
                parsing_state['state'] = ParsingState.END
            else:
                parts = line.split(',')
                if len(parts) == 2 and parts[0].isdigit():
                    timestamp, value = int(parts[0]), int(parts[1])
                    parsed_data['data'][parsing_state['current_cpu']][-1]['data'].append((timestamp, value))


def parse_file(input_path: str) -> RunData:
    with open(input_path, 'r', encoding='utf-8') as file:
        lines = file.readlines()

    parsing_state = {
        'state': ParsingState.RUN_INFO,
        'current_cpu': None,
        'current_event_index': None,
        'events': {},
    }
    parsed_data = {'data': {}, 'task_timings': []}

    for line in lines:
        parse_line(parsing_state, line, parsed_data)

        if parsing_state['state'] == ParsingState.END:
            break

    return RunData(**parsed_data)


def main(input_path: str, output_path: str) -> None:
    run_data = parse_file(input_path)
    json_str = run_data.model_dump_json(indent=4)

    with open(output_path, 'w+', encoding='utf-8') as file:
        file.write(json_str)


if __name__ == '__main__':
    parser = ArgumentParser(description='Parse raw log files into structured jsons')
    parser.add_argument('input_path', help='Path to the raw log file')
    parser.add_argument('output_path', help='Path to the json file')

    args = parser.parse_args()

    main(argv[1], argv[2])
