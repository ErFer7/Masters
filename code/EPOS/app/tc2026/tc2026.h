#pragma once

#include <architecture/tsc.h>
#include <clerk.h>
#include <process.h>
#include <real-time.h>
#include <utility/convert.h>
#include <utility/random.h>

#include "benchmarks.h"
#ifdef __visionfive2__
#include "machine/riscv/visionfive2/visionfive2_temperature_sensor.h"
#endif
#include "tasksets.h"

using namespace EPOS;

OStream cout;

class BenchmarkRunner {
    typedef TSC::Time_Stamp Time_Stamp;

   private:
    static const unsigned int TEST_DURATION = Traits<Build>::EXPECTED_SIMULATION_TIME - 15;  // in seconds
    static const unsigned int SELECTED_TASKSET = 1;
    static const unsigned int SEED = 20260610;
    static const unsigned int EPOS_MONITOR_ENABLED = true;

    inline static constexpr Taskset taskset = TASKSETS[SELECTED_TASKSET - 1];
    inline static constexpr unsigned int task_count = taskset.size;

   public:
    inline static void init() {
        cout << "TC 2026 Research Experiment Benchmark\n"
             << "Running experiments with the following configurations:\n"
             << ">  Test duration: " << TEST_DURATION << '\n'
             << ">  RNG Seed: " << SEED << '\n'
             << ">  Selected taskset: " << SELECTED_TASKSET << '\n'
             << ">  CPU Clock: " << CPU::clock() / 1000000 << "MHz" << '\n'
#ifdef __visionfive2__
             << ">  CPU voltage: " << PMIC::cpu_voltage() << "mV" << '\n'
             << ">  DDR Clock: " << Clock_Tree::ddr_clock() / 1000000 << "MHz"
#endif
             << endl;

        Monitor::print_monitor_info();

        _rand->seed(SEED);
        _init_taskset<0>();
    }

    inline static void free() {
        cout << "Taskset deallocation..." << endl;
        _free_taskset<0>();
        cout << "Done." << endl;
    }

    // TODO: Remember the resume on RT!
    inline static void run() {
        Time_Stamp tsc0 = _get_time() + Convert::us2count<Time_Stamp, Time_Base>(TSC::frequency(), 10000);

        if (EPOS_MONITOR_ENABLED) {
            Monitor::enable_captures();
        }

        cout << "Creating threads..." << endl;
        _init_thread<0>(10000);
        cout << "Done" << endl;

        cout << "Starting threads..." << endl;
        for (unsigned int i = 0; i < task_count; i++) {
            _threads[i]->resume();
        }
        cout << "Done" << endl;

        cout << "Current time: " << _us(tsc0) << endl;

        _logger = new Thread(Thread::Configuration(Thread::READY, Thread::Criterion(1000000, 1000000, 10000, 0)),
                             &_log_status);

        for (unsigned int i = 0; i < task_count; i++) {
            _threads[i]->join();
            cout << "Joined task [" << i << ']' << endl;
        }

        _logger->join();
        cout << "Joined logger" << endl;

        if (EPOS_MONITOR_ENABLED) {
            Monitor::disable_captures();
        }

        Time_Stamp times = _get_time() - tsc0;

        cout << "Returned to application main!" << endl;
        cout << "Elapsed: " << _us(times) << endl;
        cout << "Threads: " << task_count << endl;
    }

    inline static void print_thread_timing() {
        cout << "-----------------------------------------------------" << endl;
        cout << "...............Threads Timing Behavior..............." << endl;
        cout << "-----------------------------------------------------" << endl;

        for (unsigned int i = 0; i < task_count; i++) {
            cout << "Task [" << i << "]: " << _threads[i] << " - " << benchmark_name(taskset.tasks[i].task) << '\n'
                 << ">   Counted iterations: " << _current_iteration[i] << endl;

            const int total_jobs = _calc_jobs(i);
            const int total_iterations = total_jobs * _calc_iter_per_job(i);

            cout << ">   Execution time: " << _us(_task_runtime[i]) << " us\n"
                 << ">   Job WCET: " << _us(_job_wcet[i]) << " us\n"
                 << ">   Iteration WCET: " << _us(_iteration_wcet[i]) << " us\n"
                 << ">   Average job runtime: " << _us(_task_runtime[i] / total_jobs) << " us\n"
                 << ">   Average iteration runtime: " << _us(_task_runtime[i] / total_iterations) << " us\n"
                 << ">   Iterations per job: " << _calc_iter_per_job(i) << endl;
        }
    }

   private:
    static constexpr int _calc_iter_per_job(int i) {
        int iterations = static_cast<int>(static_cast<float>(taskset.tasks[i].wcet) / taskset.tasks[i].duration);
        if (iterations < 1) iterations = 1;

        return iterations;
    }

    static constexpr int _calc_jobs(int i) { return (TEST_DURATION * 1000000) / taskset.tasks[i].period; }

    inline static Time_Stamp _get_time() { return TSC::time_stamp(); }

    inline static Microsecond _us(Time_Stamp ts) {
        return Convert::count2us<Hertz, Time_Stamp, Time_Base>(TSC::frequency(), ts);
    }

    template <int ID>
    inline static void _init_taskset() {
        constexpr BenchmarkType benchmark_type = taskset.tasks[ID].task;

        auto *benchmark = BenchmarkTraits<benchmark_type>::create();
        _benchmarks[ID] = benchmark;

        _task_runtime[ID] = 0;
        _job_wcet[ID] = 0;
        _iteration_wcet[ID] = 0;
        _current_iteration[ID] = 0;
        _return[ID] = 0;

        _init_taskset<ID + 1>();
    }

    template <int ID>
    inline static void _free_taskset() {
        constexpr auto task = taskset.tasks[ID].task;
        using Benchmark = typename BenchmarkTraits<task>::Type;
        Benchmark *benchmark = static_cast<Benchmark *>(_benchmarks[ID]);

        delete benchmark;

        cout << ">  Deleting thread " << _threads[ID] << endl;

        delete _threads[ID];

        _free_taskset<ID + 1>();
    }

    template <unsigned int ID>
    inline static void _init_thread(Microsecond activation) {
        constexpr int job = _calc_jobs(ID);

        _threads[ID] = new RT_Thread(&_run_func<ID>,
                                     taskset.tasks[ID].period,
                                     taskset.tasks[ID].deadline,
                                     taskset.tasks[ID].wcet,
                                     activation,
                                     job,
                                     taskset.tasks[ID].cpu);

        cout << ">  Thread[" << ID << "]<" << _threads[ID] << ">(" << benchmark_name(taskset.tasks[ID].task)
             << "):\n   Period = " << taskset.tasks[ID].period << ", deadline = " << taskset.tasks[ID].deadline
             << ", wcet = " << taskset.tasks[ID].wcet << ", activation = " << activation << ", times = " << job
             << ", cpu = " << taskset.tasks[ID].cpu << endl;

        _init_thread<ID + 1>(activation);
    }

    static int _log_status() {
        for (unsigned int i = 0; i < TEST_DURATION; i++) {
            cout << ">  Iteration [" << i << ']'
#ifdef __visionfive2__
                 << ", Clock: " << Clock_Tree::cpu_clock() << "Hz"
                 << ", Temperature: " << static_cast<float>(Temperature_Sensor::get_temperature()) / 1000.0f << 'C'
#endif
                 << endl;

            for (unsigned int i = 0; i < task_count; i++) {
                cout << "   > Task [" << i << "], IWCET: " << _iteration_wcet[i] << "us; return -> " << _return[i]
                     << endl;
            }

            Delay(1000000);
        }

        return 0;
    }

    template <unsigned int ID>
    static void _run_func() {
        constexpr auto task = taskset.tasks[ID].task;
        using Benchmark = typename BenchmarkTraits<task>::Type;
        Benchmark *benchmark = static_cast<Benchmark *>(_benchmarks[ID]);

        unsigned int my_iter_per_job = _calc_iter_per_job(ID);

        Time_Stamp init = _get_time();

        for (unsigned int iterations = 0; iterations < my_iter_per_job; iterations++) {
            _return[ID] = benchmark->run();
        }

        Time_Stamp job_runtime = _get_time() - init;
        _task_runtime[ID] += job_runtime;

        _current_iteration[ID]++;

        Time_Stamp average_iteration_runtime = job_runtime / my_iter_per_job;

        if (average_iteration_runtime > _iteration_wcet[ID]) {
            _iteration_wcet[ID] = average_iteration_runtime;
        }

        if (job_runtime > _job_wcet[ID]) {
            _job_wcet[ID] = job_runtime;
        }
    }

   private:
    inline static Time_Stamp _task_runtime[task_count];
    inline static Time_Stamp _job_wcet[task_count];
    inline static Time_Stamp _iteration_wcet[task_count];

    inline static Random *_rand;
    inline static unsigned int _current_iteration[task_count];
    inline static Thread *_threads[task_count];
    inline static Thread *_logger;
    inline static void *_benchmarks[task_count];
    inline static int _return[task_count];
};

template <>
inline void BenchmarkRunner::_init_taskset<BenchmarkRunner::task_count>() {}

template <>
inline void BenchmarkRunner::_init_thread<BenchmarkRunner::task_count>(Microsecond activation) {}

template <>
inline void BenchmarkRunner::_free_taskset<BenchmarkRunner::task_count>() {}
