from sys import argv

from definitions import (
    L2CachePMUEvent,
    MonitorData,
    PMUEvent,
    PMUEventVariance,
    L2CachePMUEventVariance,
    SystemEvent,
    TaskRateDataset,
    TaskRateParsingState,
)


# TODO: Detect the taskset based on the data
def parse_line(parsing_state: dict, line: str, data: dict) -> None:
    line = line.strip()

    match parsing_state['state']:
        case TaskRateParsingState.RUN_INFO:
            if line.startswith('>  Selected taskset:'):
                data['taskset'] = int(line.split()[-1].strip())
            if line.startswith('Collected task data'):
                parsing_state['state'] = TaskRateParsingState.DATA
        case TaskRateParsingState.DATA:
            if line.startswith('Task data'):
                task = int(line.split()[2].strip('[]: '))
                data['data'][task] = {}
                parsing_state['current_task'] = task
                parsing_state['current_event'] = None
            elif line.startswith('System buffer') or line.startswith('PMU buffer') or line.startswith('ANN buffer'):
                event = None
                parts = line.split(':')
                is_variance = parts[0].split()[-1].strip('()') == 'variance'

                if is_variance:
                    try:
                        event = PMUEventVariance(parts[-1] + '_VARIANCE')
                    except ValueError:
                        pass

                    try:
                        event = L2CachePMUEventVariance(parts[-1] + '_VARIANCE')
                    except ValueError:
                        pass
                else:
                    try:
                        event = SystemEvent(parts[-1])
                    except ValueError:
                        pass

                    try:
                        event = PMUEvent(parts[-1])
                    except ValueError:
                        pass

                    try:
                        event = L2CachePMUEvent(parts[-1])
                    except ValueError:
                        pass

                parsing_state['current_event'] = event
                data['data'][parsing_state['current_task']][parsing_state['current_event']] = MonitorData(
                    **{'event': event, 'data': []}
                )
            elif line.startswith('Taskset deallocation...'):
                parsing_state['state'] = TaskRateParsingState.END
            else:
                parts = line.split(',')
                if len(parts) == 2 and parts[0].isdigit():
                    timestamp = int(parts[0])

                    try:
                        value = int(parts[1])
                    except ValueError:
                        value = float(parts[1])

                    data['data'][parsing_state['current_task']][parsing_state['current_event']].data.append(
                        (timestamp, value)
                    )


def parse_file(file_path: str) -> TaskRateDataset:
    with open(file_path, 'r', encoding='utf-8') as file:
        lines = file.readlines()

    parsing_state = {'state': TaskRateParsingState.RUN_INFO, 'current_task': None, 'current_event': None}

    data = {'taskset': None, 'data': {}}

    for line in lines:
        parse_line(parsing_state, line, data)

        if parsing_state['state'] == TaskRateParsingState.END:
            break

    for task in data['data']:
        data['data'][task] = list(data['data'][task].values())

    return TaskRateDataset(**data)


def main(file_path: str, saving_name: str) -> None:
    run_data = parse_file(file_path)
    json_str = run_data.model_dump_json(indent=4)

    with open(saving_name, 'w+', encoding='utf-8') as file:
        file.write(json_str)


if __name__ == '__main__':
    main(argv[1], argv[2])
