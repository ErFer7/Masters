#include "stress_test.hpp"

#include <linux/sched.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

#include "cpu_hungry.h"
#include "disparity.h"

using std::cout;
using std::endl;

// Configuration
const bool MEASURE_TIME = true;
const unsigned int TEST_DURATION = 30;  // in seconds
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
// constexpr float CPU_IT_DURATION = 0.003f;  // in microseconds

constexpr static StressTask taskset_1[] = {
    {1000000, 1000000, 200000, 1, BANDWIDTH_HEAVY, BANDWIDTH_IT_DURATION},
    {1000000, 1000000, 200000, 1, DISPARITY, DISPARITY_IT_DURATION},
    {1000000, 1000000, 200000, 2, DISPARITY, DISPARITY_IT_DURATION},
    {1000000, 1000000, 200000, 2, CPU_HUNGRY, CPU_IT_DURATION},
    {1000000, 1000000, 200000, 3, CPU_HUNGRY, CPU_IT_DURATION},
    {1000000, 1000000, 200000, 3, DISPARITY, DISPARITY_IT_DURATION},
};

constexpr static StressTask taskset_2[] = {
    {500000, 500000, 100000, 1, BANDWIDTH_HEAVY, BANDWIDTH_IT_DURATION},
    {1000000, 1000000, 200000, 1, DISPARITY, DISPARITY_IT_DURATION},
    {2000000, 2000000, 400000, 2, DISPARITY, DISPARITY_IT_DURATION},
    {500000, 500000, 70000, 2, CPU_HUNGRY, CPU_IT_DURATION},
    {250000, 250000, 40000, 3, CPU_HUNGRY, CPU_IT_DURATION},
    {500000, 500000, 200000, 3, DISPARITY, DISPARITY_IT_DURATION},
};

constexpr static StressTask taskset_3[] = {
    {200000, 200000, 20000, 1, BANDWIDTH_LIGHT, BANDWIDTH_IT_DURATION},
    {200000, 200000, 10000, 2, BANDWIDTH_LIGHT, BANDWIDTH_IT_DURATION},
    {2000000, 2000000, 800000, 2, DISPARITY, DISPARITY_IT_DURATION},
    {200000, 200000, 60000, 3, CPU_HUNGRY, CPU_IT_DURATION},
    {1000000, 1000000, 200000, 3, DISPARITY, DISPARITY_IT_DURATION},
    {500000, 500000, 120000, 3, CPU_HUNGRY, CPU_IT_DURATION},
    {2000000, 2000000, 200000, 3, BANDWIDTH_LIGHT, BANDWIDTH_IT_DURATION},
};

constexpr static StressTask taskset_4[] = {
    {500000, 500000, 100000, 1, BANDWIDTH_HEAVY, BANDWIDTH_IT_DURATION},
    {2000000, 2000000, 400000, 2, CPU_HUNGRY, CPU_IT_DURATION},
    {2000000, 2000000, 400000, 2, DISPARITY, DISPARITY_IT_DURATION},
    {1000000, 1000000, 200000, 2, CPU_HUNGRY, CPU_IT_DURATION},
    {250000, 250000, 50000, 3, BANDWIDTH_HEAVY, BANDWIDTH_IT_DURATION},
    {2000000, 2000000, 400000, 3, DISPARITY, DISPARITY_IT_DURATION},
};

constexpr static Taskset tasksets[] = {{taskset_1, sizeof(taskset_1) / sizeof(StressTask)},
                                       {taskset_2, sizeof(taskset_2) / sizeof(StressTask)},
                                       {taskset_3, sizeof(taskset_3) / sizeof(StressTask)},
                                       {taskset_4, sizeof(taskset_4) / sizeof(StressTask)}};

constexpr static Taskset taskset = tasksets[SELECTED_TASKSET - 1];
constexpr static unsigned int task_count = taskset.size;

typedef uint64_t Time_Stamp;
typedef uint64_t Microsecond;

volatile unsigned long long *bandwidth_array[task_count];
Time_Stamp task_runtime[task_count];
Time_Stamp job_wcet[task_count];
Time_Stamp iteration_wcet[task_count];
unsigned int actual_cpu[task_count];

pthread_barrier_t start_barrier;

class RandomGen {
    unsigned int seed_val;

   public:
    void seed(unsigned int s) { seed_val = s; }
    unsigned int random() { return rand_r(&seed_val); }
};

RandomGen *rng;

unsigned int current_iteration[task_count];
bool behavior[task_count];

signed char *t_img1[task_count];
signed char *t_img2[task_count];

DisparityAlloc disparity_allocs[task_count];

unsigned int configurable_bench_write(unsigned int id, unsigned int mem_size, bool randomicity = false);
int disparity(int id);

inline Time_Stamp get_time();
inline Microsecond us(Time_Stamp ts);
constexpr int calc_iter_per_job(int i);
constexpr int calc_jobs(int i);

void init_taskset();
void free_taskset();

template <unsigned int ID>
void run_func();

struct local_sched_attr {
    uint32_t size;
    uint32_t sched_policy;
    uint64_t sched_flags;
    int32_t sched_nice;
    uint32_t sched_priority;
    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
};

int sched_setattr(pid_t pid, const struct local_sched_attr *attr, unsigned int flags) {
    return syscall(SYS_sched_setattr, pid, attr, flags);
}

#ifndef SCHED_DEADLINE
#define SCHED_DEADLINE 6
#endif

struct ThreadParams {
    unsigned int period_us;
    unsigned int deadline_us;
    unsigned int wcet_us;
    unsigned int activation_us;
    int jobs;
    int cpu;
    void (*task_func)();
};

void *rt_thread_wrapper(void *arg) {
    ThreadParams *p = static_cast<ThreadParams *>(arg);
    pid_t tid = syscall(SYS_gettid);

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(p->cpu, &cpuset);
    if (sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset) != 0) {
        perror("Failed to set CPU affinity");
    }

    struct local_sched_attr attr = {};
    attr.size = sizeof(attr);
    attr.sched_policy = SCHED_DEADLINE;
    attr.sched_runtime = static_cast<uint64_t>(p->wcet_us) * 1000ULL;
    attr.sched_deadline = static_cast<uint64_t>(p->deadline_us) * 1000ULL;
    attr.sched_period = static_cast<uint64_t>(p->period_us) * 1000ULL;

    if (sched_setattr(tid, &attr, 0) != 0) {
        perror("sched_setattr failed");
    }

    pthread_barrier_wait(&start_barrier);

    if (p->activation_us > 0) {
        struct timespec act_time;

        clock_gettime(CLOCK_MONOTONIC, &act_time);
        act_time.tv_nsec += p->activation_us * 1000ULL;

        while (act_time.tv_nsec >= 1000000000ULL) {
            act_time.tv_sec++;
            act_time.tv_nsec -= 1000000000ULL;
        }

        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &act_time, NULL);
    }

    for (int j = 0; j < p->jobs; j++) {
        p->task_func();

        sched_yield();
    }

    return nullptr;
}

class RT_Thread {
   public:
    pthread_t thread;
    ThreadParams params;

    RT_Thread(void (*func)(), int period, int deadline, int wcet, int activation, int jobs, int cpu) {
        params = {(unsigned int)period,
                  (unsigned int)deadline,
                  (unsigned int)wcet,
                  (unsigned int)activation,
                  jobs,
                  cpu,
                  func};

        pthread_create(&thread, nullptr, rt_thread_wrapper, &params);
    }

    void join() { pthread_join(thread, nullptr); }
    void resume() { /* Start triggered by global barrier in main */ }
};

RT_Thread *threads[task_count];

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

// Hardware sensor emulation via sysfs
long long get_cpu_clock() {
    std::ifstream file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
    long long freq = 0;
    if (file >> freq) return freq * 1000;
    return 0;
}

int freq_control() {
    for (unsigned int i = 0; i < TEST_DURATION; i++) {
        cout << ">  Iteration [" << i << "], Clock: " << get_cpu_clock() << "Hz" << endl;
        std::this_thread::sleep_for(std::chrono::microseconds(1000000));
    }

    return TEST_DURATION;
}

int main() {
    // Prevent kernel paging for hard real-time guarantees
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        std::cerr << "Warning: Failed to lock memory.\n";
    }

    cout << "Stress feature test\n"
         << "Running experiments with the following configurations:\n"
         << ">  Measure time: " << (MEASURE_TIME ? "true" : "false") << '\n'
         << ">  Test duration: " << TEST_DURATION << '\n'
         << ">  Selected taskset: " << SELECTED_TASKSET << '\n'
         << ">  Size of image 1: " << sizeof(img1) << '\n'
         << ">  Size of image 2: " << sizeof(img2) << '\n'
         << ">  Size of the bandwidth allocation: " << BANDWIDTH_ALLOC_SIZE << '\n'
         << ">  CPU Clock: " << get_cpu_clock() / 1000000 << "MHz" << endl;

    std::this_thread::sleep_for(std::chrono::microseconds(500000));

    rng = new RandomGen();
    rng->seed(0);

    init_taskset();

    // +1 for the main thread executing the resume barrier wait
    pthread_barrier_init(&start_barrier, NULL, task_count + 1);

    Time_Stamp tsc0 = get_time() + 10000;  // Adding activation offset directly

    cout << "Creating threads..." << endl;
    init_thread<0>(10000);
    cout << "Done" << endl;

    cout << "Current time: " << us(tsc0) << endl;

    std::thread freq(freq_control);  // Basic std::thread is fine for non-RT monitoring task

    cout << "Starting tasks..." << endl;
    pthread_barrier_wait(&start_barrier);
    cout << "Done" << endl;

    // Wait for 30 seconds
    freq.join();

    // Safely wait for all threads to read the exit flag and return cleanly
    for (unsigned int i = 0; i < task_count; i++) {
        threads[i]->join();
    }

    Time_Stamp times = get_time() - tsc0;

    cout << "Returned to application main!" << endl;
    cout << "Elapsed: " << us(times) << endl;
    cout << "Threads: " << task_count << endl;

    cout << "-----------------------------------------------------" << endl;
    cout << "...............Threads Timing Behavior..............." << endl;
    cout << "-----------------------------------------------------" << endl;

    for (unsigned int i = 0; i < task_count; i++) {
        // Output formatting remains exactly the same
        cout << "Task [" << i << "]: " << reinterpret_cast<void *>(threads[i]->thread) << '\n'
             << ">   Counted iterations: " << current_iteration[i] << '\n'
             << ">   Target CPU: " << taskset.tasks[i].cpu << '\n'
             << ">   Actual CPU: " << actual_cpu[i] << endl;

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

    pthread_barrier_destroy(&start_barrier);
    free_taskset();
    delete rng;

    return 0;
}

inline Time_Stamp get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000ULL;
}

inline Microsecond us(Time_Stamp ts) { return ts; }

constexpr int calc_iter_per_job(int i) {
    return static_cast<int>(static_cast<float>(taskset.tasks[i].wcet) / taskset.tasks[i].duration);
}

constexpr int calc_jobs(int i) { return (TEST_DURATION * 1000000) / taskset.tasks[i].period; }

void init_taskset() {
    cout << "Taskset initialization..." << endl;
    for (unsigned int i = 0; i < task_count; i++) {
        switch (taskset.tasks[i].task) {
            case DISPARITY:
                t_img1[i] = new signed char[sizeof(img1)];
                t_img2[i] = new signed char[sizeof(img2)];
                for (unsigned int j = 0; j < sizeof(img1); j++) t_img1[i][j] = img1[j];
                for (unsigned int j = 0; j < sizeof(img2); j++) t_img2[i][j] = img2[j];
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
        cout << ">  Deleting thread ID " << i << endl;
        delete threads[i];
    }
    cout << "Done." << endl;
}

template <unsigned int ID>
void run_func() {
    Time_Stamp init;

    unsigned int my_iter_per_job = calc_iter_per_job(ID);
    volatile unsigned int ret;

    if ((current_iteration[ID] + 1) % ITERATION_CHANGE_BEHAVIOR == 0) {
        behavior[ID] = !behavior[ID];
    }

    if (MEASURE_TIME) init = get_time();

    if (current_iteration[ID] == 0) {
        unsigned cpu_id;
        syscall(SYS_getcpu, &cpu_id, nullptr, nullptr);
        actual_cpu[ID] = cpu_id;
    }

    for (unsigned int iterations = 0; iterations < my_iter_per_job; iterations++) {
        switch (taskset.tasks[ID].task) {
            case BANDWIDTH_HEAVY:
                ret += configurable_bench_write(ID, L2_CACHE_SIZE);
                break;
            case CPU_HUNGRY:
                ret += cpu_hungry();
                break;
            case DISPARITY:
                ret += disparity(ID);
                break;
            case BANDWIDTH_LIGHT:
                ret += configurable_bench_write(ID, L1_CACHE_SIZE);
                break;
            case BANDWIDTH_MIXED:
                if (behavior[ID]) {
                    ret += configurable_bench_write(ID, L2_CACHE_SIZE);
                } else {
                    ret += configurable_bench_write(ID, L1_CACHE_SIZE);
                }
                break;
            case BANDWIDTH_RANDOM:
            default:
                ret += configurable_bench_write(ID, 0, true);
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

unsigned int configurable_bench_write(unsigned int id, unsigned int mem_size, bool randomicity) {
    unsigned long long limit = static_cast<unsigned long long>(mem_size) / sizeof(unsigned long long);
    unsigned long long increment = static_cast<unsigned long long>(CACHE_LINE_SIZE) / sizeof(unsigned long long);

    if (randomicity) {
        unsigned long long address = 0;
        mem_size = rng->random() % 2 ? L2_CACHE_SIZE : L1_CACHE_SIZE;
        limit = static_cast<unsigned long long>(mem_size) / sizeof(unsigned long long);

        for (unsigned long long i = 0; i < limit; i += increment) {
            address = ((static_cast<unsigned long long>(rng->random()) % limit) * i) % limit;
            bandwidth_array[id][address] += i;
        }
    } else {
        for (volatile unsigned long long i = 0; i < limit; i += increment) {
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

    retDisparity = getDisparity(imleft, imright, WIN_SZ, SHIFT, &disparity_allocs[id]);
    int height = retDisparity->height;

    return height;
}
