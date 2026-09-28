// EPOS Xilinx Ultrascale+ ZU3EG - FZ3 - (ARM Cortex-A53) Memory Map

#ifndef __fz3_memory_map_h
#define __fz3_memory_map_h

#include <machine/cortex/cortex_memory_map.h>

__BEGIN_SYS

struct Memory_Map: public Cortex_Memory_Map
{
private:
    static const bool emulated  = Traits<Build>::EXPECTED_SIMULATION_TIME;

public:
    enum : unsigned long {
        // Base addresses for memory-mapped control and I/O devices
        MBOX_COM_BASE   = NOT_USED,             // RAM memory for device-os communication (must be mapped as device by the MMU)
        MBOX_CTRL_BASE  = NOT_USED,             // BCM MailBox
        PPS_BASE        = 0xff000000,             // Private Peripheral Space
        TSC_BASE        = NOT_USED,
        TIMER0_BASE     = 0xff260000,             // System Timer (free running)
        DMA0_BASE       = NOT_USED,
        GICD_BASE       = 0xf9010000,           // 0xf9000000, but up to 0xf900ffff the memory is reserved. GICD is right after this area
        GICC_BASE       = 0xf9020000,
        GICH_BASE       = 0xf9040000,
        IC_BASE         = GICD_BASE,
        TIMER1_BASE     = NOT_USED,             // ARM Timer (frequency relative to processor frequency)
        MBOX_BASE       = NOT_USED,
        ARM_MBOX0       = NOT_USED,             // IOCtrl (MBOX 0) is also mapped on this address
        PM_BASE         = NOT_USED,             // Power Manager
        RAND_BASE       = NOT_USED,
        GPIO_BASE       = NOT_USED,
        UART_BASE       = 0xff000000,             // UART
        UART1_BASE      = 0xff010000,             // UART1
        SD0_BASE        = NOT_USED,             // Custom sdhci controller
        AUX_BASE        = NOT_USED,             // mini UART + 2 x SPI master
        SD1_BASE        = NOT_USED,             // Arasan sdhci controller
        DMA1_BASE       = NOT_USED,

        VECTOR_TABLE    = RAM_BASE + 0x8000,
        FLAT_PAGE_TABLE = (RAM_TOP - 1024 * 32 * 1024) & ~(0x3fff), // used only with No_MMU in LIBRARY mode; 32-bit: 16KB, 4096 4B entries, each pointing to 1 MB regions, thus mapping up to 4 GB; 64-bit: 16KB, 2048 8B entries, each pointing to 32 MB regions, thus mapping up to 64 GB; 16K-aligned for TTBR;
        BOOT_STACK      = FLAT_PAGE_TABLE - Traits<Build>::CPUS * Traits<Machine>::STACK_SIZE, // will be used as the stack's base, not the stack pointer

        FREE_BASE       = VECTOR_TABLE + 16 * 1024, // + (armv7 ? 4 : 16) * 1024,
        FREE_TOP        = (BOOT_STACK - 1) & ~(0x3fff),

        // Logical Address Space -- Need to be verified
        APP_LOW         = Traits<Machine>::APP_LOW,
        APP_HIGH        = Traits<Machine>::APP_HIGH,

        APP_CODE        = Traits<Machine>::APP_CODE,
        APP_DATA        = Traits<Machine>::APP_DATA,

        PHY_MEM         = Traits<Machine>::PHY_MEM,
        IO              = Traits<Machine>::IO,
        SYS             = Traits<Machine>::SYS,
        SYS_CODE        = NOT_USED,
        SYS_INFO        = NOT_USED,
        SYS_PT          = NOT_USED,
        SYS_PD          = NOT_USED,
        SYS_DATA        = NOT_USED,
        SYS_STACK       = NOT_USED,
        SYS_HEAP        = NOT_USED,
        SYS_HIGH        = NOT_USED
    };
};

__END_SYS

#endif
