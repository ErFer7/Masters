#ifndef __traits_h
#define __traits_h

#include <system/config.h>

__BEGIN_SYS

template<> struct Traits<Build>: public Traits_Tokens
{
    // Basic configuration
    static const unsigned int SMOD = LIBRARY;
    static const unsigned int ARCHITECTURE = IA32;
    static const unsigned int MACHINE = PC;
    static const unsigned int MODEL = Legacy_PC;
    static const unsigned int CPUS = ((MODEL == Legacy_PC) || (MODEL == Raspberry_Pi3) || (MODEL == Realview_PBX) || (MODEL == Zynq)) ? 4 : (MODEL == SiFive_U) ?  2 : 1;
    static const unsigned int NETWORKING = STANDALONE;
    static const unsigned int EXPECTED_SIMULATION_TIME = 10000; // s (0 => not simulated)

    // Default flags
    static const bool enabled = true;
    static const bool monitored = false;
    static const bool debugged = true;
    static const bool hysterically_debugged = false;
};

// Utilities
template<> struct Traits<Debug>: public Traits<Build>
{
    static const bool error   = true;
    static const bool warning = true;
    static const bool info    = false;
    static const bool trace   = false;
};

template<> struct Traits<Lists>: public Traits<Build>
{
    static const bool debugged = hysterically_debugged;
};

template<> struct Traits<Spin>: public Traits<Build>
{
    static const bool debugged = hysterically_debugged;
};

template<> struct Traits<Heaps>: public Traits<Build>
{
    static const bool debugged = hysterically_debugged;
};

template<> struct Traits<Observers>: public Traits<Build>
{
    // Some observed objects are created before initializing the Display
    // Enabling debug may cause trouble in some Machines
    static const bool debugged = false;
};


// System Parts (mostly to fine control debugging)
template<> struct Traits<Boot>: public Traits<Build>
{
};

template<> struct Traits<Setup>: public Traits<Build>
{
};

template<> struct Traits<Init>: public Traits<Build>
{
};

template<> struct Traits<Framework>: public Traits<Build>
{
};

template<> struct Traits<Aspect>: public Traits<Build>
{
    static const bool debugged = hysterically_debugged;
};


// Mediators
template<> struct Traits<CPU>: public Traits<Build>
{
    static const unsigned int ENDIANESS         = LITTLE;
    static const unsigned int WORD_SIZE         = 32;
    static const unsigned int CLOCK             = 2000000000;
    static const bool unaligned_memory_access   = true;
};

template<> struct Traits<TSC>: public Traits<Build>
{
};

template<> struct Traits<MMU>: public Traits<Build>
{
    static const bool colorful = true;
    static const unsigned int COLORS = 8;
};

template<> struct Traits<FPU>: public Traits<Build>
{
    static const bool enabled = true;
    static const bool user_save = true;
};

template<> struct Traits<PMU>: public Traits<Build>
{
    static const bool enabled = true;
    enum { V1, V2, V3, DUO, MICRO, ATOM, SANDY_BRIDGE };
    static const unsigned int VERSION = SANDY_BRIDGE;
};


class Machine_Common;
template<> struct Traits<Machine_Common>: public Traits<Build> {};

template<> struct Traits<Machine>: public Traits<Machine_Common>
{
    static const unsigned int NOT_USED          = 0xffffffff;

    // Physical Memory
    static const unsigned int RAM_BASE          = 0x00000000;
    static const unsigned int RAM_TOP           = 0x40000000; 	// 1 GB (max 1792 MB)
    static const unsigned int MIO_BASE          = NOT_USED;	// defined by SETUP during PCI initialization (max 244 MB)
    static const unsigned int MIO_TOP           = NOT_USED;	// defined by SETUP

    // Physical Memory at Boot
    static const unsigned int BOOT              = 0x00007c00;
    static const unsigned int IMAGE             = 0x00008000;
    static const unsigned int SETUP             = 0x00100000;   // 1 MB
    static const unsigned int RAMDISK           = 0x0fa28000;   // MEMDISK-dependent
    static const unsigned int RAMDISK_SIZE      = 0x003c0000;

    // Logical Memory Map
    static const unsigned int APP_LOW           = 0x80000000;   // 2 GB
    static const unsigned int APP_HIGH          = 0xff7fffff;   // SYS - 1

    static const unsigned int APP_CODE          = APP_LOW;      // APP_CODE < APP_DATA must hold
    static const unsigned int APP_DATA          = APP_CODE + 4 * 1024 * 1024;

    static const unsigned int INIT              = 0x00200000;   // 2 MB (only used during boot)
    static const unsigned int PHY_MEM           = 0x00000000; 	// (max 1792 MB)
    static const unsigned int IO                = 0x70000000; 	// 2 GB - 256 MB  (max 244 MB)
    static const unsigned int SYS               = 0xff800000;   // 4 GB - 8 MB

    // Default Sizes and Quantities
    static const unsigned int MAX_THREADS       = 15;
    static const unsigned int STACK_SIZE        = 16 * 1024;
    static const unsigned int HEAP_SIZE         = 4 * 1024 * 1024;
};

template<> struct Traits<PCI>: public Traits<Machine_Common>
{
    static const int MAX_BUS = 16;
    static const int MAX_DEV_FN = 0xff;
    static const unsigned int MAX_REGION_SIZE = 0x04000000; // 64 MB
};

template<> struct Traits<IC>: public Traits<Machine_Common>
{
    static const bool debugged = hysterically_debugged;
};

template<> struct Traits<Timer>: public Traits<Machine_Common>
{
    static const bool debugged = hysterically_debugged;

    // Meaningful values for the PC's timer frequency range from 100 to
    // 10000 Hz. The choice must respect the scheduler time-slice, i. e.,
    // it must be higher than the scheduler invocation frequency.
    static const int FREQUENCY = 1000; // Hz
};

template<> struct Traits<RTC>: public Traits<Machine_Common>
{
    static const unsigned int EPOCH_DAY = 1;
    static const unsigned int EPOCH_MONTH = 1;
    static const unsigned int EPOCH_YEAR = 1970;
    static const unsigned int EPOCH_DAYS = 719499;
};

template<> struct Traits<EEPROM>: public Traits<Machine_Common>
{
};

template<> struct Traits<UART>: public Traits<Machine_Common>
{
    static const unsigned int UNITS = 2;

    static const unsigned int CLOCK = 1843200; // 1.8432 MHz

    static const unsigned int DEF_BAUD_RATE = 115200;
    static const unsigned int DEF_DATA_BITS = 8;
    static const unsigned int DEF_PARITY = 0; // none
    static const unsigned int DEF_STOP_BITS = 1;

    static const unsigned int COM1 = 0x3f8; // to 0x3ff, IRQ4
    static const unsigned int COM2 = 0x2f8; // to 0x2ff, IRQ3
    static const unsigned int COM3 = 0x3e8; // to 0x3ef, no IRQ
    static const unsigned int COM4 = 0x2e8; // to 0x2ef, no IRQ
};

template<> struct Traits<Serial_Display>: public Traits<Machine_Common>
{
    static const bool enabled = (Traits<Build>::EXPECTED_SIMULATION_TIME != 0);
    static const int ENGINE = UART;
    static const int UNIT = 0; // COM1
    static const int COLUMNS = 80;
    static const int LINES = 24;
    static const int TAB_SIZE = 8;
};

template<> struct Traits<Serial_Keyboard>: public Traits<Machine_Common>
{
    static const bool enabled = (Traits<Build>::EXPECTED_SIMULATION_TIME != 0);
};

template<> struct Traits<Display>: public Traits<Machine_Common>
{
    static const bool enabled = !Traits<Serial_Display>::enabled;
    static const int COLUMNS = 80;
    static const int LINES = 25;
    static const int TAB_SIZE = 8;
};

template<> struct Traits<Keyboard>: public Traits<Machine_Common>
{
    static const bool enabled = !Traits<Serial_Keyboard>::enabled;
};

template<> struct Traits<Scratchpad>: public Traits<Machine_Common>
{
    static const bool enabled = false;
    static const unsigned int ADDRESS = 0xa0000; // VGA Graphic mode frame buffer
    static const unsigned int SIZE = 96 * 1024;
};

template<> struct Traits<Ethernet>: public Traits<Machine_Common>
{
    typedef LIST<PCNet32, PCNet32> DEVICES;
    static const unsigned int UNITS = DEVICES::Length;

    static const bool enabled = (Traits<Build>::NETWORKING != STANDALONE) && (UNITS > 0);

    static const bool promiscuous = false;
};

template<> struct Traits<PCNet32>: public Traits<Ethernet>
{
    static const unsigned int UNITS = DEVICES::Count<PCNet32>::Result;
    static const bool enabled = Traits<Ethernet>::enabled && (UNITS > 0);

    static const unsigned int SEND_BUFFERS = 64; // per unit
    static const unsigned int RECEIVE_BUFFERS = 256; // per unit
};

template<> struct Traits<E100>: public Traits<Ethernet>
{
    static const unsigned int UNITS = DEVICES::Count<E100>::Result;
    static const bool enabled = Traits<Ethernet>::enabled && (UNITS > 0);

    static const unsigned int SEND_BUFFERS = 64; // per unit
    static const unsigned int RECEIVE_BUFFERS = 64; // per unit
};

template<> struct Traits<C905>: public Traits<Ethernet>
{
    static const unsigned int UNITS = DEVICES::Count<C905>::Result;
    static const bool enabled = Traits<Ethernet>::enabled && (UNITS > 0);

    static const unsigned int SEND_BUFFERS = 64; // per unit
    static const unsigned int RECEIVE_BUFFERS = 64; // per unit
};

template<> struct Traits<RTL8139>: public Traits<Ethernet>
{
    static const unsigned int UNITS = DEVICES::Count<RTL8139>::Result;
    static const bool enabled = Traits<Ethernet>::enabled && (UNITS > 0);

    static const unsigned int SEND_BUFFERS = 4; // per unit
    static const unsigned int RECEIVE_BUFFERS = 8192; // no descriptor, just a memory block of 8192 bits
};

template<> struct Traits<FPGA>: public Traits<Machine_Common>
{
    static const bool enabled = false;

    static const unsigned int DMA_BUFFER_SIZE = 64 * 1024; // 64 KB
};


// API Components
template<> struct Traits<Application>: public Traits<Build>
{
    static const unsigned int STACK_SIZE = Traits<Machine>::STACK_SIZE;
    static const unsigned int HEAP_SIZE = Traits<Machine>::HEAP_SIZE;
    static const unsigned int MAX_THREADS = Traits<Machine>::MAX_THREADS;
};

template<> struct Traits<System>: public Traits<Build>
{
    static const bool multithread = (Traits<Application>::MAX_THREADS > 1) || (CPUS > 1);
    static const bool multitask = (SMOD != LIBRARY);
    static const bool multicore = multithread && (CPUS > 1);
    static const bool multiheap = multitask || Traits<Scratchpad>::enabled;

    static const unsigned long LIFE_SPAN = 1 * YEAR; // s
    static const unsigned int DUTY_CYCLE = 1000000; // ppm

    static const bool reboot = true;

    static const unsigned int STACK_SIZE = Traits<Machine>::STACK_SIZE;
    static const unsigned int HEAP_SIZE = (Traits<Application>::MAX_THREADS + Traits<Build>::CPUS) * (multitask ? STACK_SIZE : Traits<Application>::STACK_SIZE);
};

template<> struct Traits<Task>: public Traits<Build>
{
    static const bool enabled = Traits<System>::multitask;
};

template<> struct Traits<Thread>: public Traits<Build>
{
    static const bool enabled = Traits<System>::multithread;
    static const bool smp = Traits<System>::multicore;
    static const bool trace_idle = hysterically_debugged;
    static const bool simulate_capacity = false;

    typedef IF<(CPUS > 1), PEDF, EDF>::Result Criterion;
    static const unsigned int QUANTUM = 10000; // us
};

template<> struct Traits<Scheduler<Thread>>: public Traits<Build>
{
    static const bool debugged = Traits<Thread>::trace_idle || hysterically_debugged;
};

template<> struct Traits<Synchronizer>: public Traits<Build>
{
    static const bool enabled = Traits<System>::multithread;
};

template<> struct Traits<Alarm>: public Traits<Build>
{
    static const bool visible = hysterically_debugged;
};

template<> struct Traits<SmartData>: public Traits<Build>
{
    static const unsigned char PREDICTOR = NONE;
};

template<> struct Traits<Network>: public Traits<Build>
{
    typedef LIST<> NETWORKS;

    static const unsigned int RETRIES = 3;
    static const unsigned int TIMEOUT = 10; // s

    static const bool enabled = (Traits<Build>::NETWORKING != STANDALONE) && (NETWORKS::Length > 0);
};

template<> struct Traits<ELP>: public Traits<Network>
{
    typedef Ethernet NIC_Family;
    static constexpr unsigned int NICS[] = {0}; // relative to NIC_Family (i.e. Traits<Ethernet>::DEVICES[NICS[i]]
    static const unsigned int UNITS = COUNTOF(NICS);

    static const bool enabled = Traits<Network>::enabled && (NETWORKS::Count<ELP>::Result > 0);
};

template<> struct Traits<TSTP>: public Traits<Network>
{
    typedef Ethernet NIC_Family;
    static constexpr unsigned int NICS[] = {0}; // relative to NIC_Family (i.e. Traits<Ethernet>::DEVICES[NICS[i]]
    static const unsigned int UNITS = COUNTOF(NICS);

    static const unsigned int KEY_SIZE = 16;
    static const unsigned int RADIO_RANGE = 8000; // approximated radio range in centimeters

    static const bool enabled = Traits<Network>::enabled && (NETWORKS::Count<TSTP>::Result > 0);
};

template<> struct Traits<IP>: public Traits<Network>
{
    typedef Ethernet NIC_Family;
    static constexpr unsigned int NICS[] = {0};  // relative to NIC_Family (i.e. Traits<Ethernet>::DEVICES[NICS[i]]
    static const unsigned int UNITS = COUNTOF(NICS);

    struct Default_Config {
        static const unsigned int  TYPE    = DHCP;
        static const unsigned long ADDRESS = 0;
        static const unsigned long NETMASK = 0;
        static const unsigned long GATEWAY = 0;
    };

    template<unsigned int UNIT>
    struct Config: public Default_Config {};

    static const unsigned int TTL  = 0x40; // Time-to-live

    static const bool enabled = Traits<Network>::enabled && (NETWORKS::Count<IP>::Result > 0);
};

template<> struct Traits<UDP>: public Traits<Network>
{
    static const bool checksum = true;
};

template<> struct Traits<TCP>: public Traits<Network>
{
    static const unsigned int WINDOW = 4096;
};

template<> struct Traits<DHCP>: public Traits<Network>
{
};

template<> struct Traits<Monitor>: public Traits<Build>
{
    static const bool enabled = monitored;

    static constexpr System_Event       SYSTEM_EVENTS[]                 = { ELAPSED_TIME, DEADLINE_MISSES, CPU_EXECUTION_TIME, THREAD_EXECUTION_TIME, RUNNING_THREAD };
    static constexpr Hertz              SYSTEM_EVENTS_FREQUENCIES[]     = {            1,               1,                  1,                     1,              1 }; // in Hz

    static constexpr PMU_Event          PMU_EVENTS[]                    = { INSTRUCTIONS_RETIRED, BRANCHES, CACHE_MISSES };
    static constexpr Hertz              PMU_EVENTS_FREQUENCIES[]        = {                     1,        1,            1}; // in Hz

    static constexpr Transducer_Event   TRANSDUCER_EVENTS[]             = { CPU_VOLTAGE, CPU_TEMPERATURE };
    static constexpr Hertz              TRANSDUCER_EVENTS_FREQUENCIES[] = {           1,               1 }; // in Hz
};

__END_SYS

#endif
