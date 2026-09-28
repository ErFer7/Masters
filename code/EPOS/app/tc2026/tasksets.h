#pragma once

#include "benchmarks.h"

struct StressTask {
    const unsigned int period;
    const unsigned int deadline;
    const unsigned int wcet;
    const unsigned int cpu;
    const BenchmarkType task;
    const float duration;  // Iteration duration
};

struct Taskset {
    const StressTask *tasks;
    const unsigned int size;
};

static constexpr float SINGLE = 200000.0f;

#ifdef __sifive_u__
static constexpr float BANDWIDTH_IT_DURATION = 370.0f;     // in microseconds
static constexpr float DISPARITY_IT_DURATION = 173910.0f;  // in microseconds
static constexpr float CPU_IT_DURATION = 16.0f;            // in microseconds
#endif

#ifdef __visionfive2__
static constexpr float BANDWIDTH_IT_DURATION = 1600.0f;    // in microseconds
static constexpr float DISPARITY_IT_DURATION = 200000.0f;  // in microseconds
static constexpr float CPU_IT_DURATION = 30.0f;            // in microseconds
#endif

// Old tasksets
static constexpr StressTask SBESC_TASKSET_1[] = {
    {1000000, 1000000, 200000, 1, BANDWIDTH_L2, BANDWIDTH_IT_DURATION},  // 20 - band
    {1000000, 1000000, 200000, 1, DISPARITY, DISPARITY_IT_DURATION},     // 20 - disp

    {1000000, 1000000, 200000, 2, DISPARITY, DISPARITY_IT_DURATION},  // 20 - disp
    {1000000, 1000000, 200000, 2, CPU_HUNGRY, CPU_IT_DURATION},       // 20 - cpu

    {1000000, 1000000, 200000, 3, CPU_HUNGRY, CPU_IT_DURATION},       // 20 - cpu
    {1000000, 1000000, 200000, 3, DISPARITY, DISPARITY_IT_DURATION},  // 20 - disp
};  // HP = 1

static constexpr StressTask SBESC_TASKSET_2[] = {
    {500000, 500000, 100000, 1, BANDWIDTH_L2, BANDWIDTH_IT_DURATION},  // 20 - band
    {1000000, 1000000, 200000, 1, DISPARITY, DISPARITY_IT_DURATION},   // 20 - disp

    {2000000, 2000000, 400000, 2, DISPARITY, DISPARITY_IT_DURATION},  // 20 - disp
    {500000, 500000, 70000, 2, CPU_HUNGRY, CPU_IT_DURATION},          // 20 - cpu

    {250000, 250000, 40000, 3, CPU_HUNGRY, CPU_IT_DURATION},        // 20 - cpu
    {500000, 500000, 200000, 3, DISPARITY, DISPARITY_IT_DURATION},  // 20 - disp
};  // HP = 2

static constexpr StressTask SBESC_TASKSET_3[] = {
    {200000, 200000, 20000, 1, BANDWIDTH_L1, BANDWIDTH_IT_DURATION},  // 20 - band

    {200000, 200000, 10000, 2, BANDWIDTH_L1, BANDWIDTH_IT_DURATION},  // 20 - band
    {2000000, 2000000, 800000, 2, DISPARITY, DISPARITY_IT_DURATION},  // 20 - disp

    {200000, 200000, 60000, 3, CPU_HUNGRY, CPU_IT_DURATION},             // 20 - cpu
    {1000000, 1000000, 200000, 3, DISPARITY, DISPARITY_IT_DURATION},     // 20 - disp
    {500000, 500000, 120000, 3, CPU_HUNGRY, CPU_IT_DURATION},            // 20 - cpu
    {2000000, 2000000, 200000, 3, BANDWIDTH_L1, BANDWIDTH_IT_DURATION},  // band
};  // HP = 2

// Low utilization taskset 1 (Can get to DVFS level 0)
constexpr static StressTask SBESC_TASKSET_1_LOW[] = {
    {1000000, 1000000, 50000, 1, BANDWIDTH_L2, BANDWIDTH_IT_DURATION},  // 10 - band
    {1000000, 1000000, 50000, 1, DISPARITY, DISPARITY_IT_DURATION},     // 10 - disp

    {1000000, 1000000, 50000, 2, DISPARITY, DISPARITY_IT_DURATION},  // 10 - disp
    {1000000, 1000000, 50000, 2, CPU_HUNGRY, CPU_IT_DURATION},       // 10 - cpu

    {1000000, 1000000, 50000, 3, CPU_HUNGRY, CPU_IT_DURATION},       // 10 - cpu
    {1000000, 1000000, 50000, 3, DISPARITY, DISPARITY_IT_DURATION},  // 10 - disp
};  // HP = 1

// Low utilization taskset 2 (Can get to DVFS level 0)
constexpr static StressTask SBESC_TASKSET_2_LOW[] = {
    {500000, 500000, 25000, 1, BANDWIDTH_L2, BANDWIDTH_IT_DURATION},  // 10 - band
    {1000000, 1000000, 50000, 1, DISPARITY, DISPARITY_IT_DURATION},   // 10 - disp

    {2000000, 2000000, 100000, 2, DISPARITY, DISPARITY_IT_DURATION},  // 10 - disp
    {500000, 500000, 17500, 2, CPU_HUNGRY, CPU_IT_DURATION},          // 10 - cpu

    {250000, 250000, 10000, 3, CPU_HUNGRY, CPU_IT_DURATION},       // 10 - cpu
    {500000, 500000, 50000, 3, DISPARITY, DISPARITY_IT_DURATION},  // 10 - disp
};  // HP = 2

// Overhead measuring taskset
static constexpr StressTask SBESC_TASKSET_OVERHEAD[] = {
    {1000000, 1000000, 200000, 1, BANDWIDTH_L2, BANDWIDTH_IT_DURATION},  // 10 - band

    {1000000, 1000000, 200000, 2, DISPARITY, DISPARITY_IT_DURATION},  // 10 - disp

    {1000000, 1000000, 200000, 3, CPU_HUNGRY, CPU_IT_DURATION},  // 10 - cpu
};  // HP = 2

// Taskset 7
// New tasksets
static constexpr StressTask TASKSET_1[] = {
    {1000000, 1000000, 150000, 1, H264_DEC, 20.0f},     // Memory bound  // 15%
    {1000000, 1000000, 150000, 1, DIJKSTRA, 18775.0f},  // Mixed         // 15%

    {1000000, 1000000, 150000, 2, HUFF_ENC, 228.0f},  // Mixed      // 15%
    {1000000, 1000000, 150000, 2, FAC, 0.11275f},     // CPU bound  // 15%

    {1000000, 1000000, 150000, 3, PRIME, 0.18f},        // CPU bound  // 15%
    {1000000, 1000000, 150000, 3, ADPCM_ENC, 1355.0f},  // Mixed      // 15%
};  // HP = 1

// Taskset 8
static constexpr StressTask TASKSET_2[] = {
    {1000000, 1000000, 150000, 1, BANDWIDTH_L2, SINGLE},  // 15%
    {1000000, 1000000, 150000, 1, GSM_ENC, SINGLE},       // 15%

    {1000000, 1000000, 150000, 2, POINTER_CHASE_L2, SINGLE},  // 15%
    {1000000, 1000000, 150000, 2, BITCOUNT, SINGLE},          // 15%

    {1000000, 1000000, 150000, 3, COSF, SINGLE},      // 15%
    {1000000, 1000000, 150000, 3, G723_ENC, SINGLE},  // 15%
};  // HP = 1

// Taskset 9
static constexpr StressTask TASKSET_3[] = {
    {1000000, 1000000, 150000, 1, MPEG2, SINGLE},      // 15%
    {1000000, 1000000, 150000, 1, STATEMATE, SINGLE},  // 15%

    {1000000, 1000000, 150000, 2, NDES, SINGLE},     // 15%
    {1000000, 1000000, 150000, 2, DEG2RAD, SINGLE},  // 15%

    {1000000, 1000000, 150000, 3, MD5, SINGLE},         // 15%
    {1000000, 1000000, 150000, 3, AMMUNITION, SINGLE},  // 15%
};  // HP = 1

// Taskset 10
static constexpr StressTask TASKSET_4[] = {
    {1000000, 1000000, 150000, 1, SUSAN, SINGLE},  // 15%
    {1000000, 1000000, 150000, 1, FMREF, SINGLE},  // 15%

    {1000000, 1000000, 150000, 2, DISPARITY, SINGLE},  // 15%
    {1000000, 1000000, 150000, 2, SHA, SINGLE},        // 15%

    {1000000, 1000000, 150000, 3, RIJNDAEL_ENC, SINGLE},  // 15%
    {1000000, 1000000, 150000, 3, DIJKSTRA, SINGLE},      // 15%
};  // HP = 1

// Taskset 11
static constexpr StressTask TASKSET_5[] = {
    {2000000, 2000000, 160000, 1, BANDWIDTH_L2, SINGLE},
    {2000000, 2000000, 160000, 1, PRIME, SINGLE},
    {2000000, 2000000, 160000, 1, HUFF_ENC, SINGLE},
    {2000000, 2000000, 160000, 1, POINTER_CHASE_L2, SINGLE},
    {2000000, 2000000, 160000, 1, DEG2RAD, SINGLE},

    {2000000, 2000000, 160000, 2, DIJKSTRA, SINGLE},
    {2000000, 2000000, 160000, 2, BANDWIDTH_L1, SINGLE},
    {2000000, 2000000, 160000, 2, BITCOUNT, SINGLE},
    {2000000, 2000000, 160000, 2, ADPCM_ENC, SINGLE},
    {2000000, 2000000, 160000, 2, SUSAN, SINGLE},

    {2000000, 2000000, 160000, 3, FAC, SINGLE},
    {2000000, 2000000, 160000, 3, H264_DEC, BANDWIDTH_IT_DURATION},
    {2000000, 2000000, 160000, 3, MPEG2, BANDWIDTH_IT_DURATION},
    {2000000, 2000000, 160000, 3, COSF, BANDWIDTH_IT_DURATION},
    {2000000, 2000000, 160000, 3, GSM_ENC, BANDWIDTH_IT_DURATION},
};  // HP = 1

static constexpr Taskset TASKSETS[] = {
    {SBESC_TASKSET_1, sizeof(SBESC_TASKSET_1) / sizeof(StressTask)},
    {SBESC_TASKSET_2, sizeof(SBESC_TASKSET_2) / sizeof(StressTask)},
    {SBESC_TASKSET_3, sizeof(SBESC_TASKSET_3) / sizeof(StressTask)},
    {SBESC_TASKSET_1_LOW, sizeof(SBESC_TASKSET_1_LOW) / sizeof(StressTask)},
    {SBESC_TASKSET_2_LOW, sizeof(SBESC_TASKSET_2_LOW) / sizeof(StressTask)},
    {SBESC_TASKSET_OVERHEAD, sizeof(SBESC_TASKSET_OVERHEAD) / sizeof(StressTask)},
    {TASKSET_1, sizeof(TASKSET_1) / sizeof(StressTask)},
    {TASKSET_2, sizeof(TASKSET_2) / sizeof(StressTask)},
    {TASKSET_3, sizeof(TASKSET_3) / sizeof(StressTask)},
    {TASKSET_4, sizeof(TASKSET_4) / sizeof(StressTask)},
    {TASKSET_5, sizeof(TASKSET_5) / sizeof(StressTask)},
};
