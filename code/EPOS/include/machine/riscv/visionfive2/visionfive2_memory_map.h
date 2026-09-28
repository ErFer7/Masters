// EPOS VisionFive 2 (RISC-V) Memory Map

#ifndef __riscv_visionfive2_memory_map_h
#define __riscv_visionfive2_memory_map_h

#include <system/memory_map.h>

__BEGIN_SYS

struct Memory_Map {
  private:
    static const bool multitask = Traits<System>::multitask;

  public:
    enum : unsigned long {
        NOT_USED = Traits<Machine>::NOT_USED,

        // Physical Memory
        RAM_BASE        = Traits<Machine>::RAM_BASE,
        RAM_TOP         = Traits<Machine>::RAM_TOP,
        MIO_BASE        = Traits<Machine>::MIO_BASE,
        MIO_TOP         = Traits<Machine>::MIO_TOP,
        LAST_PAGE       = RAM_TOP + 1 - 4096,
        M_INT_HANDLER   = LAST_PAGE, // the last page is used by the _int_m2s() machine mode interrupt forwarder installed by SETUP before going into supervisor mode; code and stack share the same page, with code at the bottom and the stack at the top
        FLAT_MEM_MAP    = M_INT_HANDLER - 4096, // in LIBRARY mode, a single-level mapping of the whole memory space is used
        BOOT_STACK      = FLAT_MEM_MAP - Traits<Machine>::STACK_SIZE * Traits<Build>::CPUS, // will be used as the stack's base, not the stack pointer
        FREE_BASE       = RAM_BASE,
        FREE_TOP        = BOOT_STACK - 1,

        // Memory-mapped devices
        BIOS_BASE       = 0x00001000,  // BIOS ROM
        TEST_BASE       = 0x00100000,  // SiFive test engine
        RTC_BASE        = 0x00101000,  // Goldfish RTC
        CLINT_BASE      = 0x02000000,  // SiFive CLINT
        TIMER_BASE      = 0x02004000,  // CLINT Timer
        L2_CACHE_BASE   = 0x02010000,  // VisionFive2 cache controller
        PLIC_BASE       = 0x0c000000,  // SiFive PLIC
        UART0_BASE      = 0x10000000,  // SiFive UART
        PRCI_BASE       = 0x10000000,  // VisionFive2 Power, Reset, Clock, Interrupt
        SPI0_BASE       = 0x10060000,  // VisionFive2 QSPI 0
        SPI1_BASE       = 0x10070000,  // VisionFive2 QSPI 1
        SPI2_BASE       = 0x10080000,  // VisionFive2 QSPI 2
        I2C5_BASE       = 0x12050000,  // VisionFive2 I2C5
        TEMP_BASE       = 0x120e0000,  // VisionFive2 temperature sensor
        SYS_CRG_BASE    = 0x13020000,  // VisionFive2 System CRG
        SYS_CON_BASE    = 0x13030000,  // VisionFive2 System CON
        ETH_BASE        = 0x16030000,  // VisionFive2 Ethernet
        SYS_AON_BASE    = 0x17000000,  // VisionFive2 System AON
        POWER_BASE      = 0x1703FFFF,  // VisionFive2 Power Management Unit
        FLASH_BASE      = 0x20000000,  // Virt / VisionFive2 Flash

        // Physical Memory at Boot
        BOOT            = Traits<Machine>::BOOT,
        IMAGE           = Traits<Machine>::IMAGE,
        SETUP           = Traits<Machine>::SETUP,

        // Logical Address Space
        APP_LOW         = Traits<Machine>::APP_LOW,
        APP_HIGH        = Traits<Machine>::APP_HIGH,
        APP_CODE        = Traits<Machine>::APP_CODE,
        APP_DATA        = Traits<Machine>::APP_DATA,

        PHY_MEM         = Traits<Machine>::PHY_MEM,

        IO              = Traits<Machine>::IO,

        SYS             = Traits<Machine>::SYS,
        SYS_CODE        = multitask ? SYS + 0x00000000 : NOT_USED,
        SYS_INFO        = multitask ? SYS + 0x00100000 : NOT_USED,
        SYS_PT          = multitask ? SYS + 0x00101000 : NOT_USED,
        SYS_PD          = multitask ? SYS + 0x00102000 : NOT_USED,
        SYS_DATA        = multitask ? SYS + 0x00103000 : NOT_USED,
        SYS_STACK       = multitask ? SYS + 0x00200000 : NOT_USED,
        INIT            = multitask ? SYS_STACK : NOT_USED,
        SYS_HEAP        = multitask ? SYS + 0x00400000 : NOT_USED,
        SYS_HIGH        = multitask ? SYS + 0x5fffffff : NOT_USED
    };
};

__END_SYS

#endif
