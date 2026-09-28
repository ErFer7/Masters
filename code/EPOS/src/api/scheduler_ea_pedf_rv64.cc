// EPOS CPU Energy-aware PEDF Scheduler Component Implementation
#include <clerk.h>
#include <process.h>

#include <machine/clock_tree.h>
#include "scheduler.h"

#ifdef __DVFS_H
#ifdef __PMU_H
__BEGIN_SYS

bool EA_PEDF_RV64::_enabled = false;

Clerk<System> *EA_PEDF_RV64::_system_clerks[Traits<Build>::CPUS][COUNTOF(EA_PEDF_RV64::SYSTEM_EVENTS)];
Clerk<PMU> *EA_PEDF_RV64::_pmu_clerks[Traits<Build>::CPUS][COUNTOF(EA_PEDF_RV64::PMU_EVENTS)];
float EA_PEDF_RV64::_pmu_min_values[Traits<Build>::CPUS][COUNTOF(EA_PEDF_RV64::PMU_EVENTS)];
float EA_PEDF_RV64::_pmu_max_values[Traits<Build>::CPUS][COUNTOF(EA_PEDF_RV64::PMU_EVENTS)];
unsigned int EA_PEDF_RV64::_system_buffer_count = COUNTOF(EA_PEDF_RV64::SYSTEM_EVENTS);
unsigned long long EA_PEDF_RV64::_hyperperiod;
unsigned long long EA_PEDF_RV64::_next_hyperperiod = -1ULL;
EA_PEDF_RV64::Thread_List EA_PEDF_RV64::_thread_list;
FANN::fann *EA_PEDF_RV64::_ann[Traits<Build>::CPUS];
float EA_PEDF_RV64::_core_utilization[Traits<Build>::CPUS];
float EA_PEDF_RV64::_predicted_core_utilization[Traits<Build>::CPUS];

unsigned long long EA_PEDF_RV64::_next_periodic_sampling = 0;

bool EA_PEDF_RV64::_decreased_frequency = false;
bool EA_PEDF_RV64::_train_ann[Traits<Build>::CPUS];
EA_PEDF_RV64::ANN_Dataset EA_PEDF_RV64::_ann_training_dataset[Traits<Build>::CPUS];

bool EA_PEDF_RV64::_balanced = false;
bool EA_PEDF_RV64::_revoke = false;

float EA_PEDF_RV64::_activity_weights[PMU_INPUTS] =
    {0.3026, 0.1828, 0.2926, 0.3867, 0.4251, 0.3102, 0.2826, 0.3252, 0.4707, 0.2459};

EA_PEDF_RV64::Activity EA_PEDF_RV64::_activity_vectors[Traits<Build>::CPUS];
EA_PEDF_RV64::Activity EA_PEDF_RV64::_last_activity_vectors[Traits<Build>::CPUS];
float EA_PEDF_RV64::_last_activity_cpu[Traits<Build>::CPUS];
float EA_PEDF_RV64::_last_activity_variance = 3.4e38f;
float EA_PEDF_RV64::_activity_cpu[Traits<Build>::CPUS];
float EA_PEDF_RV64::_migration_optimization_threshold = 0.05f;
unsigned int EA_PEDF_RV64::_pre_migration_frequency_level = 3;
bool EA_PEDF_RV64::_train_activity = false;
unsigned int EA_PEDF_RV64::_last_migration_from = 0;
unsigned int EA_PEDF_RV64::_last_migration_to = 0;
EA_PEDF_RV64 *EA_PEDF_RV64::_last_migrated = nullptr;
EA_PEDF_RV64 *EA_PEDF_RV64::_last_swapped = nullptr;

unsigned int EA_PEDF_RV64::_cooldown = 5;

bool EA_PEDF_RV64::_has_missed_deadlines = false;

EA_PEDF_RV64::EA_PEDF_RV64(const Microsecond &d,
                           const Microsecond &p,
                           const Microsecond &c,
                           unsigned int cpu,
                           bool monitored,
                           Thread *thread)
    : PEDF(d, p, c, cpu),
      _monitored(monitored),
      _target_migration_cpu(-1) {
    if (_monitored) {
        // We allow up to 1 collection per period plus some extra collections
        unsigned int max_buffer_size = (TIME_SPAN * 1000000 / count2us(_period)) + ALLOCATION_SAFETY_MARGIN;

        _thread = thread;

        if (SAFE) {
            _last_hyperperiod_deadline_misses = 0;
        }

        allocate_system_buffers<0>(max_buffer_size);
        allocate_pmu_buffers<0>(max_buffer_size);

        _first_periodic_sample = true;
        _l1_by_l2_buffer = Data_Buffer<float>(ARCHITECTURE_DEPENDENT_EVENT116, RAW, IGNORED_INPUT, max_buffer_size);

        _next_periodic_sampling = TSC::time_stamp();

        if (PREDICT) {
            kout << "EA_PEDF_RV64: Inserting thread " << thread << " into the list" << endl;

            _thread_list.insert(new Thread_List::Element(thread));

            _predicted_utilization_buffer =
                Data_Buffer<unsigned long long>(PREDICTED_JOB_UTILIZATION, RAW, IGNORED_INPUT, max_buffer_size);

            for (unsigned int i = 0; i < INPUTS; i++) {
                _ann_input[i] = 0;
            }

            _ann_output = 0;

            if (TRAIN) {
                for (unsigned int i = 0; i < 4; i++) {
                    _ann_expected_output[i] = Average_Value<float>();
                    _ann_expected_output[i].average = -1.0f;
                }
            }

            if (BALANCE) {
                _activity_sum = 0.0f;

                for (unsigned int cpu = 0; cpu < Traits<Build>::CPUS; cpu++) {
                    _migration_locked[cpu] = false;
                }
            }

            if (_hyperperiod == 0) {
                _hyperperiod = us2count(p);
            } else {
                _hyperperiod = Math::lcm(_hyperperiod, us2count(p));
            }
        }
    }
}

void EA_PEDF_RV64::init() {
    init_system_monitoring<0>();
    init_pmu_monitoring<0>();

    if (PREDICT) {
        for (unsigned int cpu = 1; cpu < Traits<Build>::CPUS; cpu++) {
            _ann[cpu] = FANN::fann_create_from_config(ANN_DEVIATION_THRESHOLD);
        }
    }
}

void EA_PEDF_RV64::handle(Event event) {
    PEDF::handle(event);

    if (!_enabled) {
        return;
    }

    if (PREDICT && event & TIMER_INTERRUPTION && CPU::id() == 0 && reached_hyperperiod()) {
        if (SAFE) {
            _has_missed_deadlines = false;
        }

        collect_data();
        handle_deadline_misses();

        if (_cooldown == 0) {
            if (TRAIN && _decreased_frequency) {
                train();
            }

            predict();

            if (BALANCE && !_decreased_frequency) {
                balance_load();
            }
        } else {
            _cooldown--;
        }

        reset_hyperperiod();
    }

    if (!_monitored) {
        return;
    }

    if (event & TIMER_INTERRUPTION) {
        collect_periodic_data();
    }

    if (event & JOB_EXECUTION_START) {
        reset_periodic_data_collection();

        for (unsigned int i = 0; i < _system_buffer_count; i++) {
            _system_buffers[i].read_start(_system_clerks[CPU::id()][i]->read(_thread));
        }

        for (unsigned int i = 0; i < PMU_EVENTS_COUNT; i++) {
            _pmu_buffers[i].read_start(_pmu_clerks[CPU::id()][i]->read());
        }

        return;
    }

    if (event & RETREAT) {
        for (unsigned int i = 0; i < _system_buffer_count; i++) {
            _system_buffers[i].read_retreat(_system_clerks[CPU::id()][i]->read(_thread));
        }

        for (unsigned int i = 0; i < PMU_EVENTS_COUNT; i++) {
            _pmu_buffers[i].read_retreat(_pmu_clerks[CPU::id()][i]->read());
        }

        return;
    }

    if (event & DISPATCH) {
        _first_periodic_sample = true;

        for (unsigned int i = 0; i < _system_buffer_count; i++) {
            _system_buffers[i].read_dispatch(_system_clerks[CPU::id()][i]->read(_thread));
        }

        for (unsigned int i = 0; i < PMU_EVENTS_COUNT; i++) {
            _pmu_buffers[i].read_dispatch(_pmu_clerks[CPU::id()][i]->read());
        }

        return;
    }

    if (event & JOB_EXECUTION_FINISH) {
        Microsecond timestamp = now();

        for (unsigned int i = 0; i < _system_buffer_count; i++) {
            _system_buffers[i].read_finish(timestamp, _system_clerks[CPU::id()][i]->read(_thread));
        }

        for (unsigned int i = 0; i < PMU_EVENTS_COUNT; i++) {
            _pmu_buffers[i].read_finish(timestamp, _pmu_clerks[CPU::id()][i]->read());
        }

        for (unsigned int i = 0; i < PMU_EVENTS_COUNT; i++) {
            _pmu_variance_buffer[i].read_finish(timestamp, _job_pmu_variance[i].get());
        }

        _l1_by_l2_buffer.read_finish(timestamp, static_cast<float>(_pmu_buffers[2].last_data_value()) / static_cast<float>(_pmu_buffers[6].last_data_value()));

        _predicted_utilization_buffer.read_finish(
            timestamp, static_cast<unsigned long long>(_ann_output * count2us(_hyperperiod)));
    }
}

void EA_PEDF_RV64::print_data() {
    OStream os;

    if (!_monitored) {
        os << "This thread wasn't monitored!" << endl;
    }

    os << "begin_data" << endl;

    for (unsigned int i = 0; i < _system_buffer_count; i++) {
        os << "System buffer: " << Clerk<System>::get_event_name(static_cast<System_Event>(_system_buffers[i].event))
           << endl;

        for (unsigned int j = 0; j < _system_buffers[i].captures; j++) {
            Data<unsigned long long> *data = &_system_buffers[i].buffer[j];

            os << data->timestamp << "," << data->value << endl;
        }
    }

    for (unsigned int i = 0; i < PMU_EVENTS_COUNT; i++) {
        os << "PMU buffer: " << PMU::get_event_name(_pmu_buffers[i].event) << endl;

        for (unsigned int j = 0; j < _pmu_buffers[i].captures; j++) {
            Data<unsigned long long> *data = &_pmu_buffers[i].buffer[j];

            os << data->timestamp << "," << data->value << endl;
        }

        os << "PMU buffer (variance): " << PMU::get_event_name(_pmu_variance_buffer[i].event) << endl;

        for (unsigned int j = 0; j < _pmu_variance_buffer[i].captures; j++) {
            Data<float> *data = &_pmu_variance_buffer[i].buffer[j];

            os << data->timestamp << "," << data->value << endl;
        }
    }

    os << "PMU buffer (virtual): " << PMU::get_event_name(_l1_by_l2_buffer.event) << endl;

    for (unsigned int j = 0; j < _l1_by_l2_buffer.captures; j++) {
        Data<float> *data = &_l1_by_l2_buffer.buffer[j];

        os << data->timestamp << "," << data->value << endl;
    }

    if (PREDICT) {
        os << "ANN buffer: "
           << Clerk<System>::get_event_name(static_cast<System_Event>(_predicted_utilization_buffer.event)) << endl;

        for (unsigned int i = 0; i < _predicted_utilization_buffer.captures; i++) {
            Data<unsigned long long> *data = &_predicted_utilization_buffer.buffer[i];

            os << data->timestamp << "," << data->value << endl;
        }

        os << "end_data" << endl;
    }
}

template <unsigned int CHANNEL>
inline void EA_PEDF_RV64::init_pmu_monitoring() {
    unsigned int used_channels = 0;

    _pmu_clerks[CPU::id()][CHANNEL] = new (SYSTEM) Clerk<PMU>(PMU_EVENTS[CHANNEL]);
    _pmu_min_values[CPU::id()][CHANNEL] = PMU_INITIAL_MIN[CHANNEL];
    _pmu_max_values[CPU::id()][CHANNEL] = PMU_INITIAL_MAX[CHANNEL];

    used_channels++;

    if (used_channels > Clerk<PMU>::CHANNELS + Clerk<PMU>::FIXED) {
        kout << "Monitor::init: some events not monitored because all PMU channels are busy!" << endl;
    }

    init_pmu_monitoring<CHANNEL + 1>();
};

template <>
inline void EA_PEDF_RV64::init_pmu_monitoring<COUNTOF(EA_PEDF_RV64::PMU_EVENTS)>() {}

template <unsigned int CHANNEL>
inline void EA_PEDF_RV64::init_system_monitoring() {
    if (CPU::id() == 0) {
        kout << "EA_PEDF_RV64::init: monitoring system event " << SYSTEM_EVENTS[CHANNEL] << endl;
    }

    _system_clerks[CPU::id()][CHANNEL] = new (SYSTEM) Clerk<System>(SYSTEM_EVENTS[CHANNEL]);

    init_system_monitoring<CHANNEL + 1>();
};

template <>
inline void EA_PEDF_RV64::init_system_monitoring<COUNTOF(EA_PEDF_RV64::SYSTEM_EVENTS)>() {}

template <unsigned int CHANNEL>
void EA_PEDF_RV64::allocate_system_buffers(const unsigned int max_buffer_size) {
    _system_buffers[CHANNEL] = Data_Buffer<unsigned long long>(
        SYSTEM_EVENTS[CHANNEL], SYSTEM_EVENTS_MODE[CHANNEL], SYSTEM_ANN_INPUT_MAP[CHANNEL], max_buffer_size);
    allocate_system_buffers<CHANNEL + 1>(max_buffer_size);
}

template <>
void EA_PEDF_RV64::allocate_system_buffers<COUNTOF(EA_PEDF_RV64::SYSTEM_EVENTS)>(unsigned int max_buffer_size) {}

template <unsigned int CHANNEL>
void EA_PEDF_RV64::allocate_pmu_buffers(const unsigned int max_buffer_size) {
    _pmu_buffers[CHANNEL] = Data_Buffer<unsigned long long>(
        PMU_EVENTS[CHANNEL], PMU_EVENTS_MODE[CHANNEL], PMU_ANN_INPUT_MAP[CHANNEL], max_buffer_size);

    _last_periodic_pmu_sample[CHANNEL] = 0;
    _job_pmu_variance[CHANNEL] = Variance();
    _pmu_variance_buffer[CHANNEL] = Data_Buffer<float>(PMU_EVENTS[CHANNEL], RAW, IGNORED_INPUT, max_buffer_size);

    allocate_pmu_buffers<CHANNEL + 1>(max_buffer_size);
}

template <>
void EA_PEDF_RV64::allocate_pmu_buffers<COUNTOF(EA_PEDF_RV64::PMU_EVENTS)>(unsigned int max_buffer_size) {}

void EA_PEDF_RV64::collect_periodic_data() {
    // TODO: Implement sampling based on frequency

    if (TSC::time_stamp() > _next_periodic_sampling) {
        _next_periodic_sampling += PERIODIC_SAMPLING_PERIOD;

        for (unsigned int i = 0; i < PMU_EVENTS_COUNT; i++) {
            if (_first_periodic_sample) {
                _last_periodic_pmu_sample[i] = _pmu_clerks[CPU::id()][i]->read();

                continue;
            }

            unsigned long long sample = _pmu_clerks[CPU::id()][i]->read();
            unsigned long long rate = sample - _last_periodic_pmu_sample[i];

            _last_periodic_pmu_sample[i] = sample;

            _job_pmu_variance[i].push(static_cast<float>(rate));
        }

        if (_first_periodic_sample) {
            _first_periodic_sample = false;
        }
    }
}

void EA_PEDF_RV64::reset_periodic_data_collection() {
    _first_periodic_sample = true;

    for (unsigned int i = 0; i < PMU_EVENTS_COUNT; i++) {
        _job_pmu_variance[i].reset();
    }
}

void EA_PEDF_RV64::collect_data() {
    // kout << "EA_PEDF_RV64::collect_data()" << endl;

    // if (SAFE) {
    //     _has_missed_deadlines = false;
    // }
    //
    // for (auto it = _thread_list.begin(); it != _thread_list.end(); it = it->next()) {
    //     EA_PEDF_RV64 *criterion = &it->object()->criterion();
    //     unsigned int cpu = criterion->queue();
    //
    //     if (SAFE) {
    //         _has_missed_deadlines = _has_missed_deadlines || criterion->_statistics.deadline_misses >
    //                                                              criterion->_last_hyperperiod_deadline_misses;
    //
    //         criterion->_last_hyperperiod_deadline_misses = criterion->_statistics.deadline_misses;
    //     }
    //
    //     if (_cooldown > 0) continue;
    //
    //     for (unsigned int i = 0; i < _system_buffer_count; i++) {
    //         int ann_input_index = criterion->_system_buffers[i].ann_input_index;
    //         float value_acc = criterion->_system_buffers[i].accumulated_collection(criterion->_period);
    //
    //         if (ann_input_index < 0) {
    //             continue;
    //         }
    //
    //         float normalized_value = 0.0;
    //
    //         if (criterion->_system_buffers[i].event == JOB_UTILIZATION) {
    //             normalized_value = value_acc / static_cast<float>(count2us(_hyperperiod));
    //         } else if (criterion->_system_buffers[i].event == CPU_CLOCK) {
    //             normalized_value = normalize_min_max(
    //                 static_cast<float>(criterion->_system_buffers[i].last_data_value()), MIN_FREQUENCY,
    //                 MAX_FREQUENCY);
    //         }
    //
    //         criterion->_ann_input[ann_input_index] = normalized_value;
    //     }
    //
    //     for (unsigned int i = 0; i < PMU_EVENTS_COUNT; i++) {
    //         int ann_input_index = criterion->_pmu_buffers[i].ann_input_index;
    //         float value_acc = criterion->_pmu_buffers[i].accumulated_collection(criterion->_period);
    //
    //         if (ann_input_index < 0) {
    //             continue;
    //         }
    //
    //         if (value_acc < _pmu_min_values[cpu][i]) {
    //             _pmu_min_values[cpu][i] = value_acc;
    //         }
    //
    //         if (value_acc > _pmu_max_values[cpu][i]) {
    //             _pmu_max_values[cpu][i] = value_acc;
    //         }
    //
    //         float normalized_value = normalize_min_max(value_acc, _pmu_min_values[cpu][i], _pmu_max_values[cpu][i]);
    //         criterion->_ann_input[ann_input_index] = normalized_value;
    //     }
    //
    //     if (TRAIN) {
    //         criterion->_ann_expected_output[HardwareClock::get_cpu_clock_frequency_level()].push(
    //             criterion->_ann_input[0]);
    //         _ann_training_dataset[cpu].push(
    //             criterion->_ann_input, HardwareClock::get_cpu_clock_frequency_level(), criterion);
    //     }
    // }
}

void EA_PEDF_RV64::handle_deadline_misses() {
    if (!_has_missed_deadlines) return;

    kout << "EA_PEDF_RV64: Deadline miss detected!" << endl;

    // HardwareClock::set_cpu_clock_frequency_level(HardwareClock::MAX);

    cooldown(1);

    if (BALANCE) {
        _revoke = true;
    }
}

void EA_PEDF_RV64::train() {
    // kout << "EA_PEDF_RV64::train()" << endl;

    // for (auto it = _thread_list.begin(); it != _thread_list.end(); it = it->next()) {
    //     EA_PEDF_RV64 *criterion = &it->object()->criterion();
    //     unsigned int cpu = criterion->queue();
    //
    //     float utilization = criterion->_ann_expected_output[HardwareClock::get_cpu_clock_frequency_level()].get();
    //     float predicted_utilization = criterion->_ann_output;
    //
    //     float error = utilization - predicted_utilization;
    //
    //     if (error < 0) {
    //         error *= -1;
    //     }
    //
    //     // TODO: Comment
    //     // kout << "EA_PEDF_RV64: Error for thread " << criterion->_thread << ": " << error << " = " << utilization
    //     //      << " - " << predicted_utilization << endl;
    //
    //     _train_ann[cpu] |= error > ANN_DEVIATION_THRESHOLD;
    // }
    //
    // for (unsigned int cpu = 1; cpu < Traits<Build>::CPUS; cpu++) {
    //     if (!_train_ann[cpu] || _ann_training_dataset[cpu].size < 2) {
    //         continue;
    //     }
    //
    //     FANN::fann_reset_MSE(_ann[cpu]);
    //
    //     for (unsigned i = 0; i < MAX_TRAINS; i++) {
    //         bool end = true;
    //
    //         for (unsigned int count = 0, j = _ann_training_dataset[cpu].first_index();
    //              count < _ann_training_dataset[cpu].size;
    //              count++, j = ANN_Dataset::next_index(j)) {
    //             float output = _ann_training_dataset[cpu].output(j);
    //
    //             // Avoid training with invalid data
    //             if (output < 0) continue;
    //
    //             // TODO: Comment
    //             // if (i == 0) {
    //             //     kout << "Training dataset: In [";
    //             //
    //             //     for (unsigned int k = 0; k < INPUTS; k++) {
    //             //         kout << _ann_training_dataset[cpu].inputs[j][k] << (k < INPUTS - 1 ? ", " : "] ");
    //             //     }
    //             //
    //             //     kout << "Out [" << output << ']' << endl;
    //             // }
    //
    //             float error =
    //                 FANN::fann_train_data_incremental(_ann[cpu], _ann_training_dataset[cpu].inputs[j], &output);
    //
    //             end = end && error <= ANN_DEVIATION_THRESHOLD;
    //         }
    //
    //         if (end) {
    //             break;
    //         }
    //     }
    //
    //     _train_ann[cpu] = false;
    // }
}

void EA_PEDF_RV64::predict() {
    // for (unsigned int cpu = 1; cpu < Traits<Build>::CPUS; cpu++) {
    //     _predicted_core_utilization[cpu] = 0.0f;
    //
    //     if (BALANCE || SAFE) {
    //         _core_utilization[cpu] = 0.0f;
    //     }
    // }
    //
    // for (auto it = _thread_list.begin(); it != _thread_list.end(); it = it->next()) {
    //     EA_PEDF_RV64 *criterion = &it->object()->criterion();
    //     unsigned int cpu = criterion->queue();
    //
    //     if (BALANCE || SAFE) {
    //         _core_utilization[cpu] += criterion->_ann_input[0];
    //     }
    //
    //     criterion->_ann_output = *FANN::fann_run(_ann[cpu], criterion->_ann_input, false);
    //
    //     _predicted_core_utilization[cpu] += criterion->_ann_output;
    //
    //     // TODO: Comment
    //     // kout << "EA_PEDF_RV64: Utilization for thread " << criterion->_thread << " on core " << cpu
    //     //      << ": Predicted (Lower DVFS): " << criterion->_ann_output << ", Current: " <<
    //     criterion->_ann_input[0]
    //     //      << endl;
    // }
    //
    // _decreased_frequency = false;
    //
    // bool result = true;
    //
    // if (SAFE) {
    //     for (unsigned int cpu = 1; cpu < Traits<Build>::CPUS; cpu++) {
    //         if (_core_utilization[cpu] < 1.0f) continue;
    //
    //         kout << "EA_PEDF_RV64: The utilization went beyond 100%! "
    //              << "Core 1 utilization: " << _core_utilization[1] << '\n'
    //              << "Core 2 utilization: " << _core_utilization[2] << '\n'
    //              << "Core 3 utilization: " << _core_utilization[3] << '\n'
    //              << endl;
    //
    //         HardwareClock::set_cpu_clock_frequency_level(HardwareClock::MAX);
    //
    //         cooldown(1);
    //
    //         if (BALANCE) {
    //             _revoke = true;
    //         }
    //
    //         return;
    //     }
    // }
    //
    // for (unsigned int cpu = 1; cpu < Traits<Build>::CPUS; cpu++) {
    //     // TODO: Comment
    //     // kout << "EA_PEDF_RV64: Prediction for core " << cpu << ": " << _predicted_core_utilization[cpu] << endl;
    //
    //     result = _predicted_core_utilization[cpu] < 1.0f - SAFETY_MARGIN;
    //
    //     if (!result) {
    //         break;
    //     }
    // }
    //
    // if (result) {
    //     unsigned int frequency_level = HardwareClock::get_cpu_clock_frequency_level();
    //
    //     if (frequency_level > MIN_FREQUENCY_LEVEL) {
    //         HardwareClock::set_cpu_clock_frequency_level(frequency_level - 1);
    //         _decreased_frequency = true;
    //         cooldown(1);
    //     }
    //
    //     if (BALANCE && HardwareClock::get_cpu_clock_frequency_level() == MIN_FREQUENCY_LEVEL) {
    //         _balanced = true;
    //
    //         // TODO: Comment
    //         // kout << "EA_PEDF_RV64: Balanced" << endl;
    //     }
    // }
}

void EA_PEDF_RV64::balance_load() {
    // Revokation
    // On deadline misses it should always revoke
    // If the cores didn't vote for a DVFS scale and we are in a higher frequency than we were before, then revoke (it
    // was a bad migration) If the cores didn't vote but we are at the minimum frequency, it won't revoke (0 > n will be
    // false)
    // if (_revoke || HardwareClock::get_cpu_clock_frequency_level() > _pre_migration_frequency_level) {
    //     // TODO: Comment
    //     // kout << "EA_PEDF_RV64: Revoking" << endl;
    //
    //     if (_last_migrated != nullptr) {
    //         unsigned int ipi_mask = 0;
    //
    //         // TODO: Comment
    //         // kout << "EA_PEDF_RV64: Migrating thread " << _last_migrated->_thread << " from " << _last_migration_to
    //         //      << " to " << _last_migration_from << endl;
    //
    //         _last_migrated->_migration_locked[_last_migration_to] = true;
    //         _last_migrated->_thread->prepare_migration(_last_migration_from, ipi_mask);
    //
    //         if (_last_swapped != nullptr) {
    //             // TODO: Comment
    //             // kout << "EA_PEDF_RV64: Swapping thread " << _last_swapped->_thread << " from " <<
    //             // _last_migration_from
    //             //      << " to " << _last_migration_to << endl;
    //
    //             _last_swapped->_migration_locked[_last_migration_from] = true;
    //             _last_swapped->_thread->prepare_migration(_last_migration_to, ipi_mask);
    //         }
    //
    //         _migration_optimization_threshold += MIGRATION_OPTIMIZATION_THRESHOLD_INCREASE_STEP;
    //
    //         // TODO: Comment
    //         // kout << "EA_PEDF_RV64: New migration optimization threshold: " << _migration_optimization_threshold <<
    //         // endl;
    //
    //         _last_migration_from = 0;
    //         _last_migration_to = 0;
    //         _last_migrated = nullptr;
    //         _last_swapped = nullptr;
    //         cooldown(1);
    //
    //         HardwareClock::set_cpu_clock_frequency_level(HardwareClock::MAX);
    //         Thread::execute_migrations(ipi_mask);
    //     } else {
    //         // kout << "EA_PEDF_RV64: Nothing to revoke" << endl;
    //     }
    //
    //     _revoke = false;
    //     _balanced = false;
    //
    //     return;
    // } else if ((!_balanced && HardwareClock::get_cpu_clock_frequency_level() == _pre_migration_frequency_level)) {
    //     _migration_optimization_threshold += MIGRATION_OPTIMIZATION_SOFT_THRESHOLD_INCREASE_STEP;
    // }
    //
    // if (_balanced) {
    //     return;
    // }
    //
    // // TODO: Comment
    // // kout << "EA_PEDF_RV64: Imbalanced" << endl;
    //
    // // Reset
    // for (unsigned int cpu = 1; cpu < Traits<Build>::CPUS; cpu++) {
    //     _activity_cpu[cpu] = 0.0f;
    //
    //     if (TRAIN_ACTIVITY_WEIGHTS) {
    //         _activity_vectors[cpu].reset();
    //     }
    // }
    //
    // // Accumulate activities
    // for (auto it = _thread_list.begin(); it != _thread_list.end(); it = it->next()) {
    //     EA_PEDF_RV64 *criterion = &it->object()->criterion();
    //     unsigned int cpu = criterion->queue();
    //
    //     criterion->_activity_sum = 0.0f;
    //
    //     for (unsigned int i = INPUTS - PMU_INPUTS, j = 0; i < INPUTS; i++, j++) {
    //         float feature_activity = criterion->_ann_input[i] * criterion->_ann_output;
    //         criterion->_activity_sum += feature_activity * _activity_weights[j];
    //
    //         if (TRAIN_ACTIVITY_WEIGHTS) {
    //             _activity_vectors[cpu][j] += feature_activity;
    //         }
    //     }
    //
    //     _activity_cpu[cpu] += criterion->_activity_sum;
    // }
    //
    // if (TRAIN_ACTIVITY_WEIGHTS && _train_activity) {
    //     float error;
    //     float update[PMU_INPUTS];
    //
    //     for (unsigned int cpu = 1; cpu < Traits<Build>::CPUS; cpu++) {
    //         error = _activity_cpu[cpu] - _last_activity_cpu[cpu];
    //
    //         for (unsigned int i = 0; i < PMU_INPUTS; i++) {
    //             update[i] = _last_activity_vectors[cpu][i] * error;
    //         }
    //     }
    //
    //     // TODO: Comment
    //     // kout << "EA_PEDF_RV64: New activity weights: [";
    //
    //     for (unsigned int i = 0; i < PMU_INPUTS; i++) {
    //         update[i] /= Traits<Build>::CPUS - 1;
    //         _activity_weights[i] -= ACTIVITY_TRAINING_LEARNING_RATE * -update[i];
    //
    //         // TODO: Comment
    //         // kout << _activity_weights[i] << (i < PMU_INPUTS - 1 ? ", " : "]");
    //     }
    //
    //     // TODO: Comment
    //     // kout << endl;
    //
    //     for (unsigned int cpu = 1; cpu < Traits<Build>::CPUS; cpu++) {
    //         _activity_cpu[cpu] = 0.0f;
    //     }
    //
    //     for (auto it = _thread_list.begin(); it != _thread_list.end(); it = it->next()) {
    //         EA_PEDF_RV64 *criterion = &it->object()->criterion();
    //         unsigned int cpu = criterion->queue();
    //
    //         criterion->_activity_sum = 0.0f;
    //
    //         for (unsigned int i = INPUTS - PMU_INPUTS, j = 0; i < INPUTS; i++, j++) {
    //             criterion->_activity_sum += criterion->_ann_input[i] * _activity_weights[j] * criterion->_ann_output;
    //         }
    //
    //         _activity_cpu[cpu] += criterion->_activity_sum;
    //     }
    //
    //     _train_activity = false;
    // }
    //
    // float original_migration_score = activity_variance();
    // float best_migration_score = original_migration_score;
    //
    // unsigned int migration_from = 0;
    // unsigned int migration_to = 0;
    // EA_PEDF_RV64 *migration_candidate = nullptr;  // from -> to
    // EA_PEDF_RV64 *swap_candidate = nullptr;       // to -> from
    //
    // // Single Migrations
    // for (auto it = _thread_list.begin(); it != _thread_list.end(); it = it->next()) {
    //     EA_PEDF_RV64 *criterion = &it->object()->criterion();
    //     unsigned int current_cpu = criterion->queue();
    //
    //     // Temporarily remove thread from current CPU
    //     _activity_cpu[current_cpu] -= criterion->_activity_sum;
    //
    //     for (unsigned int target_cpu = 1; target_cpu < Traits<Build>::CPUS; target_cpu++) {
    //         if (target_cpu == current_cpu || criterion->_migration_locked[target_cpu]) continue;
    //
    //         // New variance
    //         _activity_cpu[target_cpu] += criterion->_activity_sum;
    //
    //         float score = activity_variance();
    //
    //         if (score < best_migration_score &&
    //             score < original_migration_score * (1.0f - _migration_optimization_threshold)) {
    //             best_migration_score = score;
    //             migration_from = current_cpu;
    //             migration_to = target_cpu;
    //             migration_candidate = criterion;
    //
    //             // TODO: Comment
    //             // kout << "EA_PEDF_RV64: Migration candidate chosen with score " << score
    //             //      << ", original migration score: " << original_migration_score << endl;
    //         }
    //
    //         _activity_cpu[target_cpu] -= criterion->_activity_sum;
    //     }
    //
    //     _activity_cpu[current_cpu] += criterion->_activity_sum;
    // }
    //
    // // Swaps
    // if (migration_candidate == nullptr) {
    //     for (auto it1 = _thread_list.begin(); it1 != _thread_list.end(); it1 = it1->next()) {
    //         EA_PEDF_RV64 *criterion1 = &it1->object()->criterion();
    //         unsigned int cpu1 = criterion1->queue();
    //
    //         _activity_cpu[cpu1] -= criterion1->_activity_sum;
    //
    //         for (auto it2 = it1->next(); it2 != _thread_list.end(); it2 = it2->next()) {
    //             EA_PEDF_RV64 *criterion2 = &it2->object()->criterion();
    //             unsigned int cpu2 = criterion2->queue();
    //
    //             if (cpu1 == cpu2) continue;
    //
    //             if (criterion1->_migration_locked[cpu2] || criterion2->_migration_locked[cpu1]) continue;
    //
    //             // Temporarily apply swap
    //             _activity_cpu[cpu2] -= criterion2->_activity_sum;
    //             _activity_cpu[cpu2] += criterion1->_activity_sum;
    //             _activity_cpu[cpu1] += criterion2->_activity_sum;
    //
    //             float score = activity_variance();
    //
    //             if (score < best_migration_score &&
    //                 score < original_migration_score * (1.0f - _migration_optimization_threshold)) {
    //                 best_migration_score = score;
    //                 migration_from = cpu1;
    //                 migration_to = cpu2;
    //                 migration_candidate = criterion1;
    //                 swap_candidate = criterion2;
    //
    //                 // TODO: Comment
    //                 // kout << "EA_PEDF_RV64: Swap candidates chosen with score " << score
    //                 //      << ", original migration score: " << original_migration_score << endl;
    //             }
    //
    //             _activity_cpu[cpu2] -= criterion1->_activity_sum;
    //             _activity_cpu[cpu1] -= criterion2->_activity_sum;
    //             _activity_cpu[cpu2] += criterion2->_activity_sum;
    //         }
    //
    //         _activity_cpu[cpu1] += criterion1->_activity_sum;
    //     }
    // }
    //
    // // Migration
    // if (migration_candidate != nullptr) {
    //     unsigned int ipi_mask = 0;
    //
    //     // TODO: Comment
    //     // kout << "EA_PEDF_RV64: Migrating thread " << migration_candidate->_thread << " from " << migration_from
    //     //      << " to " << migration_to << endl;
    //
    //     migration_candidate->_thread->prepare_migration(migration_to, ipi_mask);
    //
    //     if (swap_candidate != nullptr) {
    //         // TODO: Comment
    //         // kout << "EA_PEDF_RV64: Swapping thread " << swap_candidate->_thread << " from " << migration_to << "
    //         to "
    //         //      << migration_from << endl;
    //
    //         swap_candidate->_thread->prepare_migration(migration_from, ipi_mask);
    //     }
    //
    //     if (TRAIN_ACTIVITY_WEIGHTS) _train_activity = true;
    //
    //     _last_migration_from = migration_from;
    //     _last_migration_to = migration_to;
    //     _last_migrated = migration_candidate;
    //     _last_swapped = swap_candidate;
    //
    //     cooldown(1);
    //
    //     _last_activity_variance = activity_variance();
    //
    //     for (unsigned int cpu = 1; cpu < Traits<Build>::CPUS; cpu++) {
    //         _last_activity_cpu[cpu] = _activity_cpu[cpu];
    //         _last_activity_vectors[cpu] = _activity_vectors[cpu];
    //     }
    //
    //     _pre_migration_frequency_level = HardwareClock::get_cpu_clock_frequency_level();
    //     HardwareClock::set_cpu_clock_frequency_level(HardwareClock::MAX);
    //     Thread::execute_migrations(ipi_mask);
    // }
}

float EA_PEDF_RV64::activity_variance() {
    float activity_average = 0.0f;
    float activity_variance = 0.0f;

    for (unsigned int cpu = 1; cpu < Traits<Build>::CPUS; cpu++) {
        activity_average += _activity_cpu[cpu];
    }

    activity_average /= (Traits<Build>::CPUS - 1);

    for (unsigned int cpu = 1; cpu < Traits<Build>::CPUS; cpu++) {
        activity_variance += (activity_average - _activity_cpu[cpu]) * (activity_average - _activity_cpu[cpu]);
    }

    return activity_variance;
}

__END_SYS
#endif
#endif
