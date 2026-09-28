// EPOS Scheduler Component Declarations

#ifndef __scheduler_h
#define __scheduler_h

#include <architecture/cpu.h>
#include <architecture/pmu.h>
#include <architecture/tsc.h>
#include <utility/scheduling.h>
#include <utility/math.h>
#include <utility/convert.h>
#include <utility/fann.h>
#include "process.h"
#include "system/config.h"

__BEGIN_SYS

// All scheduling criteria, or disciplines, must define operator int() with
// the semantics of returning the desired order of a given object within the
// scheduling list
class Scheduling_Criterion_Common
{
    friend class Thread;                // for handle()
    friend class Periodic_Thread;       // for handle()
    friend class RT_Thread;             // for handle()
    friend class Clerk<System>;         // for _statistics

protected:
    typedef IF<Traits<TSC>::enabled, TSC_Common::Time_Stamp, Timer_Common::Tick>::Result Tick;

public:
    // Priorities
    enum : int {
        CEILING = -1000,
        MAIN    = -1,
        HIGH    = 0,
        NORMAL  = (unsigned(1) << (sizeof(int) * 8 - 3)) - 1,
        LOW     = (unsigned(1) << (sizeof(int) * 8 - 2)) - 1,
        IDLE    = (unsigned(1) << (sizeof(int) * 8 - 1)) - 1
    };

    // Constructor helpers
    enum : unsigned int {
        SAME        = 0,
        NOW         = 0,
        UNKNOWN     = 0,
        ANY         = -1U
    };

    // Policy types
    enum : int {
        PERIODIC    = HIGH,
        SPORADIC    = NORMAL,
        APERIODIC   = LOW
    };

    // Policy events
    typedef int Event;
    enum {
        CREATION             = 1 << 0,
        FINISH               = 1 << 1,
        DISPATCH             = 1 << 2,
        RETREAT              = 1 << 3,
        JOB_RELEASE          = 1 << 4,
        JOB_FINISH           = 1 << 5,
        JOB_EXECUTION_START  = 1 << 6,
        JOB_EXECUTION_FINISH = 1 << 7,
        JOB_ALARM_HANDLE     = 1 << 8,
        TIMER_INTERRUPTION   = 1 << 9,
    };

    // Policy operations
    typedef int Operation;
    enum {
        COLLECT         = 1 << 16,
        CHARGE          = 1 << 17,
        AWARD           = 1 << 18,
        UPDATE          = 1 << 19,
        REBALANCE       = 1 << 20,
        HALT            = 1 << 21
    };

    // INFO: Temporary job status
    typedef int JobStatus;
    enum {
        UNINITIALIZED = 1 << 0,
        RUNNING = 1 << 1,
        FINISHED = 1 << 2
    };

    // Policy traits
    static const bool timed = false;
    static const bool dynamic = false;
    static const bool preemptive = true;

    // Multicore algorithms
    static const unsigned int QUEUES = 1;
    static const unsigned int HEADS = 1;

    // Runtime Statistics (for policies that don't use any; that's why its a union)
    union Dummy_Statistics {  // for Traits<System>::monitored = false
        // Thread related statistics
        Tick thread_creation;                   // tick in which the thread was created
        Tick thread_destruction;                // tick in which the thread was destroyed
        Tick thread_execution_time;             // accumulated execution time (in ticks)
        Tick thread_last_dispatch;              // tick in which the thread was last dispatched to the CPU
        Tick thread_last_preemption;            // tick in which the thread left the CPU by the last time

        // Job related statistics
        // INFO: Temporary metrics used until the overall RT metrics are fixed
        Tick job_last_preemption;            // tick in which the thread left the CPU by the last time
        Tick job_execution_start;                         // tick in which the last job of a periodic thread started (different from "thread_last_dispatch" since jobs can be preempted)
        Tick job_execution_block_initial_time;
        Tick job_partial_utilization;
        unsigned int deadline_misses;

        Tick job_release;                       // tick in which the last job of a periodic thread was made ready for execution
        Tick job_start;                         // tick in which the last job of a periodic thread started (different from "thread_last_dispatch" since jobs can be preempted)
        Tick job_finish;                        // tick in which the last job of a periodic thread finished (i.e. called _alarm->p() at wait_netxt(); different from "thread_last_preemption" since jobs can be preempted)
        Tick job_utilization;                   // accumulated execution time (in ticks)
        unsigned int jobs_released;             // number of jobs of a thread that were released so far (i.e. the number of times _alarm->v() was called by the Alarm::handler())
        unsigned int jobs_finished;             // number of jobs of a thread that finished execution so far (i.e. the number of times alarm->p() was called at wait_next())

        static Tick cpu_time[Traits<Build>::CPUS];
    };

    struct Real_Statistics {  // for Traits<System>::monitored = true
        // Thread related statistics
        Tick thread_creation;                   // tick in which the thread was created
        Tick thread_destruction;                // tick in which the thread was destroyed
        Tick thread_execution_time;             // accumulated execution time (in ticks)
        Tick thread_last_dispatch;              // tick in which the thread was last dispatched to the CPU
        Tick thread_last_preemption;            // tick in which the thread left the CPU by the last time

        // Job related statistics
        // INFO: Temporary metrics used until the overall RT metrics are fixed
        Tick job_last_preemption;            // tick in which the thread left the CPU by the last time
        Tick job_execution_start;                         // tick in which the last job of a periodic thread started (different from "thread_last_dispatch" since jobs can be preempted)
        Tick job_execution_block_initial_time;
        Tick job_partial_utilization;
        unsigned int deadline_misses;

        Tick job_release;                       // tick in which the last job of a periodic thread was made ready for execution
        Tick job_start;                         // tick in which the last job of a periodic thread started (different from "thread_last_dispatch" since jobs can be preempted)
        Tick job_finish;                        // tick in which the last job of a periodic thread finished (i.e. called _alarm->p() at wait_netxt(); different from "thread_last_preemption" since jobs can be preempted)
        Tick job_utilization;                   // accumulated execution time (in ticks)
        unsigned int jobs_released;             // number of jobs of a thread that were released so far (i.e. the number of times _alarm->v() was called by the Alarm::handler())
        unsigned int jobs_finished;             // number of jobs of a thread that finished execution so far (i.e. the number of times alarm->p() was called at wait_next())

        static Tick cpu_time[Traits<Build>::CPUS];

        Real_Statistics():
            thread_creation(0),
            thread_destruction(0),
            thread_execution_time(0),
            thread_last_dispatch(0),
            thread_last_preemption(0),
            job_last_preemption(0),
            job_execution_start(0),
            job_execution_block_initial_time(0),
            job_partial_utilization(0),
            deadline_misses(0),
            job_release(0),
            job_start(0),
            job_finish(0),
            job_utilization(0),
            jobs_released(0),
            jobs_finished(0) {}
    };

    typedef IF<Traits<Build>::monitored, Real_Statistics, Dummy_Statistics>::Result Statistics;

protected:
    Scheduling_Criterion_Common() {}

    Scheduling_Criterion_Common(const Scheduling_Criterion_Common &scheduling_criterion_common) {}

public:
    Microsecond period() { return 0;}
    Microsecond deadline() { return 0; }
    Microsecond capacity() { return 0; }

    bool periodic() { return false; }

    volatile Statistics & statistics() { return _statistics; }

protected:
    void handle(Event event);

    unsigned int queue() const { return 0; }
    void queue(unsigned int q) {}

    static Tick now();

    static void init() {}

protected:
    Statistics _statistics;
};

// Priority (static and dynamic)
class Priority: public Scheduling_Criterion_Common
{
public:
    template <typename ... Tn>
    Priority(int i = NORMAL, Tn & ... an): _priority(i) {}

    Priority(const Priority &priority): Scheduling_Criterion_Common(priority), _priority(priority._priority) {}

    operator const volatile int() const volatile { return _priority; }

protected:
    volatile int _priority;
};

// Round-Robin
class RR: public Priority
{
public:
    static const bool timed = true;
    static const bool dynamic = false;
    static const bool preemptive = true;

public:
    template <typename ... Tn>
    RR(int i = NORMAL, Tn & ... an): Priority(i) {}

    RR(const RR &rr) : Priority(rr) {}
};

// First-Come, First-Served (FIFO)
class FCFS: public Priority
{
public:
    static const bool timed = false;
    static const bool dynamic = false;
    static const bool preemptive = false;

public:
    template <typename ... Tn>
    FCFS(int i = NORMAL, Tn & ... an);

    FCFS(const FCFS &fcfs) : Priority(fcfs) {}
};


// Real-time Algorithms
class RT_Common: public Priority
{
    friend class FCFS;
    friend class Thread;                // for handle() and queue()
    friend class Periodic_Thread;       // for handle() and queue()
    friend class RT_Thread;             // for handle() and queue()

public:
    static const bool timed = true;
    static const bool preemptive = true;

protected:
    RT_Common(int i): Priority(i), _period(0), _deadline(0), _capacity(0), _statistics(Statistics()), _job_status(UNINITIALIZED) {} // aperiodic

    RT_Common(const RT_Common &rt_common) :
        Priority(rt_common),
        _period(rt_common._period),
        _deadline(rt_common._deadline),
        _capacity(rt_common._capacity),
        _statistics(rt_common._statistics),
        _job_status(rt_common._job_status) {}

    RT_Common(int i, Microsecond p, Microsecond d, Microsecond c): Priority(i), _period(ticks(p)), _deadline(ticks(d ? d : p)), _capacity(ticks(c)) {}

public:
    Microsecond period() { return time(_period); }
    Microsecond deadline() { return time(_deadline); }
    Microsecond capacity() { return time(_capacity); }

    bool periodic() { return (_priority >= PERIODIC) && (_priority <= SPORADIC); }

    volatile Statistics & statistics() { return _statistics; }

protected:
    void handle(Event event);

    Tick ticks(Microsecond time);
    Microsecond time(Tick ticks);

protected:
    Tick _period;
    Tick _deadline;
    Tick _capacity;
    Statistics _statistics;
    JobStatus _job_status;
};

// Rate Monotonic
class RM: public RT_Common
{
public:
    static const bool dynamic = false;

public:
    RM(int p = APERIODIC): RT_Common(p) {}
    RM(Microsecond p, Microsecond d = SAME, Microsecond c = UNKNOWN): RT_Common(int(ticks(p)), p, d, c) {}

    RM(const RM &rm) : RT_Common(rm) {}
};

// Deadline Monotonic
class DM: public RT_Common
{
public:
    static const bool dynamic = false;

public:
    DM(int p = APERIODIC): RT_Common(p) {}
    DM(Microsecond p, Microsecond d = SAME, Microsecond c = UNKNOWN): RT_Common(int(ticks(d ? d : p)), p, d, c) {}

    DM(const DM &dm) : RT_Common(dm) {}
};

// Laxity Monotonic
class LM: public RT_Common
{
public:
    static const bool dynamic = false;

public:
    LM(int p = APERIODIC): RT_Common(p) {}
    LM(Microsecond p, Microsecond d, Microsecond c): RT_Common(int(ticks((d ? d : p) - c)), p, d, c) {}

    LM(const LM &lm) : RT_Common(lm) {}
};

// Earliest Deadline First
class EDF: public RT_Common
{
public:
    static const bool dynamic = true;

public:
    EDF(int p = APERIODIC): RT_Common(p) {}

    EDF(const EDF &edf) : RT_Common(edf) {}

    EDF(Microsecond p, Microsecond d = SAME, Microsecond c = UNKNOWN);

    void handle(Event event);
};

// Least Laxity First
class LLF: public RT_Common
{
public:
    static const bool dynamic = true;

public:
    LLF(int p = APERIODIC): RT_Common(p) {}
    LLF(Microsecond p, Microsecond d = SAME, Microsecond c = UNKNOWN);

    LLF(const LLF &llf) : RT_Common(llf) {}

    void handle(Event event);
};


// Multicore Algorithms
class Variable_Queue_Scheduler
{
protected:
    Variable_Queue_Scheduler(unsigned int queue): _queue(queue) {};

    Variable_Queue_Scheduler(const Variable_Queue_Scheduler &variable_queue_scheduler) : _queue(variable_queue_scheduler._queue) {}

    unsigned int queue() const volatile { return _queue; }
    void queue(unsigned int q) { _queue = q; }

protected:
    volatile unsigned int _queue;
    static volatile unsigned int _next_queue;
};


// Global Round-Robin
class GRR: public RR
{
public:
    static const unsigned int HEADS = Traits<Machine>::CPUS;

public:
    template <typename ... Tn>
    GRR(int p = NORMAL, Tn & ... an): RR(p) {}

    GRR(const GRR &grr) : RR(grr) {}

    static unsigned int current_head() { return CPU::id(); }
};

// Fixed CPU (fully partitioned)
class Fixed_CPU: public Priority, public Variable_Queue_Scheduler
{
public:
    static const bool dynamic = false;

    static const unsigned int QUEUES = Traits<Machine>::CPUS;

public:
    template <typename ... Tn>
    Fixed_CPU(int p = NORMAL, unsigned int cpu = ANY, Tn & ... an)
    : Priority(p), Variable_Queue_Scheduler(((_priority == IDLE) || (_priority == MAIN)) ? CPU::id() : (cpu != ANY) ? cpu : ++_next_queue %= CPU::cores()) {}

    Fixed_CPU(const Fixed_CPU &fixed_cpu) : Priority(fixed_cpu), Variable_Queue_Scheduler(fixed_cpu) {}

    using Variable_Queue_Scheduler::queue;
    static unsigned int current_queue() { return CPU::id(); }
};

// CPU Affinity
class CPU_Affinity: public Priority, public Variable_Queue_Scheduler
{
public:
    static const bool dynamic = false;
    static const unsigned int QUEUES = Traits<Machine>::CPUS;

public:
    template <typename ... Tn>
    CPU_Affinity(int p = NORMAL, unsigned int cpu = ANY, Tn & ... an)
    : Priority(p), Variable_Queue_Scheduler(((_priority == IDLE) || (_priority == MAIN)) ? CPU::id() : (cpu != ANY) ? cpu : ++_next_queue %= CPU::cores()) {}

    CPU_Affinity(const CPU_Affinity &cpu_affinity) : Priority(cpu_affinity), Variable_Queue_Scheduler(cpu_affinity) {}

    void handle(Event event);

    using Variable_Queue_Scheduler::queue;
    static unsigned int current_queue() { return CPU::id(); }

private:
    Statistics _statistics;
};


/// Partitioned Rate Monotonic (multicore)
class PRM: public RM, public Variable_Queue_Scheduler
{
public:
    static const unsigned int QUEUES = Traits<Machine>::CPUS;

public:
    PRM(int p = APERIODIC)
    : RM(p), Variable_Queue_Scheduler(((_priority == IDLE) || (_priority == MAIN)) ? CPU::id() : 0) {}

    PRM(const Microsecond & d, const Microsecond & p = SAME, const Microsecond & c = UNKNOWN, unsigned int cpu = ANY)
    : RM(d, p, c), Variable_Queue_Scheduler((cpu != ANY) ? cpu : ++_next_queue %= CPU::cores()) {}

    PRM(const PRM &prm) : Variable_Queue_Scheduler(prm) {}

    using Variable_Queue_Scheduler::queue;
    static unsigned int current_queue() { return CPU::id(); }
};

// Global Earliest Deadline First (multicore)
class GEDF: public EDF
{
public:
    static const unsigned int HEADS = Traits<Machine>::CPUS;

public:
    GEDF(int p = APERIODIC): EDF(p) {}
    GEDF(const Microsecond & d, const Microsecond & p = SAME, const Microsecond & c = UNKNOWN, unsigned int cpu = ANY)
    : EDF(d, p, c) {}

    GEDF(const GEDF &gedf) : EDF(gedf) {}

    unsigned int queue() const { return current_head(); }
    void queue(unsigned int q) {}
    static unsigned int current_head() { return CPU::id(); }
};

// Partitioned Earliest Deadline First (multicore)
class PEDF: public EDF, public Variable_Queue_Scheduler
{
public:
    static const unsigned int QUEUES = Traits<Machine>::CPUS;

public:
    PEDF(int p = APERIODIC)
    : EDF(p), Variable_Queue_Scheduler(((_priority == IDLE) || (_priority == MAIN)) ? CPU::id() : 0) {}

    PEDF(const PEDF &pedf): EDF(pedf), Variable_Queue_Scheduler(pedf) {}

    PEDF(const Microsecond & d, const Microsecond & p = SAME, const Microsecond & c = UNKNOWN, unsigned int cpu = ANY)
    : EDF(d, p, c), Variable_Queue_Scheduler((cpu != ANY) ? cpu : ++_next_queue %= CPU::cores()) {}

    using Variable_Queue_Scheduler::queue;
    static unsigned int current_queue() { return CPU::id(); }
};

#ifdef __PMU_H
// NOTE: Experimental Monitored PEDF scheduler that will eventually be the EA_PEDF
class EA_PEDF_RV64: public PEDF
{
    friend class Thread;
    typedef TSC::Time_Stamp Time_Stamp;
    typedef Timer::Tick Tick;
    typedef Simple_List<Thread> Thread_List;

    typedef int LogMode;
    enum {
        RAW,
        RATE,
    };

    static const unsigned long TIME_SPAN = (Traits<Build>::EXPECTED_SIMULATION_TIME != 0) ? Traits<Build>::EXPECTED_SIMULATION_TIME : Traits<System>::LIFE_SPAN;
    static const unsigned long ALLOCATION_SAFETY_MARGIN = 100;

    static const unsigned int PERIODIC_SAMPLING_FREQUENCY = 40;
    static const unsigned long long PERIODIC_SAMPLING_PERIOD = 1000000 / PERIODIC_SAMPLING_FREQUENCY;

    static const bool PREDICT = false;
    static const bool TRAIN = false;
    static const bool BALANCE = false;
    static const bool TRAIN_ACTIVITY_WEIGHTS = false;
    static const bool SAFE = false;

    static constexpr float MIN_FREQUENCY = 375999999.0f;
    static constexpr float MAX_FREQUENCY = 1503999999.0f;

    static const unsigned int MIN_FREQUENCY_LEVEL = 0;

    static const int L2_PMU_EVENT_START = 61;

    static const unsigned long long COUNTER_MAX = -1ULL;

    static const unsigned int MAX_ANN_DATASET_BUFFER_SIZE = 16;
    static constexpr float ANN_DEVIATION_THRESHOLD = 0.02;
    static constexpr float SAFETY_MARGIN = 0.05;
    static const unsigned int MAX_TRAINS = 8;

    // ANN config
    static const unsigned int INPUTS = 12;
    static const unsigned int PMU_INPUTS = 10;

    static constexpr System_Event SYSTEM_EVENTS[] = { JOB_UTILIZATION, CPU_CLOCK, DEADLINE_MISSES, CORE };

    static constexpr LogMode SYSTEM_EVENTS_MODE[] = { RAW, RAW, RAW, RAW };

    static constexpr PMU_Event PMU_EVENTS[] = { CPU_CYCLES,
                                                INSTRUCTIONS_RETIRED,

                                                L1_CACHE_MISSES,
                                                BRANCH_DIRECTION_MISPREDICTIONS,

                                                ARCHITECTURE_DEPENDENT_EVENT86,   // L2_INNER_ACQUIREBLOCK_HIT_L2
                                                ARCHITECTURE_DEPENDENT_EVENT76,   // L2_INNER_RELEASEDATA_TTON
                                                ARCHITECTURE_DEPENDENT_EVENT115,  // L2_CACHE_MISS
                                                ARCHITECTURE_DEPENDENT_EVENT114,  // L2_DEMAND_MISS_HIT_MSHR_ALLOC_HINT
                                                ARCHITECTURE_DEPENDENT_EVENT113   // L2_INNER_PROBEBLOCK_TON_STORE_MISS
                                                };

    static constexpr unsigned int PMU_EVENTS_COUNT = COUNTOF(PMU_EVENTS);

    static constexpr LogMode PMU_EVENTS_MODE[] = { RATE,
                                                   RATE,
                                                   RATE,
                                                   RATE,
                                                   RATE,
                                                   RATE,
                                                   RATE,
                                                   RATE,
                                                   RATE,
                                                   };

    static const int IGNORED_INPUT = -1;

    static constexpr int SYSTEM_ANN_INPUT_MAP[] = { 0, 1, IGNORED_INPUT, IGNORED_INPUT };

    static constexpr int PMU_ANN_INPUT_MAP[] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

    static constexpr float IGNORED_COUNTER = -1.0f;

    static constexpr float PMU_INITIAL_MIN[] = {
        166835028.0f,
        18665160.0f,
        665.0f,
        55.0f,
        1871375.0f,
        1192910.0f,
        2061.0f,
        3483.0f,
        0.0f,
        957.0f
    };

    static constexpr float PMU_INITIAL_MAX[] = {
        500293481.0f,
        325733612.0f,
        3271570.0f,
        220655.0f,
        6732361.0f,
        3374726.0f,
        1281490.0f,
        271980.0f,
        37.0f,
        40711.0f
    };

    static constexpr float MIGRATION_OPTIMIZATION_THRESHOLD_INCREASE_STEP = 0.15f;
    static constexpr float MIGRATION_OPTIMIZATION_SOFT_THRESHOLD_INCREASE_STEP = 0.05f;
    static constexpr float ACTIVITY_TRAINING_LEARNING_RATE = 0.7f;
    static constexpr float MIGRATION_MIN_ENHANCEMENT_THRESHOLD = 0.05f;

    template<typename Type>
    struct Data {
        Time_Stamp timestamp;
        Type value;
    };

    template<typename Type>
    struct Data_Buffer {
        Event event;
        LogMode log_mode;
        unsigned int max_size;
        unsigned int captures;
        unsigned long long last_value;
        unsigned long long partial_diff;
        int ann_input_index;
        Data<Type> *buffer;

        Data_Buffer():
            event(0),
            log_mode(0),
            max_size(0),
            captures(0),
            last_value(0),
            partial_diff(0),
            ann_input_index(0),
            buffer(nullptr) {}

        Data_Buffer(const Event & event, const LogMode & log_mode, const int & ann_input_index, const unsigned int & max_buffer_size):
            event(event),
            log_mode(log_mode),
            max_size(max_buffer_size),
            captures(0),
            last_value(0),
            partial_diff(0),
            ann_input_index(ann_input_index),
            buffer(new (SYSTEM) Data<Type>[max_buffer_size]) {
        }

        inline void read_start(const Type & value) {
            if (log_mode == RATE) {
                last_value = value;
                partial_diff = 0;
            }
        }

        inline void read_retreat(const Type & value) {
            if (log_mode == RATE) {
                unsigned long long value_diff = value >= last_value ? value - last_value : (COUNTER_MAX - last_value) + value + 1;
                partial_diff += value_diff;
            }
        }

        inline void read_dispatch(const Type & value) {
            if (log_mode == RATE) {
                last_value = value;
            }
        }

        inline void read_finish(const Time_Stamp & timestamp, const Type & value) {
            if (captures >= max_size) {
                return;
            }

            Data<Type> *data = &buffer[captures++];

            Type prev_value = log_mode == RATE ? last_value : 0;
            Type value_diff = value >= prev_value ? value - prev_value : (COUNTER_MAX - prev_value) + value + 1;

            data->timestamp = timestamp;
            data->value = value_diff + (log_mode == RATE ? partial_diff : 0);
        }

        inline unsigned long long last_data_value() {
            return buffer[captures - 1].value;
        }

        inline float accumulated_collection(const Tick & period) {
            float value_acc = 0.0f;

            if (captures == 0) return value_acc;

            unsigned int collection_count = _hyperperiod / period;
            int collection_end = collection_count < captures ? static_cast<int>(captures - collection_count) : 0;

            for (int i = static_cast<int>(captures - 1); i >= collection_end; i--) {
                value_acc += static_cast<float>(buffer[i].value);
            }

            return value_acc;
        }
    };

    struct ANN_Dataset {
        int start;
        int end;
        unsigned int size;
        float inputs[MAX_ANN_DATASET_BUFFER_SIZE][INPUTS];
        unsigned int hardware_levels[MAX_ANN_DATASET_BUFFER_SIZE];
        EA_PEDF_RV64 * criteria[MAX_ANN_DATASET_BUFFER_SIZE];

        ANN_Dataset(): start(0), end(0), size(0) {}

        inline void push(const float (&values)[INPUTS], unsigned int hardware_level, EA_PEDF_RV64 * criterion) {
            for (unsigned int i = 0; i < INPUTS; i++) {
                inputs[end][i] = values[i];
            }

            hardware_levels[end] = hardware_level;
            criteria[end] = criterion;
            end = next_index(end);

            if (size == MAX_ANN_DATASET_BUFFER_SIZE) {
                start = next_index(start);
            } else {
                size++;
            }
        }

        inline unsigned int first_index() {
            return start;
        }

        inline unsigned int last_index() {
            return prev_index(end);
        }

        static inline int next_index(int i) {
            return (i + 1) % MAX_ANN_DATASET_BUFFER_SIZE;
        }

        static inline int prev_index(int i) {
            return (i - 1 < 0) ? MAX_ANN_DATASET_BUFFER_SIZE - 1 : i - 1;
        }

        inline float output(int i) {
            if (hardware_levels[i] == 0) return 1.0f;

            return criteria[i]->_ann_expected_output[hardware_levels[i] - 1].get();
        }
    };

    template<typename Type>
    struct Average_Value {
        Type sum;
        Type count;
        Type average;

        Average_Value(): sum(0), count(0), average(0) {}

        inline void push(const Type & value) {
            sum += value;
            count++;
            average = sum / count;
        }

        inline Type get() {
            return average;
        }
    };

    struct Variance {
        float average;
        unsigned int count;
        float square_sum;

        Variance(): average(0.0f), count(0), square_sum(0.0f) {}

        inline void push(const float & value) {
            count++;
            float old_average = average;
            average += (value - average) / count;
            square_sum += (value - old_average) * (value - average);
        }

        inline float get() {
            return square_sum / count;
        }

        inline void reset() {
            average = 0.0f;
            count = 0;
            square_sum = 0.0f;
        }
    };

    struct Activity {
        Activity() { reset(); }

        Activity(const Activity &a) {
            for(unsigned int i = 0; i < PMU_INPUTS; i++)
                vector[i] = a.vector[i];
        }

        inline void reset() {
            for(unsigned int i = 0; i < PMU_INPUTS; i++)
                vector[i] = 0;
        }

        inline float & operator[](unsigned int index) {
            return vector[index];
        }

        inline void operator=(const Activity & a) {
            for(unsigned int i = 0; i < PMU_INPUTS; i++)
                vector[i] = a.vector[i];
        }

        float vector[PMU_INPUTS];
    };

public:
    EA_PEDF_RV64(int p = APERIODIC): PEDF(p), _monitored(false) {}

    EA_PEDF_RV64(const EA_PEDF_RV64 &criterion): PEDF(criterion), _monitored(criterion._monitored), _target_migration_cpu(criterion._target_migration_cpu) {
        if (_monitored) {
            for (unsigned int i = 0; i < _system_buffer_count; i++) {
                _system_buffers[i] = criterion._system_buffers[i];
            }

            for (unsigned int i = 0; i < PMU_EVENTS_COUNT; i++) {
                _pmu_buffers[i] = criterion._pmu_buffers[i];
                _last_periodic_pmu_sample[i] = criterion._last_periodic_pmu_sample[i];
                _job_pmu_variance[i] = criterion._job_pmu_variance[i];
                _pmu_variance_buffer[i] = criterion._pmu_variance_buffer[i];
            }

            _l1_by_l2_buffer = criterion._l1_by_l2_buffer;
            _thread = criterion._thread;
            _first_periodic_sample = criterion._first_periodic_sample;

            if (PREDICT) {
                _predicted_utilization_buffer = criterion._predicted_utilization_buffer;

                for (unsigned int i = 0; i < INPUTS; i++) {
                    _ann_input[i] = criterion._ann_input[i];
                }

                _ann_output = criterion._ann_output;

                if (TRAIN) {
                    for (unsigned int i = 0; i < 4; i++) {
                        _ann_expected_output[i] = criterion._ann_expected_output[i];
                    }
                }

                if (BALANCE) {
                    _activity_sum = criterion._activity_sum;

                    for (unsigned int i = 0; i < Traits<Build>::CPUS; i++) {
                        _migration_locked[i] = criterion._migration_locked[i];
                    }
                }
            }
        }
    }

    EA_PEDF_RV64(const Microsecond & d, const Microsecond & p = SAME, const Microsecond & c = UNKNOWN, unsigned int cpu = ANY, bool monitored = false, Thread *thread = nullptr);

    using PEDF::queue;

    inline void target_migration_cpu(int target_cpu) {
        _target_migration_cpu = target_cpu;
    }

    inline int target_migration_cpu() {
        return _target_migration_cpu;
    }

    static void init();

    inline static void enable() {
        kout << "EA_PEDF_RV64::enable()" << endl;

        _enabled = true;
        reset_hyperperiod(1);
        _hyperperiod += 500;
    }

    inline static void disable() {
        kout << "EA_PEDF_RV64::disable()" << endl;

        _enabled = false;
    }

    void handle(Event event);

    void print_data();

private:
    template<unsigned int CHANNEL>
    static void init_system_monitoring();

    template<unsigned int CHANNEL>
    static void init_pmu_monitoring();

    template<unsigned int CHANNEL>
    void allocate_system_buffers(const unsigned int max_buffer_size);

    template<unsigned int CHANNEL>
    void allocate_pmu_buffers(const unsigned int max_buffer_size);

    static inline Microsecond count2us(Time_Stamp t) {
        return Convert::count2us<Hertz, Time_Stamp, Microsecond>(TSC::frequency(), t);
    }

    static inline Tick us2count(Microsecond us) {
        return Convert::us2count<TSC::Time_Stamp, Time_Base>(TSC::frequency(), us);
    }

    static inline Microsecond now() {
        return count2us(TSC::time_stamp());
    }

    static inline bool reached_hyperperiod() {
        return TSC::time_stamp() > _next_hyperperiod;
    }

    static inline void reset_hyperperiod(unsigned int ignored_hyperperiods = 0) {
        _next_hyperperiod += _hyperperiod * (ignored_hyperperiods + 1);
    }

    static inline void cooldown(unsigned int hyperperiods = 1) {
        _cooldown = hyperperiods;
    }

    void collect_periodic_data();

    void reset_periodic_data_collection();

    static void collect_data();

    static void handle_deadline_misses();

    static void train();

    static void predict();

    static void balance_load();

    static float activity_variance();

    static inline float normalize_min_max(float value, float min, float max) { return (value - min) / (max - min); }

private:
    bool _monitored;

    Data_Buffer<unsigned long long> _system_buffers[COUNTOF(SYSTEM_EVENTS)];
    Data_Buffer<unsigned long long> _pmu_buffers[PMU_EVENTS_COUNT];
    Data_Buffer<float> _l1_by_l2_buffer;

    bool _first_periodic_sample;
    unsigned long _last_periodic_pmu_sample[PMU_EVENTS_COUNT];
    Variance _job_pmu_variance[PMU_EVENTS_COUNT];
    Data_Buffer<float> _pmu_variance_buffer[PMU_EVENTS_COUNT];

    Data_Buffer<unsigned long long> _predicted_utilization_buffer;
    Thread *_thread;

    float _ann_input[INPUTS];
    float _ann_output;
    Average_Value<float> _ann_expected_output[4];

    float _activity_sum;
    bool _migration_locked[Traits<Build>::CPUS];
    int _target_migration_cpu;

    unsigned long long _last_hyperperiod_deadline_misses;

    static bool _enabled;

    static Clerk<System> *_system_clerks[Traits<Build>::CPUS][COUNTOF(SYSTEM_EVENTS)];
    static Clerk<PMU> *_pmu_clerks[Traits<Build>::CPUS][PMU_EVENTS_COUNT];
    static unsigned int _system_buffer_count;

    static unsigned long long _next_periodic_sampling;

    static unsigned long long _hyperperiod;
    static unsigned long long _next_hyperperiod;
    static float _pmu_min_values[Traits<Build>::CPUS][PMU_EVENTS_COUNT];
    static float _pmu_max_values[Traits<Build>::CPUS][PMU_EVENTS_COUNT];
    static Thread_List _thread_list;
    static struct FANN::fann *_ann[Traits<Build>::CPUS];
    static float _core_utilization[Traits<Build>::CPUS];
    static float _predicted_core_utilization[Traits<Build>::CPUS];

    static bool _decreased_frequency;  // TODO: Check if this can be safely removed
    static bool _train_ann[Traits<Build>::CPUS];
    static ANN_Dataset _ann_training_dataset[Traits<Build>::CPUS];

    static bool _balanced;
    static bool _revoke;
    static float _activity_weights[PMU_INPUTS];
    static Activity _activity_vectors[Traits<Build>::CPUS];
    static Activity _last_activity_vectors[Traits<Build>::CPUS];
    static float _activity_cpu[Traits<Build>::CPUS];
    static float _last_activity_cpu[Traits<Build>::CPUS];
    static float _last_activity_variance;
    static float _migration_optimization_threshold;
    static unsigned int _pre_migration_frequency_level;
    static bool _train_activity;
    static unsigned int _last_migration_from;
    static unsigned int _last_migration_to;
    static EA_PEDF_RV64 *_last_migrated;
    static EA_PEDF_RV64 *_last_swapped;

    static unsigned int _cooldown;

    static bool _has_missed_deadlines;
};

// Energy-Aware ANN - Partitioned Earliest Deadline First (multicore)
class EA_PEDF: public PEDF
{
    friend class _SYS::Thread;
    friend class _SYS::Periodic_Thread;
    friend class _SYS::RT_Thread;
    friend class _SYS::Clerk<System>;         // for _statistics

public:
    static const bool collecting = true;
    static const bool charging = true;
    static const bool awarding = true;
    static const bool migrating = true;
    static const bool system_wide = true;

    // PMU Monitoring
    static const unsigned int PMU_EVENTS = 6;

    // ANN
    static const unsigned int INPUTS = PMU_EVENTS + 2; // Thread Execution Time and Current CPU Frequency
    static const unsigned int MAX_TRAINS = 8;
    static const unsigned int SAMPLES_PER_TRAIN_LIMIT = 16;
    static constexpr float ANN_DEVIATION_THRESHOLD = 0.02;

    // Voting and DVFS
    static constexpr float SAFETY_MARGIN = 0.05;
    static const unsigned int FREQUENCY_LEVEL = 100 * 1000 * 1000; // 100 MHz

    // Activity Weights
    static const bool TRAIN_WEIGHTS = false;
    static constexpr float LEARNING_RATE = 0.7;

    struct Activity {
        Activity() { reset(); }

        Activity(const Activity &a) {
            for(unsigned int i = 0; i < PMU_EVENTS; ++i)
                vector[i] = a.vector[i];
            utilization = a.utilization;
        }

        void reset() {
            for(unsigned int i = 0; i < PMU_EVENTS; ++i)
                vector[i] = 0;
            utilization = 0;
        }

        void operator+=(const Activity & a) {
            for(unsigned int i = 0; i < PMU_EVENTS; ++i)
                vector[i] += a.vector[i];
            utilization += a.utilization;
        }

        void operator-=(const Activity & a) {
            for(unsigned int i = 0; i < PMU_EVENTS; ++i)
                vector[i] -= a.vector[i];
            utilization -= a.utilization;
        }

        float sum() {
            float result = 0;
            for(unsigned int i = 0; i < PMU_EVENTS; ++i)
                result += vector[i];
            return result;
        }

        bool fits(float* _vector) {
            bool result = true;
            for(unsigned int i = 0; i < PMU_EVENTS; ++i)
                result &= vector[i]+ _vector[i] < (1 * _activity_weights[i]);
            return result;
        }

        bool fits(const Activity & a) {
            bool result = true;
            for(unsigned int i = 0; i < PMU_EVENTS; ++i)
                result &= vector[i]+ a.vector[i] <= (1 * _activity_weights[i]);
            return result;// && (a.utilization + utilization < 1 - Traits<Criterion>::VARIANCE_THRESHOLDS[0]);
        }

        float vector[PMU_EVENTS];
        float utilization;
    };

    struct Statistics {
        Statistics(): thread_execution_time(0), last_thread_dispatch(0), destination_cpu(ANY), output(0) {
            for(unsigned int i = 1; i < Traits<Build>::CPUS; ++i)
                migration_locked[i] = false;

            for(unsigned int i = 0; i < PMU_EVENTS; ++i) {
                input[i] = 0;
                last_pmu_read[i] = 0;
                pmu_accumulated[i] = 0;
            }
            input[6] = 0; // Thread Execution Time
            input[7] = 0; // CPU Frequency
        }

        // Thread Execution Time
        TSC::Time_Stamp thread_execution_time;  // Sum of jobs execution time
        TSC::Time_Stamp last_thread_dispatch;   // The timestamp of the last dispatch
        // On context-switch: execution time += TSC::timestamp() - last_dispatch
        // Migration
        unsigned int destination_cpu; // to help migration
        bool migration_locked[Traits<Build>::CPUS];

        // ANN
        float input[INPUTS];
        float output;

        Activity activity;
        Activity last_activity;

        // Per-Task Monitoring PMU
        PMU::Count pmu_accumulated[PMU_EVENTS];   // accumulated PMU counters during thread execution
        PMU::Count last_pmu_read[PMU_EVENTS];     // the pmu read() when thread entered CPU

        // Dealine Miss count - Used By Clerk<System>
        Alarm * alarm_times;            // Reference to RT_Thread private alarm (for monitoring pourposes)
        unsigned int finished_jobs;     // Number of finished jobs  <=> times alarm->p() has been called for this task
        unsigned int missed_deadlines;  // Number of finished jobs (finished_jobs) - number of dispatched jobs (alarm_times->times)

        // CPU Execution Time
        static TSC::Time_Stamp _cpu_time[Traits<Build>::CPUS];              // accumulated cpu time at each hyperperiod
        static TSC::Time_Stamp _last_hyperperiod[Traits<Build>::CPUS];      // Time Stamp of last hyperperiod
        static TSC::Time_Stamp _hyperperiod;                                // Global Hyperperiod
        static TSC::Time_Stamp _hyperperiod_cpu_time[Traits<Build>::CPUS];  // cpu time in last hyperperiod

        static Activity _activity_cpu[Traits<Build>::CPUS];
        static Activity _last_activity_cpu[Traits<Build>::CPUS];
    };

public:
    EA_PEDF(int p = APERIODIC): PEDF(p) {}

    EA_PEDF(const Microsecond & d, const Microsecond & p = SAME, const Microsecond & c = UNKNOWN, unsigned int cpu = ANY)
    : PEDF(d, p, c, cpu) {
        if(Statistics::_hyperperiod == 0)
            Statistics::_hyperperiod = Convert::us2count<TSC::Time_Stamp, Time_Base>(TSC::frequency(), p);
        else
            Statistics::_hyperperiod = Math::lcm(Statistics::_hyperperiod, Convert::us2count<TSC::Time_Stamp, Time_Base>(TSC::frequency(), p));
    }

    bool collect(bool end = false);
    bool charge(bool end = false);
    bool award(bool end = false);

    static void init();

    volatile Statistics & statistics() { return _statistics; }

    using PEDF::queue;

private:
    static Hertz _max_clock;
    static Hertz _min_clock;
    static Hertz _last_freq;

    static bool _imbalanced;
    static unsigned int _last_migration_from;
    static unsigned int _last_migration_to;
    static EA_PEDF *_last_migration;
    static EA_PEDF *_last_swap;
    static float _migration_optimization_threshold;
    static unsigned long long _imbalance_threshold;

    static EA_PEDF *_first_criterion[Traits<Build>::CPUS];
    static EA_PEDF *_last_criterion[Traits<Build>::CPUS];

    // Actuation Control
    static bool _cooldown[Traits<Build>::CPUS];                              // not actuate on cooldown
    static bool _decrease_frequency_vote[Traits<Build>::CPUS];               // CPU votes
    static bool _vote_ready[Traits<Build>::CPUS];                            // CPU vote ready
    static bool _to_learn[Traits<Build>::CPUS];                              // online learning
    static float _activity_weights[PMU_EVENTS];    // Migration feature weights

    // ANN Control
    static unsigned int _train_count[Traits<Build>::CPUS];
    static FANN::fann_type *_train_input[Traits<Build>::CPUS][SAMPLES_PER_TRAIN_LIMIT];
    static FANN::fann_type _desired_output[Traits<Build>::CPUS][SAMPLES_PER_TRAIN_LIMIT];
    static struct FANN::fann *_ann[Traits<Build>::CPUS];
    static FANN::fann_type _utilization_prediction[Traits<Build>::CPUS];

    // Collect control
    static unsigned int _collected[Traits<Build>::CPUS];

    static Clerk<PMU>* _bus_access_st_ca53_v8[Traits<Build>::CPUS];
    static Clerk<PMU>* _data_write_stall_st_buffer_full_ca53_v8[Traits<Build>::CPUS];
    static Clerk<PMU>* _immediate_branches_ca[Traits<Build>::CPUS];
    static Clerk<PMU>* _l2d_writeback[Traits<Build>::CPUS];
    static Clerk<PMU>* _cpu_cycles[Traits<Build>::CPUS];
    static Clerk<PMU>* _l1_cache_hits[Traits<Build>::CPUS];

private:
    Statistics _statistics;
    EA_PEDF * _next;
};

#endif

// Clustered Earliest Deadline First (multicore)
class CEDF: public EDF, public Variable_Queue_Scheduler
{
public:
    // QUEUES x HEADS must be equal to Traits<Machine>::CPUS
    static const unsigned int HEADS = 2;
    static const unsigned int QUEUES = Traits<Machine>::CPUS / HEADS;

public:
    CEDF(int p = APERIODIC)
    : EDF(p), Variable_Queue_Scheduler(((_priority == IDLE) || (_priority == MAIN)) ? current_queue() : 0) {} // Aperiodic

    CEDF(const Microsecond & d, const Microsecond & p = SAME, const Microsecond & c = UNKNOWN, unsigned int cpu = ANY)
    : EDF(d, p, c), Variable_Queue_Scheduler((cpu != ANY) ? cpu / HEADS : ++_next_queue %= CPU::cores() / HEADS) {}

    CEDF(const CEDF &cedf) : Variable_Queue_Scheduler(cedf) {}

    using Variable_Queue_Scheduler::queue;

    static unsigned int current_queue() { return CPU::id() / HEADS; }
    static unsigned int current_head() { return CPU::id() % HEADS; }
};

__END_SYS

__BEGIN_UTIL

// Scheduling Queues
template<typename T>
class Scheduling_Queue<T, GRR>:
public Multihead_Scheduling_List<T> {};

template<typename T>
class Scheduling_Queue<T, Fixed_CPU>:
public Scheduling_Multilist<T> {};

template<typename T>
class Scheduling_Queue<T, CPU_Affinity>:
public Scheduling_Multilist<T> {};

template<typename T>
class Scheduling_Queue<T, PRM>:
public Scheduling_Multilist<T> {};

template<typename T>
class Scheduling_Queue<T, GEDF>:
public Multihead_Scheduling_List<T> {};

template<typename T>
class Scheduling_Queue<T, PEDF>:
public Scheduling_Multilist<T> {};

template<typename T>
class Scheduling_Queue<T, EA_PEDF_RV64>:
public Scheduling_Multilist<T> {};

template<typename T>
class Scheduling_Queue<T, EA_PEDF>:
public Scheduling_Multilist<T> {};

template<typename T>
class Scheduling_Queue<T, CEDF>:
public Multihead_Scheduling_Multilist<T> {};

__END_UTIL

#endif
