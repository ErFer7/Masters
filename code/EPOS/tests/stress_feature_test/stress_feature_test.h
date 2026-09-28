#pragma once

enum Benchmark { BANDWIDTH_HEAVY, CPU_HUNGRY, DISPARITY, BANDWIDTH_LIGHT, BANDWIDTH_MIXED, BANDWIDTH_RANDOM };

struct StressTask {
    const unsigned int period;
    const unsigned int deadline;
    const unsigned int wcet;
    const unsigned int cpu;
    const Benchmark task;
    const float duration;  // Iteration duration
};

struct Taskset {
    const StressTask *tasks;
    const unsigned int size;
};
