from pydantic import BaseModel

from scripts.definitions.benchmarks import Benchmark
from scripts.definitions.events import L2CachePMUEvent, L2CachePMUEventVariance, PMUEvent, PMUEventVariance, SystemEvent


class TaskTiming(BaseModel):
    job_iterations: int = 0
    execution_time: int = 0
    job_wcet: int = 0
    iteration_wcet: int = 0
    average_job_runtime: int = 0
    average_iteration_runtime: int = 0
    iteration_per_job: int = 0


class Task(BaseModel):
    thread: int
    period: int
    deadline: int
    wcet: int
    cpu: int
    job_executions: int
    benchmark: Benchmark
    task_timing: TaskTiming


class Taskset(BaseModel):
    index: int
    tasks: list[Task]
    name: str


class MonitorData(BaseModel):
    event: SystemEvent | PMUEvent | L2CachePMUEvent | PMUEventVariance | L2CachePMUEventVariance
    data: list[tuple[int, int | float]]


class RunData(BaseModel):
    test_duration: int = 0
    rng_seed: int = 0
    real_test_duration: int = 0
    taskset: Taskset
    data: dict[int, list[MonitorData]] = {}
