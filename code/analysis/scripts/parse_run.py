from argparse import ArgumentParser
from enum import Enum
from re import search
from os.path import join
from os import sep

from definitions.benchmarks import Benchmark
from definitions.data_models import MonitorData, RunData, Task
from definitions.events import L2CachePMUEvent, L2CachePMUEventVariance, PMUEvent, PMUEventVariance, SystemEvent
from pydantic import BaseModel

THREAD_INDEX_PATTERN = r'\[(.*?)\]'
THREAD_ADDRESS_PATTERN = r'<(.*?)>'
THREAD_NAME_PATTERN = r'\((.*?)\)'


class ParsingState(Enum):
    RUN_INFO = 0
    MONITOR_INFO = 1
    TASKSET = 2
    TASK_TIMINGS = 3
    DATA = 4
    END = 5


class ParsingContext(BaseModel):
    state: ParsingState = ParsingState.RUN_INFO
    current_cpu: int | None = None
    current_event_index: int | None = None
    event_index_map: dict[
        int, dict[int, SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance]
    ] = {}
    current_thread: int | None = None
    run_data: RunData = RunData()


def parse_line(parsing_context: ParsingContext, line: str) -> None:
    line = line.strip()

    match parsing_context.state:
        case ParsingState.RUN_INFO:
            if line.startswith('>  Test duration'):
                parsing_context.run_data.test_duration = int(line.split()[-1])
            elif line.startswith('>  RNG Seed:'):
                parsing_context.run_data.rng_seed = int(line.split()[-1])
            elif line.startswith('>  Selected taskset'):
                parts = line.split()
                parsing_context.run_data.taskset.index = int(parts[-2].strip('[]'))
                parsing_context.run_data.taskset.name = parts[-1]
            elif line.startswith('Monitored events'):
                parsing_context.state = ParsingState.MONITOR_INFO
            elif line.startswith('Elapsed'):
                parsing_context.run_data.real_test_duration = int(line.split()[-1])
            elif '...............Threads Timing Behavior...............' in line:
                parsing_context.state = ParsingState.TASK_TIMINGS
        case ParsingState.MONITOR_INFO:
            if line.startswith('> Core'):
                parts = line.split()
                cpu = int(parts[-1][:1])
                parsing_context.current_cpu = cpu
                parsing_context.current_event_index = 0
                parsing_context.event_index_map[cpu] = {}
                parsing_context.run_data.data[cpu] = {}
            elif line.startswith('> System events') or line.startswith('> PMU events'):
                pass
            elif line.startswith('Creating threads...'):
                parsing_context.state = ParsingState.TASKSET
                parsing_context.current_cpu = None
            else:
                parts = line.split()

                if len(parts) <= 1:
                    return

                event_type, event = parts[1].split('::')

                match event_type:
                    case 'System':
                        event = SystemEvent(event)
                    case 'PMU':
                        event = PMUEvent(event)
                    case 'L2_PMU':
                        event = L2CachePMUEvent(event)

                parsing_context.run_data.data[parsing_context.current_cpu][event] = MonitorData(event=event)  # type: ignore
                parsing_context.event_index_map[parsing_context.current_cpu][parsing_context.current_event_index] = (  # type: ignore
                    event
                )

                parsing_context.current_event_index += 1  # type: ignore
        case ParsingState.TASKSET:
            if line.startswith('>  Thread'):
                index_match = search(THREAD_INDEX_PATTERN, line)
                address_match = search(THREAD_ADDRESS_PATTERN, line)
                name_match = search(THREAD_NAME_PATTERN, line)

                parsing_context.current_thread = index_match.group(1)  # type: ignore
                parsing_context.run_data.taskset.tasks[parsing_context.current_thread] = Task(  # type: ignore
                    thread=int(address_match.group(1), 16),  # type: ignore
                    benchmark=Benchmark(name_match.group(1)),  # type: ignore
                )
            elif line.startswith('Period'):
                parts = line.split()

                parsing_context.run_data.taskset.tasks[parsing_context.current_thread].period = int(parts[2].strip(','))  # type: ignore
                parsing_context.run_data.taskset.tasks[parsing_context.current_thread].deadline = int(  # type: ignore
                    parts[5].strip(',')
                )
                parsing_context.run_data.taskset.tasks[parsing_context.current_thread].wcet = int(parts[8].strip(','))  # type: ignore
            elif 'cpu' in line:
                parts = line.split()

                parsing_context.run_data.taskset.tasks[parsing_context.current_thread].job_executions = int(  # type: ignore
                    parts[2].strip(',')
                )
                parsing_context.run_data.taskset.tasks[parsing_context.current_thread].cpu = int(parts[5].strip(','))  # type: ignore
            elif line.startswith('Done'):
                parsing_context.state = ParsingState.RUN_INFO
        case ParsingState.TASK_TIMINGS:
            if line.startswith('Task ['):
                index_match = search(THREAD_INDEX_PATTERN, line)
                parsing_context.current_thread = index_match.group(1)  # type: ignore
            elif line.startswith('>   Counted iterations'):
                parsing_context.run_data.taskset.tasks[parsing_context.current_thread].task_timing.job_iterations = int(  # type: ignore
                    line.split()[-1]
                )
            elif line.startswith('>   Execution time'):
                parsing_context.run_data.taskset.tasks[  # type: ignore
                    parsing_context.current_thread
                ].task_timing.execution_time = int(line.split()[-2])  # type: ignore
            elif line.startswith('>   Job WCET'):
                parsing_context.run_data.taskset.tasks[parsing_context.current_thread].task_timing.job_wcet = int(  # type: ignore
                    line.split()[-2]
                )
            elif line.startswith('>   Iteration WCET'):
                parsing_context.run_data.taskset.tasks[  # type: ignore
                    parsing_context.current_thread
                ].task_timing.iteration_wcet = int(line.split()[-2])  # type: ignore
            elif line.startswith('>   Average job runtime'):
                parsing_context.run_data.taskset.tasks[  # type: ignore
                    parsing_context.current_thread
                ].task_timing.average_job_runtime = int(line.split()[-2])  # type: ignore
            elif line.startswith('>   Average iteration runtime') or line.startswith('>   Agerage iteration runtime'):
                parsing_context.run_data.taskset.tasks[  # type: ignore
                    parsing_context.current_thread
                ].task_timing.average_iteration_runtime = int(line.split()[-2])  # type: ignore
            elif line.startswith('>   Iterations per job'):
                parsing_context.run_data.taskset.tasks[  # type: ignore
                    parsing_context.current_thread
                ].task_timing.iteration_per_job = int(line.split()[-1])  # type: ignore
            elif line.startswith('begin_data'):
                parsing_context.state = ParsingState.DATA
        case ParsingState.DATA:
            if line.startswith('CPU'):
                cpu = int(line[3:])
                parsing_context.current_cpu = cpu
                parsing_context.current_event_index = -1
            elif line.startswith('TS,'):
                parsing_context.current_event_index += 1  # type: ignore
            elif line.startswith('end_data'):
                parsing_context.state = ParsingState.END
            else:
                parts = line.split(',')
                if len(parts) == 2 and parts[0].isdigit():
                    timestamp, value = int(parts[0]), int(parts[1])

                    event = parsing_context.event_index_map[parsing_context.current_cpu][  # type: ignore
                        parsing_context.current_event_index
                    ]

                    parsing_context.run_data.data[parsing_context.current_cpu][event].data.append((timestamp, value))  # type: ignore


def parse_file(input_path: str) -> RunData:
    with open(input_path, 'r', encoding='utf-8') as file:
        lines = file.readlines()

    parsing_context = ParsingContext()

    for line in lines:
        parse_line(parsing_context, line)

        if parsing_context.state == ParsingState.END:
            break

    for core, data in parsing_context.run_data.data.items():
        monitor_data: list[MonitorData] = []

        for value in data.values():  # type: ignore
            monitor_data.append(value)

        parsing_context.run_data.data[core] = monitor_data

    return parsing_context.run_data


def main(input_path: str, output_directory: str) -> None:
    run_data = parse_file(input_path)
    json_str = run_data.model_dump_json(indent=4)

    path = join(output_directory, input_path.split(sep)[-1].replace('.log', '.json'))

    print(f'Saving file to {path}')

    with open(join(output_directory, path), 'w+', encoding='utf-8') as file:
        file.write(json_str)


if __name__ == '__main__':
    parser = ArgumentParser(description='Parse raw log files into structured jsons')
    parser.add_argument('input_path', help='Path to the raw log file')
    parser.add_argument('output_directory', help='Path to a directory to save the json file')

    args = parser.parse_args()

    main(args.input_path, args.output_directory)
