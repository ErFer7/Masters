#include "stress_feature_test.h"

#include <clerk.h>
#include <machine/pmic.h>
#include <process.h>
#include <real-time.h>
#include <time.h>
#include <utility/ostream.h>
#include <utility/random.h>

#include "cpu_hungry.h"
#include "disparity.h"
#include "machine/riscv/visionfive2/visionfive2_temperature_sensor.h"

using namespace EPOS;

OStream cout;

typedef TSC::Time_Stamp Time_Stamp;

// Configuration
const bool MEASURE_TIME = true;
const unsigned int TEST_DURATION = Traits<Build>::EXPECTED_SIMULATION_TIME - 10;  // in seconds
const bool DVFS_CHANGE = false;
const unsigned int DVFS_CHANGES = 4;
const unsigned int DVFS_CHANGE_ITERATION = TEST_DURATION / DVFS_CHANGES;
const bool ENABLE_EA_PEDF = false;
const bool PRINT_TASK_DATA = false;
const unsigned int SELECTED_TASKSET = 1;
const unsigned int ITERATION_CHANGE_BEHAVIOR = 90;
const unsigned int CACHE_LINE_SIZE = 64;
const unsigned int L1_CACHE_SIZE = 32 * 1024;
const unsigned int L2_CACHE_SIZE = 2 * 1024 * 1024;
const unsigned int BANDWIDTH_ALLOC_SIZE = L2_CACHE_SIZE;

// To be measured
constexpr float BANDWIDTH_IT_DURATION = 2800.0f;    // in microseconds
constexpr float DISPARITY_IT_DURATION = 200000.0f;  // in microseconds
constexpr float CPU_IT_DURATION = 0.00375f;         // in microseconds

constexpr static StressTask taskset_1[] = {
    {1000000, 1000000, 200000, 1, BANDWIDTH_HEAVY, BANDWIDTH_IT_DURATION},  // 20 - band
    {1000000, 1000000, 200000, 1, DISPARITY, DISPARITY_IT_DURATION},        // 20 - disp

    {1000000, 1000000, 200000, 2, DISPARITY, DISPARITY_IT_DURATION},  // 20 - disp
    {1000000, 1000000, 200000, 2, CPU_HUNGRY, CPU_IT_DURATION},       // 20 - cpu

    {1000000, 1000000, 200000, 3, CPU_HUNGRY, CPU_IT_DURATION},       // 20 - cpu
    {1000000, 1000000, 200000, 3, DISPARITY, DISPARITY_IT_DURATION},  // 20 - disp
};  // HP = 1

constexpr static StressTask taskset_2[] = {
    {500000, 500000, 100000, 1, BANDWIDTH_HEAVY, BANDWIDTH_IT_DURATION},  // 20 - band
    {1000000, 1000000, 200000, 1, DISPARITY, DISPARITY_IT_DURATION},      // 20 - disp

    {2000000, 2000000, 400000, 2, DISPARITY, DISPARITY_IT_DURATION},  // 20 - disp
    {500000, 500000, 70000, 2, CPU_HUNGRY, CPU_IT_DURATION},          // 20 - cpu

    {250000, 250000, 40000, 3, CPU_HUNGRY, CPU_IT_DURATION},        // 20 - cpu
    {500000, 500000, 200000, 3, DISPARITY, DISPARITY_IT_DURATION},  // 20 - disp
};  // HP = 2

constexpr static StressTask taskset_3[] = {
    {200000, 200000, 20000, 1, BANDWIDTH_LIGHT, BANDWIDTH_IT_DURATION},  // 20 - band

    {200000, 200000, 10000, 2, BANDWIDTH_LIGHT, BANDWIDTH_IT_DURATION},  // 20 - band
    {2000000, 2000000, 800000, 2, DISPARITY, DISPARITY_IT_DURATION},     // 20 - disp

    {200000, 200000, 60000, 3, CPU_HUNGRY, CPU_IT_DURATION},                // 20 - cpu
    {1000000, 1000000, 200000, 3, DISPARITY, DISPARITY_IT_DURATION},        // 20 - disp
    {500000, 500000, 120000, 3, CPU_HUNGRY, CPU_IT_DURATION},               // 20 - cpu
    {2000000, 2000000, 200000, 3, BANDWIDTH_LIGHT, BANDWIDTH_IT_DURATION},  // band
};  // HP = 2

constexpr static StressTask taskset_4[] = {
    // Band on CPU 1
    {500000, 500000, 100000, 1, BANDWIDTH_HEAVY, BANDWIDTH_IT_DURATION},  // 20 - band 50000

    // Disparity on CPU 2 (Parallel to band on 1)
    {2000000, 2000000, 400000, 2, CPU_HUNGRY, CPU_IT_DURATION},       // 20 - cpu
    {2000000, 2000000, 400000, 2, DISPARITY, DISPARITY_IT_DURATION},  // 20 - disp
    {1000000, 1000000, 200000, 2, CPU_HUNGRY, CPU_IT_DURATION},       // 20 - cpu
    //{500000, 500000,  50000,3,0}, // 10 - band

    {250000, 250000, 50000, 3, BANDWIDTH_HEAVY, BANDWIDTH_IT_DURATION},  // 20 - band
    {2000000, 2000000, 400000, 3, DISPARITY, DISPARITY_IT_DURATION},     // 20 - disp
};  // HP = 2

// Low utilization taskset 1 (Can get to DVFS level 0)
constexpr static StressTask taskset_5[] = {
    {1000000, 1000000, 50000, 1, BANDWIDTH_HEAVY, BANDWIDTH_IT_DURATION},  // 10 - band
    {1000000, 1000000, 50000, 1, DISPARITY, DISPARITY_IT_DURATION},        // 10 - disp

    {1000000, 1000000, 50000, 2, DISPARITY, DISPARITY_IT_DURATION},  // 10 - disp
    {1000000, 1000000, 50000, 2, CPU_HUNGRY, CPU_IT_DURATION},       // 10 - cpu

    {1000000, 1000000, 50000, 3, CPU_HUNGRY, CPU_IT_DURATION},       // 10 - cpu
    {1000000, 1000000, 50000, 3, DISPARITY, DISPARITY_IT_DURATION},  // 10 - disp
};  // HP = 1

// Low utilization taskset 2 (Can get to DVFS level 0)
constexpr static StressTask taskset_6[] = {
    {500000, 500000, 25000, 1, BANDWIDTH_HEAVY, BANDWIDTH_IT_DURATION},  // 10 - band
    {1000000, 1000000, 50000, 1, DISPARITY, DISPARITY_IT_DURATION},       // 10 - disp

    {2000000, 2000000, 100000, 2, DISPARITY, DISPARITY_IT_DURATION},  // 10 - disp
    {500000, 500000, 17500, 2, CPU_HUNGRY, CPU_IT_DURATION},          // 10 - cpu

    {250000, 250000, 10000, 3, CPU_HUNGRY, CPU_IT_DURATION},       // 10 - cpu
    {500000, 500000, 50000, 3, DISPARITY, DISPARITY_IT_DURATION},  // 10 - disp
};  // HP = 2

// Overhead measuring taskset
constexpr static StressTask taskset_7[] = {
    {1000000, 1000000, 200000, 1, BANDWIDTH_HEAVY, BANDWIDTH_IT_DURATION},  // 10 - band

    {1000000, 1000000, 200000, 2, DISPARITY, DISPARITY_IT_DURATION},  // 10 - disp

    {1000000, 1000000, 200000, 3, CPU_HUNGRY, CPU_IT_DURATION},       // 10 - cpu
};  // HP = 2

constexpr static Taskset tasksets[] = {{taskset_1, sizeof(taskset_1) / sizeof(StressTask)},
                                       {taskset_2, sizeof(taskset_2) / sizeof(StressTask)},
                                       {taskset_3, sizeof(taskset_3) / sizeof(StressTask)},
                                       {taskset_4, sizeof(taskset_4) / sizeof(StressTask)},
                                       {taskset_5, sizeof(taskset_5) / sizeof(StressTask)},
                                       {taskset_6, sizeof(taskset_6) / sizeof(StressTask)},
                                       {taskset_7, sizeof(taskset_7) / sizeof(StressTask)}};

constexpr static Taskset taskset = tasksets[SELECTED_TASKSET - 1];
constexpr static unsigned int task_count = taskset.size;

// Definitions
volatile unsigned long long *bandwidth_array[task_count];

Time_Stamp task_runtime[task_count];
Time_Stamp job_wcet[task_count];
Time_Stamp iteration_wcet[task_count];

Random *rand;
unsigned int current_iteration[task_count];
bool behavior[task_count];
Thread *threads[task_count];

signed char *t_img1[task_count];
signed char *t_img2[task_count];

DisparityAlloc disparity_allocs[task_count];

unsigned int configurable_bench_write(unsigned int id, unsigned int mem_size, bool randomicity = false);

int disparity(int id);

// Functions
inline Time_Stamp get_time();
inline Microsecond us(Time_Stamp ts);

constexpr int calc_iter_per_job(int i);
constexpr int calc_jobs(int i);

int freq_control();

void init_taskset();
void free_taskset();

template <int ID>
inline void init_thread(Microsecond activation);

template <>
inline void init_thread<task_count>(Microsecond activation);

template <unsigned int ID>
void run_func();

int main() {
    cout << "Stress feature test\n"
         << "Running experiments with the following configurations:\n"
         << ">  Measure time: " << (MEASURE_TIME ? "true" : "false") << '\n'
         << ">  Test duration: " << TEST_DURATION << '\n'
         << ">  Selected taskset: " << SELECTED_TASKSET << '\n'
         << ">  Size of image 1: " << sizeof(img1) << '\n'
         << ">  Size of image 2: " << sizeof(img2) << '\n'
         << ">  Size of the bandwidth allocation: " << BANDWIDTH_ALLOC_SIZE << '\n'
         << ">  CPU Clock: " << CPU::clock() / 1000000 << "MHz" << '\n'
         << ">  CPU voltage: " << PMIC::cpu_voltage() << "mV" << '\n'
         << ">  DDR Clock: " << Clock_Tree::ddr_clock() / 1000000 << "MHz" << endl;

    Monitor::print_monitor_info();

    Delay(500000);
    rand->seed(0);

    init_taskset();

    Time_Stamp tsc0 = get_time() + Convert::us2count<Time_Stamp, Time_Base>(TSC::frequency(), 10000);

    cout << "Creating threads..." << endl;
    init_thread<0>(10000);
    cout << "Done" << endl;

    // Monitor::enable_captures();

    cout << "Current time: " << us(tsc0) << endl;

    Thread *freq =
        new Thread(Thread::Configuration(Thread::READY, Thread::Criterion(1000000, 1000000, 10000, 0)), &freq_control);

    cout << "Starting tasks..." << endl;
    for (unsigned int i = 0; i < task_count; i++) {
        threads[i]->resume();
    }

    if (ENABLE_EA_PEDF) Thread::Criterion::enable();

    cout << "Done" << endl;

    for (unsigned int i = 0; i < task_count; i++) {
        threads[i]->join();
        // Monitor::disable_captures();
    }

    if (ENABLE_EA_PEDF) Thread::Criterion::disable();

    freq->join();

    Time_Stamp times = get_time() - tsc0;

    // Monitor::disable_captures();

    cout << "Returned to application main!" << endl;
    cout << "Elapsed: " << us(times) << endl;
    cout << "Threads: " << task_count << endl;

    cout << "-----------------------------------------------------" << endl;
    cout << "...............Threads Timing Behavior..............." << endl;
    cout << "-----------------------------------------------------" << endl;

    for (unsigned int i = 0; i < task_count; i++) {
        cout << "Task [" << i << "]: " << threads[i] << '\n'
             << ">   Counted iterations: " << current_iteration[i] << endl;

        if (MEASURE_TIME) {
            const int total_jobs = calc_jobs(i);
            const int total_iterations = total_jobs * calc_iter_per_job(i);

            cout << ">   Execution time: " << us(task_runtime[i]) << " us\n"
                 << ">   Job WCET: " << us(job_wcet[i]) << " us\n"
                 << ">   Iteration WCET: " << us(iteration_wcet[i]) << " us\n"
                 << ">   Average job runtime: " << us(task_runtime[i] / total_jobs) << " us\n"
                 << ">   Average iteration runtime: " << us(task_runtime[i] / total_iterations) << " us\n"
                 << ">   Iterations per job: " << calc_iter_per_job(i) << endl;
        }
    }

    if (PRINT_TASK_DATA) {
        cout << "Collected task data:" << endl;

        for (unsigned int i = 0; i < task_count; i++) {
            cout << "Task data [" << i << "]: " << endl;
            threads[i]->criterion().print_data();
        }
    }

    free_taskset();

    return 0;
}

inline Time_Stamp get_time() { return TSC::time_stamp(); }

inline Microsecond us(Time_Stamp ts) { return Convert::count2us<Hertz, Time_Stamp, Time_Base>(TSC::frequency(), ts); }

constexpr int calc_iter_per_job(int i) {
    int iterations = static_cast<int>(static_cast<float>(taskset.tasks[i].wcet) / taskset.tasks[i].duration);
    if (iterations < 1) iterations = 1;

    return iterations;
}

constexpr int calc_jobs(int i) { return (TEST_DURATION * 1000000) / taskset.tasks[i].period; }

void init_taskset() {
    cout << "Taskset initialization..." << endl;

    for (unsigned int i = 0; i < task_count; i++) {
        switch (taskset.tasks[i].task) {
            case DISPARITY:
                t_img1[i] = new signed char[sizeof(img1)];
                t_img2[i] = new signed char[sizeof(img2)];

                for (unsigned int j = 0; j < sizeof(img1); j++) {
                    t_img1[i][j] = img1[j];
                }

                for (unsigned int j = 0; j < sizeof(img2); j++) {
                    t_img2[i][j] = img2[j];
                }

                break;
            case BANDWIDTH_HEAVY:
            case BANDWIDTH_LIGHT:
            case BANDWIDTH_MIXED:
            case BANDWIDTH_RANDOM:
                bandwidth_array[i] = new unsigned long long[BANDWIDTH_ALLOC_SIZE / sizeof(unsigned long long)];

                for (unsigned long long j = 0; j < BANDWIDTH_ALLOC_SIZE / sizeof(unsigned long long); j++) {
                    bandwidth_array[i][j] = j + j * i;
                }
                break;
            case CPU_HUNGRY:
            default:
                break;
        }
    }

    cout << "Done." << endl;
}

void free_taskset() {
    cout << "Taskset deallocation..." << endl;

    for (unsigned int i = 0; i < task_count; i++) {
        switch (taskset.tasks[i].task) {
            case DISPARITY:
                delete t_img1[i];
                delete t_img2[i];
                break;
            case BANDWIDTH_HEAVY:
            case BANDWIDTH_LIGHT:
            case BANDWIDTH_MIXED:
            case BANDWIDTH_RANDOM:
                delete bandwidth_array[i];
                break;
            case CPU_HUNGRY:
            default:
                break;
        }

        cout << ">  Deleting thread " << threads[i] << endl;

        delete threads[i];
    }

    cout << "Done." << endl;
}

template <int ID>
inline void init_thread(Microsecond activation) {
    constexpr int job = calc_jobs(ID);

    cout << ">  Thread[" << ID << "]: period = " << taskset.tasks[ID].period
         << ", deadline = " << taskset.tasks[ID].deadline << ", wcet = " << taskset.tasks[ID].wcet
         << ", activation = " << activation << ", times = " << job << ", cpu = " << taskset.tasks[ID].cpu << endl;

    current_iteration[ID] = 0;
    behavior[ID] = false;

    threads[ID] = new RT_Thread(&run_func<ID>,
                                taskset.tasks[ID].period,
                                taskset.tasks[ID].deadline,
                                taskset.tasks[ID].wcet,
                                activation,
                                job,
                                taskset.tasks[ID].cpu);

    init_thread<ID + 1>(activation);
}

template <>
inline void init_thread<task_count>(Microsecond activation) {}

template <unsigned int ID>
void run_func() {
    Time_Stamp init;

    unsigned int my_iter_per_job = calc_iter_per_job(ID);
    volatile unsigned int _;

    if ((current_iteration[ID] + 1) % ITERATION_CHANGE_BEHAVIOR == 0) {
        behavior[ID] = !behavior[ID];
    }

    if (MEASURE_TIME) init = get_time();

    for (unsigned int iterations = 0; iterations < my_iter_per_job; iterations++) {
        switch (taskset.tasks[ID].task) {
            case BANDWIDTH_HEAVY:
                _ += configurable_bench_write(ID, L2_CACHE_SIZE);  // heavy
                break;
            case CPU_HUNGRY:
                _ += cpu_hungry();
                break;
            case DISPARITY:
                _ += disparity(ID);
                break;
            case BANDWIDTH_LIGHT:
                _ += configurable_bench_write(ID, L1_CACHE_SIZE);  // light
                break;
            case BANDWIDTH_MIXED:
                if (behavior[ID]) {
                    _ += configurable_bench_write(ID, L2_CACHE_SIZE);  // heavy
                } else {
                    _ += configurable_bench_write(ID, L1_CACHE_SIZE);  // light
                }
                break;
            case BANDWIDTH_RANDOM:
            default:
                _ += configurable_bench_write(ID, 0, true);  // random
                break;
        }
    }

    if (MEASURE_TIME) {
        Time_Stamp job_runtime = get_time() - init;
        task_runtime[ID] += job_runtime;

        current_iteration[ID]++;

        Time_Stamp average_iteration_runtime = job_runtime / my_iter_per_job;

        if (average_iteration_runtime > iteration_wcet[ID]) {
            iteration_wcet[ID] = average_iteration_runtime;
        }

        if (job_runtime > job_wcet[ID]) {
            job_wcet[ID] = job_runtime;
        }
    }
}

int freq_control() {
    unsigned int dvfs_level = 3;
    bool descending = true;

    for (unsigned int i = 0; i < TEST_DURATION; i++) {
        cout << ">  Iteration [" << i << "], Clock: " << Clock_Tree::cpu_clock() << "Hz"
             << ", Temperature: " << static_cast<float>(Temperature_Sensor::get_temperature()) / 1000.0f << 'C' << endl;

        if (DVFS_CHANGE && !(i % DVFS_CHANGE_ITERATION) && i > 0) {
            kout << "Changing the DVFS level from " << dvfs_level << " to " << (dvfs_level + (descending ? -1 : 1))
                 << endl;
            // Clock_Tree::cpu_clock(descending ? --dvfs_level : ++dvfs_level);

            if (dvfs_level == 0 || dvfs_level == 3) {
                descending = !descending;
            }

            kout << "Clock changed to " << Clock_Tree::cpu_clock() << "Hz" << endl;
        }

        Delay(1000000);
    }

    return TEST_DURATION;
}

unsigned int configurable_bench_write(unsigned int id, unsigned int mem_size, bool randomicity) {
    unsigned long long limit = static_cast<unsigned long long>(mem_size) / sizeof(unsigned long long);
    unsigned long long increment = static_cast<unsigned long long>(CACHE_LINE_SIZE) / sizeof(unsigned long long);

    if (randomicity) {
        unsigned long long address = 0;
        mem_size = rand->random() % 2 ? L2_CACHE_SIZE : L1_CACHE_SIZE;
        limit = static_cast<unsigned long long>(mem_size) / sizeof(unsigned long long);

        for (unsigned long long i = 0; i < limit; i += increment) {
            address = ((static_cast<unsigned long long>(rand->random()) % limit) * i) % limit;
            bandwidth_array[id][address] += i;
        }
    } else {
        for (unsigned long long i = 0; i < limit; i += increment) {
            bandwidth_array[id][i] += i;
        }
    }

    return mem_size;
}

int disparity(int id) {
    I2D *imleft, *imright, *retDisparity;

    int WIN_SZ = 8, SHIFT = 64;

    imleft = (I2D *)t_img1[id];
    imright = (I2D *)t_img2[id];

    // We don't need to delete the returned data because it was allocated in the stack before (disparity_allocs)
    retDisparity = getDisparity(imleft, imright, WIN_SZ, SHIFT, &disparity_allocs[id]);
    int height = retDisparity->height;

    return height;
}
