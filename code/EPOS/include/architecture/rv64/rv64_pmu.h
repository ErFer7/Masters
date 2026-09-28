// EPOS RV64 PMU Mediator Declarations

#ifndef __rv64_pmu_h
#define __rv64_pmu_h

#include <architecture/cpu.h>
#define __pmu_common_only__
#include <architecture/pmu.h>
#undef __pmu_common_only__
#define __rv32_pmu_common_only__
#include <architecture/rv32/rv32_pmu.h>
#undef __rv32_pmu_common_only__

#include <system/memory_map.h>

__BEGIN_SYS

extern OStream kout;

class RV64_PMU: public RV32_PMU
{
private:
    typedef CPU::Reg Reg;

    enum : Reg {
        L2_PMU_EVENT_CONTROL  = 0x2000,
        L2_PMU_COUNTERS       = 0x3000
    };

    static const unsigned int L2_PMU_CHANNELS = Traits<Build>::MODEL == Traits_Tokens::VisionFive2 ? 64 : 0;
    static const unsigned int L2_PMU_EVENTS_START = Traits<Build>::MODEL == Traits_Tokens::VisionFive2 ? 61 : -1U;
    static const unsigned int PMU_CHANNEL_LIMIT = 4;
    static const unsigned int L2_PMU_CHANNEL_START = Traits<Build>::MODEL == Traits_Tokens::VisionFive2 ? PMU_CHANNEL_LIMIT + 1 : -1U;
    static const unsigned int L2_PMU_CHANNEL_LIMIT = Traits<Build>::MODEL == Traits_Tokens::VisionFive2 ? L2_PMU_CHANNEL_START + 6 : -1U;

    // L2PM events
    enum {
        // L2 Performance Monitor Transaction Events (L2pmeventX[7:0] = 1)
        L2_INNER_PUTFULLDATA                      = 1 <<  8 | 1,
        L2_INNER_PUTPARTIALDATA                   = 1 <<  9 | 1,
        L2_INNER_ATOMICDATA                       = 1 << 10 | 1,
        L2_INNER_GET                              = 1 << 11 | 1,
        L2_INNER_PREFETCH_READ                    = 1 << 12 | 1,
        L2_INNER_PREFETCH_WRITE                   = 1 << 13 | 1,
        L2_INNER_ACQUIREBLOCK_NTOB                = 1 << 14 | 1,
        L2_INNER_ACQUIREBLOCK_NTOT                = 1 << 15 | 1,
        L2_INNER_ACQUIREBLOCK_BTOT                = 1 << 16 | 1,
        L2_INNER_ACQUIREPERM_NTOT                 = 1 << 17 | 1,
        L2_INNER_ACQUIREPERM_BTOT                 = 1 << 18 | 1,
        L2_INNER_RELEASE_TOB                      = 1 << 19 | 1,
        L2_INNER_RELEASE_TTON                     = 1 << 20 | 1,
        L2_INNER_RELEASE_BTON                     = 1 << 21 | 1,
        L2_INNER_RELEASEDATA_TOB                  = 1 << 22 | 1,
        L2_INNER_RELEASEDATA_TTON                 = 1 << 23 | 1,
        L2_INNER_RELEASEDATA_BTON                 = 1 << 24 | 1,
        L2_OUTER_PROBEBLOCK_TOT                   = 1 << 25 | 1,
        L2_OUTER_PROBEBLOCK_TOB                   = 1 << 26 | 1,
        L2_OUTER_PROBEBLOCK_TON                   = 1 << 27 | 1,

        // L2 Query Result Events (L2pmeventX[7:0] = 2)
        L2_INNER_PUTFULLDATA_HIT_L2               = 1 <<  8 | 2,
        L2_INNER_PUTPARTIALDATA_HIT_L2            = 1 <<  9 | 2,
        L2_INNER_ATOMICDATA_HIT_L2                = 1 << 10 | 2,
        L2_INNER_GET_HIT_L2                       = 1 << 11 | 2,
        L2_INNER_PREFETCH_HIT_L2                  = 1 << 12 | 2,
        L2_INNER_ACQUIREBLOCK_HIT_L2              = 1 << 13 | 2,
        L2_INNER_ACQUIREPERM_HIT_L2               = 1 << 14 | 2,
        L2_INNER_RELEASE_HIT_L2                   = 1 << 15 | 2,
        L2_INNER_RELEASEDATA_HIT_L2               = 1 << 16 | 2,
        L2_OUTER_PROBE_HIT_L2                     = 1 << 17 | 2,
        L2_INNER_PUTFULLDATA_HIT_L2_SHARED        = 1 << 18 | 2,
        L2_INNER_PUTPARTIALDATA_HIT_L2_SHARED     = 1 << 19 | 2,
        L2_INNER_ATOMICDATA_HIT_L2_SHARED         = 1 << 20 | 2,
        L2_INNER_GET_HIT_L2_SHARED                = 1 << 21 | 2,
        L2_INNER_PREFETCH_HIT_L2_SHARED           = 1 << 22 | 2,
        L2_INNER_ACQUIREBLOCK_HIT_L2_SHARED       = 1 << 23 | 2,
        L2_INNER_ACQUIREPERM_HIT_L2_SHARED        = 1 << 24 | 2,
        L2_OUTER_PROBE_HIT_L2_SHARED              = 1 << 25 | 2,
        L2_OUTER_PROBE_HIT_L2_MODIFIED            = 1 << 26 | 2,

        // L2 Request Events (L2pmeventX[7:0] = 3)
        L2_OUTER_ACQUIREBLOCK_NTOB_L2_MISS        = 1 <<  8 | 3,
        L2_OUTER_ACQUIREBLOCK_NTOT_L2_MISS        = 1 <<  9 | 3,
        L2_OUTER_ACQUIREBLOCK_BTOT_L2_MISS        = 1 << 10 | 3,
        L2_OUTER_ACQUIREPERM_NTOT_L2_MISS         = 1 << 11 | 3,
        L2_OUTER_ACQUIREPERM_BTOT_L2_MISS         = 1 << 12 | 3,
        L2_OUTER_RELEASE_TOB_EVICTION             = 1 << 13 | 3,
        L2_OUTER_RELEASE_TON_EVICTION             = 1 << 14 | 3,
        L2_OUTER_RELEASE_BTON_EVICTION            = 1 << 15 | 3,
        L2_OUTER_RELEASEDATA_TOB                  = 1 << 16 | 3,
        L2_OUTER_RELEASEDATA_TTON_DIRTY_EVICTION  = 1 << 17 | 3,
        L2_OUTER_RELEASEDATA_BTON                 = 1 << 18 | 3,
        L2_INNER_PROBEBLOCK_TOT_CODE_MISS         = 1 << 19 | 3,
        L2_INNER_PROBEBLOCK_TOB_LOAD_MISS         = 1 << 20 | 3,
        L2_INNER_PROBEBLOCK_TON_STORE_MISS        = 1 << 21 | 3,

        // L2 Hardware Prefetcher Events (L2pmeventX[7:0] = 4)
        L2_DEMAND_MISS_HIT_MSHR_ALLOC_HINT        = 1 <<  8 | 4,

        // Combined L2 cache miss
        L2_CACHE_MISS                             = L2_OUTER_ACQUIREBLOCK_NTOB_L2_MISS | L2_OUTER_ACQUIREBLOCK_NTOT_L2_MISS,
        L1_BY_L2                                  = 0
    };

public:
    RV64_PMU() {}

    static void config(Channel channel, const Event event, Flags flags = NONE) {
        assert((channel < CHANNELS) && (event < EVENTS));

        if (CPU::id() == 0) {
            db<PMU>(TRC) << "PMU::config(c="
                << channel
                << ",e="
                << event
                << ",f="
                << flags
                << ")"
                << endl;
        }

        if((channel == 0 && _events[event] != 0) || (channel == 1 && _events[event] != 1) || (channel == 2 && _events[event] != 2)) {
            db<PMU>(WRN) << "PMU::config: channel " << channel << " is fixed in this architecture and cannot be reconfigured!" << endl;
            return;
        }

        if(channel >= FIXED) {
            if (event < L2_PMU_EVENTS_START) {
                if (_events[event] != UNSUPORTED_EVENT && channel <= PMU_CHANNEL_LIMIT) {
                    mhpmevent(_events[event], channel);
                    reset(channel);
                    start(channel);

                    return;
                }
            } else if (_l2pm_events[event] != UNSUPORTED_EVENT && channel <= L2_PMU_CHANNEL_LIMIT) {
                l2pmevent(_l2pm_events[event - L2_PMU_EVENTS_START], channel - L2_PMU_CHANNEL_START);

                return;
            }

            db<PMU>(WRN) << "PMU::config: Unsuported event or channel overflow!" << endl;
        }
    }

    static const unsigned long long get_event_code(const Event event) {
        return event < L2_PMU_EVENTS_START ? _events[event] : _l2pm_events[event - L2_PMU_EVENTS_START];
    }

    static const char *get_event_name(const Event event) {
        if (event < L2_PMU_EVENTS_START) {
            switch (_events[event]) {
                case CYCLES:
                    return "PMU::CPU_CYCLES";
                case TIME:
                    return "PMU::TIME";
                case INSTRUCTIONS_RETIRED:
                    return "PMU::INSTRUCTIONS_RETIRED";
                case EXCEPTIONS_TAKEN:
                    return "PMU::EXCEPTIONS_TAKEN";
                case INTEGER_LOAD_INSTRUCTIONS_RETIRED:
                    return "PMU::INTEGER_LOAD_INSTRUCTIONS_RETIRED";
                case INTEGER_STORE_INSTRUCTIONS_RETIRED:
                    return "PMU::INTEGER_STORE_INSTRUCTIONS_RETIRED";
                case ATOMIC_MEMORY_INSTRUCTIONS_RETIRED:
                    return "PMU::ATOMIC_MEMORY_INSTRUCTIONS_RETIRED";
                case SYSTEM_INSTRUCTIONS_RETIRED:
                    return "PMU::SYSTEM_INSTRUCTIONS_RETIRED";
                case INTEGER_ARITHMETIC_INSTRUCTIONS_RETIRED:
                    return "PMU::INTEGER_ARITHMETIC_INSTRUCTIONS_RETIRED";
                case CONDITIONAL_BRANCHES_RETIRED:
                    return "PMU::CONDITIONAL_BRANCHES_RETIRED";
                case JAL_INSTRUCTIONS_RETIRED:
                    return "PMU::JAL_INSTRUCTIONS_RETIRED";
                case JALR_INSTRUCTIONS_RETIRED:
                    return "PMU::JALR_INSTRUCTIONS_RETIRED";
                case INTEGER_MULTIPLICATION_INSTRUCTIONS_RETIRED:
                    return "PMU::INTEGER_MULTIPLICATION_INSTRUCTIONS_RETIRED";
                case INTEGER_DIVISION_INSTRUCTIONS_RETIRED:
                    return "PMU::INTEGER_DIVISION_INSTRUCTIONS_RETIRED";
                case FLOATING_POINT_LOAD_INSTRUCTION_RETIRED:
                    return "PMU::FLOATING_POINT_LOAD_INSTRUCTION_RETIRED";
                case FLOATING_POINT_STORE_INSTRUCTION_RETIRED:
                    return "PMU::FLOATING_POINT_STORE_INSTRUCTION_RETIRED";
                case FLOATING_POINT_ADDITION_RETIRED:
                    return "PMU::FLOATING_POINT_ADDITION_RETIRED";
                case FLOATING_POINT_MULTIPLICATION_RETIRED:
                    return "PMU::FLOATING_POINT_MULTIPLICATION_RETIRED";
                case FLOATING_POINT_FUSED_MULTIPLY_ADD_RETIRED:
                    return "PMU::FLOATING_POINT_FUSED_MULTIPLY_ADD_RETIRED";
                case FLOATING_POINT_DIVISION_OR_SQUARE_ROOT_RETIRED:
                    return "PMU::FLOATING_POINT_DIVISION_OR_SQUARE_ROOT_RETIRED";
                case OTHER_FLOATING_POINT_INSTRUCTION_RETIRED:
                    return "PMU::OTHER_FLOATING_POINT_INSTRUCTION_RETIRED";
                case ADDRESS_GENERATION_INTERLOCK:
                    return "PMU::ADDRESS_GENERATION_INTERLOCK";
                case LONG_LATENCY_INTERLOCK:
                    return "PMU::LONG_LATENCY_INTERLOCK";
                case CSR_READ_INTERLOCK:
                    return "PMU::CSR_READ_INTERLOCK";
                case INSTRUCTION_CACHE_ITIM_BUSY:
                    return "PMU::INSTRUCTION_CACHE_ITIM_BUSY";
                case DATA_CACHE_DTIM_BUSY:
                    return "PMU::DATA_CACHE_DTIM_BUSY";
                case BRANCH_DIRECTION_MISPREDICTION:
                    return "PMU::BRANCH_DIRECTION_MISPREDICTION";
                case BRANCH_JUMP_TARGET_MISPREDICTION:
                    return "PMU::BRANCH_JUMP_TARGET_MISPREDICTION";
                case PIPELINE_FLUSH_FROM_CSR_WRITE:
                    return "PMU::PIPELINE_FLUSH_FROM_CSR_WRITE";
                case PIPELINE_FLUSH_FROM_OTHER_EVENT:
                    return "PMU::PIPELINE_FLUSH_FROM_OTHER_EVENT";
                case INTEGER_MULTIPLICATION_INTERLOCK:
                    return "PMU::INTEGER_MULTIPLICATION_INTERLOCK";
                case FLOATING_POINT_INTERLOCK:
                    return "PMU::FLOATING_POINT_INTERLOCK";
                case INSTRUCTION_CACHE_MISS:
                    return "PMU::INSTRUCTION_CACHE_MISS";
                case DATA_CACHE_MISS_OR_MEMORY_MAPPED_IO_ACCESS:
                    return "PMU::DATA_CACHE_MISS_OR_MEMORY_MAPPED_IO_ACCESS";
                case DATA_CACHE_WRITE_BACK:
                    return "PMU::DATA_CACHE_WRITE_BACK";
                case INSTRUCTION_TLB_MISS:
                    return "PMU::INSTRUCTION_TLB_MISS";
                case DATA_TLB_MISS:
                    return "PMU::DATA_TLB_MISS";
                case L2_TLB_MISS:
                    return "PMU::L2_TLB_MISS";
                case L1_CACHE_MISS:
                    return "PMU::L1_CACHE_MISS";
                default:
                    return "PMU::UNDEFINED_EVENT";
            }
        }

        switch (_l2pm_events[event - L2_PMU_EVENTS_START]) {
            case L2_INNER_PUTFULLDATA:
                return "L2_PMU::L2_INNER_PUTFULLDATA";
            case L2_INNER_PUTPARTIALDATA:
                return "L2_PMU::L2_INNER_PUTPARTIALDATA";
            case L2_INNER_ATOMICDATA:
                return "L2_PMU::L2_INNER_ATOMICDATA";
            case L2_INNER_GET:
                return "L2_PMU::L2_INNER_GET";
            case L2_INNER_PREFETCH_READ:
                return "L2_PMU::L2_INNER_PREFETCH_READ";
            case L2_INNER_PREFETCH_WRITE:
                return "L2_PMU::L2_INNER_PREFETCH_WRITE";
            case L2_INNER_ACQUIREBLOCK_NTOB:
                return "L2_PMU::L2_INNER_ACQUIREBLOCK_NTOB";
            case L2_INNER_ACQUIREBLOCK_NTOT:
                return "L2_PMU::L2_INNER_ACQUIREBLOCK_NTOT";
            case L2_INNER_ACQUIREBLOCK_BTOT:
                return "L2_PMU::L2_INNER_ACQUIREBLOCK_BTOT";
            case L2_INNER_ACQUIREPERM_NTOT:
                return "L2_PMU::L2_INNER_ACQUIREPERM_NTOT";
            case L2_INNER_ACQUIREPERM_BTOT:
                return "L2_PMU::L2_INNER_ACQUIREPERM_BTOT";
            case L2_INNER_RELEASE_TOB:
                return "L2_PMU::L2_INNER_RELEASE_TOB";
            case L2_INNER_RELEASE_TTON:
                return "L2_PMU::L2_INNER_RELEASE_TTON";
            case L2_INNER_RELEASE_BTON:
                return "L2_PMU::L2_INNER_RELEASE_BTON";
            case L2_INNER_RELEASEDATA_TOB:
                return "L2_PMU::L2_INNER_RELEASEDATA_TOB";
            case L2_INNER_RELEASEDATA_TTON:
                return "L2_PMU::L2_INNER_RELEASEDATA_TTON";
            case L2_INNER_RELEASEDATA_BTON:
                return "L2_PMU::L2_INNER_RELEASEDATA_BTON";
            case L2_OUTER_PROBEBLOCK_TOT:
                return "L2_PMU::L2_OUTER_PROBEBLOCK_TOT";
            case L2_OUTER_PROBEBLOCK_TOB:
                return "L2_PMU::L2_OUTER_PROBEBLOCK_TOB";
            case L2_OUTER_PROBEBLOCK_TON:
                return "L2_PMU::L2_OUTER_PROBEBLOCK_TON";
            case L2_INNER_PUTFULLDATA_HIT_L2:
                return "L2_PMU::L2_INNER_PUTFULLDATA_HIT_L2";
            case L2_INNER_PUTPARTIALDATA_HIT_L2:
                return "L2_PMU::L2_INNER_PUTPARTIALDATA_HIT_L2";
            case L2_INNER_ATOMICDATA_HIT_L2:
                return "L2_PMU::L2_INNER_ATOMICDATA_HIT_L2";
            case L2_INNER_GET_HIT_L2:
                return "L2_PMU::L2_INNER_GET_HIT_L2";
            case L2_INNER_PREFETCH_HIT_L2:
                return "L2_PMU::L2_INNER_PREFETCH_HIT_L2";
            case L2_INNER_ACQUIREBLOCK_HIT_L2:
                return "L2_PMU::L2_INNER_ACQUIREBLOCK_HIT_L2";
            case L2_INNER_ACQUIREPERM_HIT_L2:
                return "L2_PMU::L2_INNER_ACQUIREPERM_HIT_L2";
            case L2_INNER_RELEASE_HIT_L2:
                return "L2_PMU::L2_INNER_RELEASE_HIT_L2";
            case L2_INNER_RELEASEDATA_HIT_L2:
                return "L2_PMU::L2_INNER_RELEASEDATA_HIT_L2";
            case L2_OUTER_PROBE_HIT_L2:
                return "L2_PMU::L2_OUTER_PROBE_HIT_L2";
            case L2_INNER_PUTFULLDATA_HIT_L2_SHARED:
                return "L2_PMU::L2_INNER_PUTFULLDATA_HIT_L2_SHARED";
            case L2_INNER_PUTPARTIALDATA_HIT_L2_SHARED:
                return "L2_PMU::L2_INNER_PUTPARTIALDATA_HIT_L2_SHARED";
            case L2_INNER_ATOMICDATA_HIT_L2_SHARED:
                return "L2_PMU::L2_INNER_ATOMICDATA_HIT_L2_SHARED";
            case L2_INNER_GET_HIT_L2_SHARED:
                return "L2_PMU::L2_INNER_GET_HIT_L2_SHARED";
            case L2_INNER_PREFETCH_HIT_L2_SHARED:
                return "L2_PMU::L2_INNER_PREFETCH_HIT_L2_SHARED";
            case L2_INNER_ACQUIREBLOCK_HIT_L2_SHARED:
                return "L2_PMU::L2_INNER_ACQUIREBLOCK_HIT_L2_SHARED";
            case L2_INNER_ACQUIREPERM_HIT_L2_SHARED:
                return "L2_PMU::L2_INNER_ACQUIREPERM_HIT_L2_SHARED";
            case L2_OUTER_PROBE_HIT_L2_SHARED:
                return "L2_PMU::L2_OUTER_PROBE_HIT_L2_SHARED";
            case L2_OUTER_PROBE_HIT_L2_MODIFIED:
                return "L2_PMU::L2_OUTER_PROBE_HIT_L2_MODIFIED";
            case L2_OUTER_ACQUIREBLOCK_NTOB_L2_MISS:
                return "L2_PMU::L2_OUTER_ACQUIREBLOCK_NTOB_L2_MISS";
            case L2_OUTER_ACQUIREBLOCK_NTOT_L2_MISS:
                return "L2_PMU::L2_OUTER_ACQUIREBLOCK_NTOT_L2_MISS";
            case L2_OUTER_ACQUIREBLOCK_BTOT_L2_MISS:
                return "L2_PMU::L2_OUTER_ACQUIREBLOCK_BTOT_L2_MISS";
            case L2_OUTER_ACQUIREPERM_NTOT_L2_MISS:
                return "L2_PMU::L2_OUTER_ACQUIREPERM_NTOT_L2_MISS";
            case L2_OUTER_ACQUIREPERM_BTOT_L2_MISS:
                return "L2_PMU::L2_OUTER_ACQUIREPERM_BTOT_L2_MISS";
            case L2_OUTER_RELEASE_TOB_EVICTION:
                return "L2_PMU::L2_OUTER_RELEASE_TOB_EVICTION";
            case L2_OUTER_RELEASE_TON_EVICTION:
                return "L2_PMU::L2_OUTER_RELEASE_TON_EVICTION";
            case L2_OUTER_RELEASE_BTON_EVICTION:
                return "L2_PMU::L2_OUTER_RELEASE_BTON_EVICTION";
            case L2_OUTER_RELEASEDATA_TOB:
                return "L2_PMU::L2_OUTER_RELEASEDATA_TOB";
            case L2_OUTER_RELEASEDATA_TTON_DIRTY_EVICTION:
                return "L2_PMU::L2_OUTER_RELEASEDATA_TTON_DIRTY_EVICTION";
            case L2_OUTER_RELEASEDATA_BTON:
                return "L2_PMU::L2_OUTER_RELEASEDATA_BTON";
            case L2_INNER_PROBEBLOCK_TOT_CODE_MISS:
                return "L2_PMU::L2_INNER_PROBEBLOCK_TOT_CODE_MISS";
            case L2_INNER_PROBEBLOCK_TOB_LOAD_MISS:
                return "L2_PMU::L2_INNER_PROBEBLOCK_TOB_LOAD_MISS";
            case L2_INNER_PROBEBLOCK_TON_STORE_MISS:
                return "L2_PMU::L2_INNER_PROBEBLOCK_TON_STORE_MISS";
            case L2_DEMAND_MISS_HIT_MSHR_ALLOC_HINT:
                return "L2_PMU::L2_DEMAND_MISS_HIT_MSHR_ALLOC_HINT";
            case L2_CACHE_MISS:
                return "L2_PMU::L2_CACHE_MISS";
            case L1_BY_L2:
                return "L2_PMU::L1_BY_L2";
            default:
                return "L2_PMU::UNDEFINED_EVENT";
        }
    }

    static void start(Channel channel) {
        db<PMU>(TRC) << "PMU::start(c=" << channel << ")" << endl;
        if (channel <= PMU_CHANNEL_LIMIT) mcounteren(mcounteren() | 1 << channel);
    }

    static Count read(Channel channel) {
        db<PMU>(TRC) << "PMU::read(c=" << channel << ")" << endl;
        return channel <= PMU_CHANNEL_LIMIT ? mhpmcounter(channel) : l2pmcounter(channel  - (PMU_CHANNEL_LIMIT + 1));
    }

    static void write(Channel channel, Count count) {
        db<PMU>(TRC) << "PMU::write(ch=" << channel << ",ct=" << count << ")" << endl;
        if (channel <= PMU_CHANNEL_LIMIT) mhpmcounter(channel, count);
    }

    static void stop(Channel channel) {
        db<PMU>(TRC) << "PMU::stop(c=" << channel << ")" << endl;
        if(channel < FIXED)
            db<PMU>(WRN) << "PMU::stop(c=" << channel << ") : fixed channels cannot be stopped!" << endl;
        if (channel <= PMU_CHANNEL_LIMIT) mcounteren(mcounteren() & ~(1 << channel));
    }

    static void reset(Channel channel) {
        db<PMU>(TRC) << "PMU::reset(c=" << channel << ")" << endl;
        if (channel <= PMU_CHANNEL_LIMIT) write(channel, 0);
    }

    static void init() {}

protected:
    static Reg mcounteren(){ Reg reg; ASM("csrr %0, mcounteren" : "=r"(reg) :); return reg;}
    static void mcounteren(Reg reg){    ASM("csrw mcounteren, %0" : : "r"(reg));}

    static Reg mhpmevent(Channel channel) {
        Reg reg;
        switch(channel)
        {
        case 3:
            ASM("csrr %0, mhpmevent3" : : "r"(reg));
            break;
        case 4:
            ASM("csrr %0, mhpmevent4" : : "r"(reg));
            break;
        case 5:
            ASM("csrr %0, mhpmevent5" : : "r"(reg));
            break;
        case 6:
            ASM("csrr %0, mhpmevent6" : : "r"(reg));
            break;
        case 7:
            ASM("csrr %0, mhpmevent7" : : "r"(reg));
            break;
        case 8:
            ASM("csrr %0, mhpmevent8" : : "r"(reg));
            break;
        case 9:
            ASM("csrr %0, mhpmevent9" : : "r"(reg));
            break;
        case 10:
            ASM("csrr %0, mhpmevent10" : : "r"(reg));
            break;
        case 11:
            ASM("csrr %0, mhpmevent11" : : "r"(reg));
            break;
        case 12:
            ASM("csrr %0, mhpmevent12" : : "r"(reg));
            break;
        case 13:
            ASM("csrr %0, mhpmevent13" : : "r"(reg));
            break;
        case 14:
            ASM("csrr %0, mhpmevent14" : : "r"(reg));
            break;
        case 15:
            ASM("csrr %0, mhpmevent15" : : "r"(reg));
            break;
        case 16:
            ASM("csrr %0, mhpmevent16" : : "r"(reg));
            break;
        case 17:
            ASM("csrr %0, mhpmevent17" : : "r"(reg));
            break;
        case 18:
            ASM("csrr %0, mhpmevent18" : : "r"(reg));
            break;
        case 19:
            ASM("csrr %0, mhpmevent19" : : "r"(reg));
            break;
        case 20:
            ASM("csrr %0, mhpmevent20" : : "r"(reg));
            break;
        case 21:
            ASM("csrr %0, mhpmevent21" : : "r"(reg));
            break;
        case 22:
            ASM("csrr %0, mhpmevent22" : : "r"(reg));
            break;
        case 23:
            ASM("csrr %0, mhpmevent23" : : "r"(reg));
            break;
        case 24:
            ASM("csrr %0, mhpmevent24" : : "r"(reg));
            break;
        case 25:
            ASM("csrr %0, mhpmevent25" : : "r"(reg));
            break;
        case 26:
            ASM("csrr %0, mhpmevent26" : : "r"(reg));
            break;
        case 27:
            ASM("csrr %0, mhpmevent27" : : "r"(reg));
            break;
        case 28:
            ASM("csrr %0, mhpmevent28" : : "r"(reg));
            break;
        case 29:
            ASM("csrr %0, mhpmevent29" : : "r"(reg));
            break;
        case 30:
            ASM("csrr %0, mhpmevent30" : : "r"(reg));
            break;
        case 31:
            ASM("csrr %0, mhpmevent31" : : "r"(reg));
            break;
        }

        return reg;
    }

    static void mhpmevent(Reg reg, Channel channel) {
        switch (channel)
        {
        case 3:
            ASM("csrw mhpmevent3,  %0" : : "r"(reg));
            break;
        case 4:
            ASM("csrw mhpmevent4,  %0" : : "r"(reg));
            break;
        case 5:
            ASM("csrw mhpmevent5,  %0" : : "r"(reg));
            break;
        case 6:
            ASM("csrw mhpmevent6,  %0" : : "r"(reg));
            break;
        case 7:
            ASM("csrw mhpmevent7,  %0" : : "r"(reg));
            break;
        case 8:
            ASM("csrw mhpmevent8,  %0" : : "r"(reg));
            break;
        case 9:
            ASM("csrw mhpmevent9,  %0" : : "r"(reg));
            break;
        case 10:
            ASM("csrw mhpmevent10, %0" : : "r"(reg));
            break;
        case 11:
            ASM("csrw mhpmevent11, %0" : : "r"(reg));
            break;
        case 12:
            ASM("csrw mhpmevent12, %0" : : "r"(reg));
            break;
        case 13:
            ASM("csrw mhpmevent13, %0" : : "r"(reg));
            break;
        case 14:
            ASM("csrw mhpmevent14, %0" : : "r"(reg));
            break;
        case 15:
            ASM("csrw mhpmevent15, %0" : : "r"(reg));
            break;
        case 16:
            ASM("csrw mhpmevent16, %0" : : "r"(reg));
            break;
        case 17:
            ASM("csrw mhpmevent17, %0" : : "r"(reg));
            break;
        case 18:
            ASM("csrw mhpmevent18, %0" : : "r"(reg));
            break;
        case 19:
            ASM("csrw mhpmevent19, %0" : : "r"(reg));
            break;
        case 20:
            ASM("csrw mhpmevent20, %0" : : "r"(reg));
            break;
        case 21:
            ASM("csrw mhpmevent21, %0" : : "r"(reg));
            break;
        case 22:
            ASM("csrw mhpmevent22, %0" : : "r"(reg));
            break;
        case 23:
            ASM("csrw mhpmevent23, %0" : : "r"(reg));
            break;
        case 24:
            ASM("csrw mhpmevent24, %0" : : "r"(reg));
            break;
        case 25:
            ASM("csrw mhpmevent25, %0" : : "r"(reg));
            break;
        case 26:
            ASM("csrw mhpmevent26, %0" : : "r"(reg));
            break;
        case 27:
            ASM("csrw mhpmevent27, %0" : : "r"(reg));
            break;
        case 28:
            ASM("csrw mhpmevent28, %0" : : "r"(reg));
            break;
        case 29:
            ASM("csrw mhpmevent29, %0" : : "r"(reg));
            break;
        case 30:
            ASM("csrw mhpmevent30, %0" : : "r"(reg));
            break;
        case 31:
            ASM("csrw mhpmevent31, %0" : : "r"(reg));
            break;
        }
    }

    static Count mhpmcounter(Reg counter) {
        assert(counter < COUNTERS);

        Count reg = 0;

        switch(counter)
        {
        case 0:
            ASM("rdcycle  %0" : "=r"(reg) : );
            break;
#ifndef __sifive_u__
        case 1:
            ASM("rdtime  %0" : "=r"(reg) : );
            break;
#endif
        case 2:
            ASM("rdinstret  %0" : "=r"(reg) : );
            break;
        case 3:
            ASM("csrr %0, mhpmcounter3"  : "=r"(reg) : );
            break;
        case 4:
            ASM("csrr %0, mhpmcounter4"  : "=r"(reg) : );
            break;
        case 5:
            ASM("csrr %0, mhpmcounter5"  : "=r"(reg) : );
            break;
        case 6:
            ASM("csrr %0, mhpmcounter6"  : "=r"(reg) : );
            break;
        case 7:
            ASM("csrr %0, mhpmcounter7"  : "=r"(reg) : );
            break;
        case 8:
            ASM("csrr %0, mhpmcounter8"  : "=r"(reg) : );
            break;
        case 9:
            ASM("csrr %0, mhpmcounter9"  : "=r"(reg) : );
            break;
        case 10:
            ASM("csrr %0, mhpmcounter10"  : "=r"(reg) : );
            break;
        case 11:
            ASM("csrr %0, mhpmcounter11"  : "=r"(reg) : );
            break;
        case 12:
            ASM("csrr %0, mhpmcounter12"  : "=r"(reg) : );
            break;
        case 13:
            ASM("csrr %0, mhpmcounter13"  : "=r"(reg) : );
            break;
        case 14:
            ASM("csrr %0, mhpmcounter14"  : "=r"(reg) : );
            break;
        case 15:
            ASM("csrr %0, mhpmcounter15"  : "=r"(reg) : );
            break;
        case 16:
            ASM("csrr %0, mhpmcounter16"  : "=r"(reg) : );
            break;
        case 17:
            ASM("csrr %0, mhpmcounter17"  : "=r"(reg) : );
            break;
        case 18:
            ASM("csrr %0, mhpmcounter18"  : "=r"(reg) : );
            break;
        case 19:
            ASM("csrr %0, mhpmcounter19"  : "=r"(reg) : );
            break;
        case 20:
            ASM("csrr %0, mhpmcounter20"  : "=r"(reg) : );
            break;
        case 21:
            ASM("csrr %0, mhpmcounter21"  : "=r"(reg) : );
            break;
        case 22:
            ASM("csrr %0, mhpmcounter22"  : "=r"(reg) : );
            break;
        case 23:
            ASM("csrr %0, mhpmcounter23"  : "=r"(reg) : );
            break;
        case 24:
            ASM("csrr %0, mhpmcounter24"  : "=r"(reg) : );
            break;
        case 25:
            ASM("csrr %0, mhpmcounter25"  : "=r"(reg) : );
            break;
        case 26:
            ASM("csrr %0, mhpmcounter26"  : "=r"(reg) : );
            break;
        case 27:
            ASM("csrr %0, mhpmcounter27"  : "=r"(reg) : );
            break;
        case 28:
            ASM("csrr %0, mhpmcounter28"  : "=r"(reg) : );
            break;
        case 29:
            ASM("csrr %0, mhpmcounter29"  : "=r"(reg) : );
            break;
        case 30:
            ASM("csrr %0, mhpmcounter30"  : "=r"(reg) : );
            break;
        case 31:
            ASM("csrr %0, mhpmcounter31"  : "=r"(reg) : );
            break;
        }
        return reg;
    }

    static void mhpmcounter(Reg counter, Count reg) {
        assert(counter < COUNTERS);

        switch(counter)
        {
        case 3:
            ASM("csrw mhpmcounter3,  %0" : : "r"(reg));
            break;
        case 4:
            ASM("csrw mhpmcounter4,  %0" : : "r"(reg));
            break;
        case 5:
            ASM("csrw mhpmcounter5,  %0" : : "r"(reg));
            break;
        case 6:
            ASM("csrw mhpmcounter6,  %0" : : "r"(reg));
            break;
        case 7:
            ASM("csrw mhpmcounter7,  %0" : : "r"(reg));
            break;
        case 8:
            ASM("csrw mhpmcounter8,  %0" : : "r"(reg));
            break;
        case 9:
            ASM("csrw mhpmcounter9,  %0" : : "r"(reg));
            break;
        case 10:
            ASM("csrw mhpmcounter10,  %0" : : "r"(reg));
            break;
        case 11:
            ASM("csrw mhpmcounter11,  %0" : : "r"(reg));
            break;
        case 12:
            ASM("csrw mhpmcounter12,  %0" : : "r"(reg));
            break;
        case 13:
            ASM("csrw mhpmcounter13,  %0" : : "r"(reg));
            break;
        case 14:
            ASM("csrw mhpmcounter14,  %0" : : "r"(reg));
            break;
        case 15:
            ASM("csrw mhpmcounter15,  %0" : : "r"(reg));
            break;
        case 16:
            ASM("csrw mhpmcounter16,  %0" : : "r"(reg));
            break;
        case 17:
            ASM("csrw mhpmcounter17,  %0" : : "r"(reg));
            break;
        case 18:
            ASM("csrw mhpmcounter18,  %0" : : "r"(reg));
            break;
        case 19:
            ASM("csrw mhpmcounter19,  %0" : : "r"(reg));
            break;
        case 20:
            ASM("csrw mhpmcounter20,  %0" : : "r"(reg));
            break;
        case 21:
            ASM("csrw mhpmcounter21,  %0" : : "r"(reg));
            break;
        case 22:
            ASM("csrw mhpmcounter22,  %0" : : "r"(reg));
            break;
        case 23:
            ASM("csrw mhpmcounter23,  %0" : : "r"(reg));
            break;
        case 24:
            ASM("csrw mhpmcounter24,  %0" : : "r"(reg));
            break;
        case 25:
            ASM("csrw mhpmcounter25,  %0" : : "r"(reg));
            break;
        case 26:
            ASM("csrw mhpmcounter26,  %0" : : "r"(reg));
            break;
        case 27:
            ASM("csrw mhpmcounter27,  %0" : : "r"(reg));
            break;
        case 28:
            ASM("csrw mhpmcounter28,  %0" : : "r"(reg));
            break;
        case 29:
            ASM("csrw mhpmcounter29,  %0" : : "r"(reg));
            break;
        case 30:
            ASM("csrw mhpmcounter30,  %0" : : "r"(reg));
            break;
        case 31:
            ASM("csrw mhpmcounter31,  %0" : : "r"(reg));
            break;
        default:
            db<PMU>(WRN) << "PMU::mhpmcounter(c=" << counter << "): counter is read-only!" << endl;
        }
    }

    static Reg l2pmevent(Channel channel) {
        if (channel > L2_PMU_CHANNEL_LIMIT)
            db<PMU>(ERR) << "PMU::l2pmevent(c=" << channel<< "): Tried to read beyond the maximum offset!" << endl;

        return reg_l2_pmu(L2_PMU_EVENT_CONTROL + (channel * 8));
    }

    static void l2pmevent(Reg reg, Channel channel) {
        if (channel > L2_PMU_CHANNEL_LIMIT)
            db<PMU>(ERR) << "PMU::l2pmevent(r=" << reg << "c=" << channel << "): Tried to write beyond the maximum offset!" << endl;

        reg_l2_pmu(L2_PMU_EVENT_CONTROL + (channel * 8)) = reg;
    }

    static Reg l2pmcounter(Channel channel) {
        if (channel > L2_PMU_CHANNEL_LIMIT)
            db<PMU>(ERR) << "PMU::l2pmcounter(c=" << channel<< "): Tried to read beyond the maximum offset!" << endl;

        return reg_l2_pmu(L2_PMU_COUNTERS + (channel * 8));
    }

    static volatile Reg &reg_l2_pmu(Reg o) {
        return reinterpret_cast<volatile Reg *>(Memory_Map::L2_CACHE_BASE)[o / sizeof(Reg)];
    }

protected:
    static const Event _l2pm_events[EVENTS];
};

class PMU: public RV64_PMU
{
    friend class CPU;

private:
    typedef RV64_PMU Engine;

public:
    using Engine::CHANNELS;
    using Engine::FIXED;
    using Engine::EVENTS;

    using Engine::Event;
    using Engine::Count;
    using Engine::Channel;

public:
    PMU() {}

    using Engine::config;
    using Engine::read;
    using Engine::write;
    using Engine::start;
    using Engine::stop;
    using Engine::reset;

private:
    static void init() { Engine::init(); }
};

__END_SYS

#endif

