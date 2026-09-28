// EPOS CPU Scheduler Component Implementation

#include <process.h>
#include <time.h>
#include <clerk.h>

__BEGIN_SYS

extern OStream kout;

volatile unsigned int Variable_Queue_Scheduler::_next_queue;
Scheduling_Criterion_Common::Tick Scheduling_Criterion_Common::Statistics::cpu_time[Traits<Build>::CPUS];

Scheduling_Criterion_Common::Tick Scheduling_Criterion_Common::now() { return Traits<TSC>::enabled ? TSC::time_stamp() : Alarm::elapsed(); }

RT_Common::Tick RT_Common::ticks(Microsecond time) {
    return Traits<TSC>::enabled ? TSC::ts(time) : Alarm::ticks(time);
}

Microsecond RT_Common::time(Tick ticks) {
    return Traits<TSC>::enabled ? TSC::time(ticks) : Alarm::time(ticks);
}

void Scheduling_Criterion_Common::handle(Event event)
{
    db<Thread>(TRC) << "Criterion::handle(this=" << this << ",e=";
    if(event & CREATION) {
        db<Thread>(TRC) << "CREATION";

        _statistics.thread_creation = now();
        _statistics.job_utilization = 0;
    }
    if(event & FINISH) {
        db<Thread>(TRC) << "FINISH";

        _statistics.thread_destruction = now();
    }
    if(event & DISPATCH) {
        db<Thread>(TRC) << "DISPATCH";

        _statistics.thread_last_dispatch = now();
    }
    if(event & RETREAT) {
        Tick cpu_time = now() - _statistics.thread_last_dispatch;

        db<Thread>(TRC) << "RETREAT";

        _statistics.thread_last_preemption = now();
        _statistics.thread_execution_time += cpu_time;
    }
    if(event & JOB_RELEASE) {
        db<Thread>(TRC) << "JOB_RELEASE";
    }
    if(event & JOB_FINISH) {
        db<Thread>(TRC) << "JOB_FINISH";
    }

    if(event & COLLECT) {
        db<Thread>(TRC) << "|COLLECT";
    }
    if(event & CHARGE) {
        db<Thread>(TRC) << "|CHARGE";
    }
    if(event & AWARD) {
        db<Thread>(TRC) << "|AWARD";
    }
    if(event & UPDATE) {
        db<Thread>(TRC) << "|UPDATE";
    }
    if(event & REBALANCE) {
        db<Thread>(TRC) << "|REBALANCE";
    }
    db<Thread>(TRC) << ")" << endl;
}

// TODO: Refactor this
void RT_Common::handle(Event event)
{
    Scheduling_Criterion_Common::handle(event);
    if(event & RETREAT) {
        _statistics.job_partial_utilization += now() - _statistics.job_execution_block_initial_time;

        _statistics.job_last_preemption = now();
    }
    if (event & DISPATCH) {
        _statistics.job_execution_block_initial_time = now();
        // if (_statistics.job_last_preemption != 0)
        //     _statistics.job_sleep_time += now() - _statistics.job_last_preemption;
    }
    if (event & JOB_EXECUTION_START) {
        _statistics.job_execution_block_initial_time = now();
        _statistics.job_partial_utilization = 0;

        _statistics.job_execution_start = now();
        _statistics.job_last_preemption = 0;
        _job_status = RUNNING;
    }
    if (event & JOB_EXECUTION_FINISH) {
        _job_status = FINISHED;
        _statistics.job_utilization = now() - _statistics.job_execution_block_initial_time + _statistics.job_partial_utilization;
    }
    if (event & JOB_ALARM_HANDLE) {
        if (_job_status & (UNINITIALIZED | RUNNING)) {
            _statistics.deadline_misses++;
        }
        _job_status = UNINITIALIZED;
    }
    if(periodic() && (event & JOB_RELEASE)) {
        _statistics.job_release = now();
        // _statistics.job_start = 0;
        // _statistics.job_utilization = 0;
        _statistics.jobs_released++;
    }
    if(periodic() && (event & JOB_FINISH)) {
        _statistics.job_finish = now();
        // _statistics.job_utilization = _statistics.job_finish - _statistics.job_release;
        _statistics.jobs_finished++;
    }
//    db<Thread>(INF) << "Criterion::handle(e=" << event << ") => {i=" << _priority << "," << *this << ",s=" << statistics() << endl;
}

template <typename ... Tn>
FCFS::FCFS(int i, Tn & ... an): Priority((i == IDLE) ? IDLE : RT_Common::now()) {}


EDF::EDF(Microsecond p, Microsecond d, Microsecond c): RT_Common(int(now() + ticks(d)), p, d, c) {}

void EDF::handle(Event event) {
    RT_Common::handle(event);

    // Update the priority of the thread at job releases, before _alarm->v(), so it enters the queue in the right order (called from Periodic_Thread::Xxx_Handler)
    if(periodic() && (event & JOB_RELEASE))
        _priority = now() + _deadline;
}


LLF::LLF(Microsecond p, Microsecond d, Microsecond c): RT_Common(int(now() + ticks((d ? d : p) - c)), p, d, c) {}

void LLF::handle(Event event) {
    if(periodic() && ((event & UPDATE) | (event & JOB_RELEASE) | (event & JOB_FINISH))) {
        _priority = now() + _deadline - _capacity + _statistics.job_utilization;
    }
    RT_Common::handle(event);

    // Update the priority of the thread at job releases, before _alarm->v(), so it enters the queue in the right order (called from Periodic_Thread::Xxx_Handler)
//    if((_priority >= PERIODIC) && (_priority < APERIODIC) && ((event & JOB_FINISH) || (event & UPDATE_ALL)))
}


// Since the definition of FCFS above is only known to this unit, forcing its instantiation here so it gets emitted in scheduler.o for subsequent linking with other units is necessary.
template FCFS::FCFS<>(int p);

__END_SYS
