from pydantic import BaseModel

from definitions.benchmarks import Benchmark
from definitions.events import L2CachePMUEvent, L2CachePMUEventVariance, PMUEvent, PMUEventVariance, SystemEvent
from definitions.data_origins import DataOrigin


class TaskTiming(BaseModel):
    job_iterations: int = -1
    execution_time: int = -1
    job_wcet: int = -1
    iteration_wcet: int = -1
    average_job_runtime: int = -1
    average_iteration_runtime: int = -1
    iteration_per_job: int = -1


class Task(BaseModel):
    thread: int = -1
    period: int = -1
    deadline: int = -1
    wcet: int = -1
    cpu: int = -1
    job_executions: int = -1
    benchmark: Benchmark | None = None
    task_timing: TaskTiming | None = TaskTiming()


class Taskset(BaseModel):
    index: int = -1
    tasks: dict[int, Task] = {}
    name: str = ''


class MonitorData(BaseModel):
    event: SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance
    data: list[tuple[int, int | float]] = []


class RunData(BaseModel):
    test_duration: int = -1
    rng_seed: int = -1
    real_test_duration: int = -1
    taskset: Taskset = Taskset()
    data: dict[
        int,
        list[MonitorData]
        | dict[SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance, MonitorData],
    ] = {}


class PeriodicDataset(BaseModel):
    test_duration: int = -1
    rng_seed: int = -1
    collection_duration: int = -1
    taskset: Taskset = Taskset()
    data: dict[
        DataOrigin,
        list[MonitorData]
        | dict[SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance, MonitorData],
    ] = {}
