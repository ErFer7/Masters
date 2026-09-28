// EPOS VisionFive2 (RISC-V) Clock Tree Mediator Declarations

#pragma once

#include <architecture/cpu.h>
#include <machine/clock_tree.h>
#include <system/config.h>
#include <system/memory_map.h>
#include <utility/debug.h>

__BEGIN_SYS

extern OStream kout;

class Clock_Tree : Clock_Tree_Common {
    friend class DVFS;

   private:
    typedef CPU::Reg8 Reg8;
    typedef CPU::Reg16 Reg16;
    typedef CPU::Reg32 Reg32;
    typedef CPU::Reg64 Reg64;

    static const unsigned long sys_crg_base = Memory_Map::SYS_CRG_BASE;
    static const unsigned long aon_crg_base = Memory_Map::SYS_AON_BASE;
    static const unsigned long sys_con_base = Memory_Map::SYS_CON_BASE;

    static const Hertz osc = Traits<Machine>::OSC;

    // Offsets
    enum SysCrg {
        CPU_ROOT = 0x000,
        CPU_DIV = 0x004,
        DDR_BUS = 0x0ac,
        GMAC5_AXI_64_AHB = 0x184,
        GMAC5_AXI_64_AXI = 0x188,
        GMAC_SOURCE = 0x18c,
        GMAC1_GTX = 0x190,
        GMAC1_RMII_RTX = 0x194,
        GMAC5_AXI_64_PTP = 0x198,
        GMAC5_AXI_64_RX = 0x19c,
        GMAC5_AXI_64_RX_INVERTER = 0x1a0,
        GMAC5_AXI64_TX = 0x1a4,
        GMAC5_AXI_64_TX_INVERTER = 0x1a8,
        GMAC1_GTXC = 0x1ac,
        GMAC0_GTX = 0x1b0,
        GMAC0_PTP = 0x1b4,
        GMAC_PHY = 0x1b8,
        GMAC0_GTXC = 0x1bc,
        CAN0_CTRL_CLK_APB = 0x1cc,
        CAN0_CTRL_CLK_TIMER = 0x1d0,
        CAN0_CTRL_CLK_CORE = 0x1d4,
        TIMER_APB = 0x1f0,
        TIMER0 = 0x1f4,
        TIMER1 = 0x1f8,
        TIMER2 = 0x1fc,
        TIMER3 = 0x200,
        TEMPERATURE_SENSOR_APB = 0x204,
        TEMPERATURE_SENSOR = 0x208,
        RESET2_ADDRESS_SELECTOR = 0x300,
        RESET3_ADDRESS_SELECTOR = 0x304,
    };

    enum AonCrg {
        AHB_GMAC5_CLOCK = 0x08,
        AXI_GMAC5_CLOCK = 0x0c,
        GMAC0_RMII_RTX = 0x10,
        GMAC5_AXI64_CLOCK_TRANSMITTER = 0x14,
        GMAC5_AXI64_CLOCK_TRANSMISSION_INVERTER = 0x18,
        GMAC5_AXI64_CLOCK_RECEIVER = 0x1c,
        GMAC5_AXI64_CLOCK_RECEIVING_INVERTER = 0x20,
        RESET_ASSERT_ADDRESS_SELECTOR = 0x38,
    };

    enum SysSysCon {
        SAIF_SYSCFG_24 = 0x18,
        SAIF_SYSCFG_28 = 0x1c,
        SAIF_SYSCFG_32 = 0x20,
        SAIF_SYSCFG_36 = 0x24,
        SAIF_SYSCFG_40 = 0x28,
        SAIF_SYSCFG_44 = 0x2c,
        SAIF_SYSCFG_48 = 0x30,
        SAIF_SYSCFG_52 = 0x34,
    };

    // Bits
    enum ClockBit {
        CLOCK_ENABLE = 1 << 31,
    };

    enum ResetBit {
        // Reset 3
        RSTN_U0_CAN_CTRL_APB = 1 << 15,
        RSTN_U0_CAN_CTRL_CORE = 1 << 16,
        RSTN_U0_CAN_CTRL_TIMER = 1 << 17,
        TIMER_RSTN_APB = 1 << 21,
        TIMER_RSTN_TIMER0 = 1 << 22,
        TIMER_RSTN_TIMER1 = 1 << 23,
        TIMER_RSTN_TIMER2 = 1 << 24,
        TIMER_RSTN_TIMER3 = 1 << 25,
        TEMP_SENSOR_RSTN_APB = 1 << 27,
        TEMP_SENSOR_RSTN_TEMP = 1 << 28,
    };

   public:
    static void init() {
        divisor(CAN0_CTRL_CLK_CORE, 15);

        enable(sys_crg_base, CAN0_CTRL_CLK_APB);
        enable(sys_crg_base, CAN0_CTRL_CLK_TIMER);
        enable(sys_crg_base, CAN0_CTRL_CLK_CORE);

        for (int offset = GMAC5_AXI_64_AHB; offset <= GMAC0_GTXC; offset += 4) {
            enable(sys_crg_base, offset);
        }

        for (int offset = AHB_GMAC5_CLOCK; offset <= GMAC5_AXI64_CLOCK_RECEIVING_INVERTER; offset += 4) {
            enable(aon_crg_base, offset);
        }

        enable(sys_crg_base, TIMER_APB);
        enable(sys_crg_base, TIMER0);
        enable(sys_crg_base, TIMER1);
        enable(sys_crg_base, TIMER2);
        enable(sys_crg_base, TIMER3);

        enable(sys_crg_base, TEMPERATURE_SENSOR_APB);
        enable(sys_crg_base, TEMPERATURE_SENSOR);

        // TODO: Use the improved reset
        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) |= RSTN_U0_CAN_CTRL_APB;
        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) &= ~RSTN_U0_CAN_CTRL_APB;

        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) |= RSTN_U0_CAN_CTRL_CORE;
        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) &= ~RSTN_U0_CAN_CTRL_CORE;

        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) |= RSTN_U0_CAN_CTRL_TIMER;
        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) &= ~RSTN_U0_CAN_CTRL_TIMER;

        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) |= TIMER_RSTN_APB;
        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) &= ~TIMER_RSTN_APB;

        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) |= TIMER_RSTN_TIMER0;
        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) &= ~TIMER_RSTN_TIMER0;

        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) |= TIMER_RSTN_TIMER1;
        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) &= ~TIMER_RSTN_TIMER1;

        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) |= TIMER_RSTN_TIMER2;
        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) &= ~TIMER_RSTN_TIMER2;

        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) |= TIMER_RSTN_TIMER3;
        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) &= ~TIMER_RSTN_TIMER3;

        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) |= TEMP_SENSOR_RSTN_APB;
        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) &= ~TEMP_SENSOR_RSTN_APB;

        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) |= TEMP_SENSOR_RSTN_TEMP;
        reg(sys_crg_base, RESET3_ADDRESS_SELECTOR) &= ~TEMP_SENSOR_RSTN_TEMP;

        // reg(sys_crg_base, RESET2_ADDRESS_SELECTOR) |= 0xC;
        // reg(sys_crg_base, RESET2_ADDRESS_SELECTOR) &= ~0xC;

        // reg(aon_crg_base, RESET_ASSERT_ADDRESS_SELECTOR) |= 0x3;
        // reg(aon_crg_base, RESET_ASSERT_ADDRESS_SELECTOR) &= ~0x3;
    }

    static Hertz pll(const unsigned int &pll) {
        if (pll > 2) return 0;

        SysSysCon dacpd_dsmpd_reg = SysSysCon(0);
        SysSysCon fbdiv_reg = SysSysCon(0);
        SysSysCon frac_postdiv1_reg = SysSysCon(0);
        SysSysCon prediv_reg = SysSysCon(0);

        unsigned int dacpd_bit = 0;
        unsigned int dsmpd_bit = 0;
        unsigned int fbdiv_mask_low = 0;
        unsigned int fbdiv_mask_high = 0;

        switch (pll) {
            case 0:
                dacpd_dsmpd_reg = SAIF_SYSCFG_24;
                dacpd_bit = 24;
                dsmpd_bit = 25;
                fbdiv_reg = SAIF_SYSCFG_28;
                fbdiv_mask_low = 0;
                frac_postdiv1_reg = SAIF_SYSCFG_32;
                prediv_reg = SAIF_SYSCFG_36;
                break;
            case 1:
                dacpd_dsmpd_reg = SAIF_SYSCFG_36;
                dacpd_bit = 15;
                dsmpd_bit = 16;
                fbdiv_reg = SAIF_SYSCFG_36;
                fbdiv_mask_low = 17;
                frac_postdiv1_reg = SAIF_SYSCFG_40;
                prediv_reg = SAIF_SYSCFG_44;
                break;
            case 2:
                dacpd_dsmpd_reg = SAIF_SYSCFG_44;
                dacpd_bit = 15;
                dsmpd_bit = 16;
                fbdiv_reg = SAIF_SYSCFG_44;
                fbdiv_mask_low = 17;
                frac_postdiv1_reg = SAIF_SYSCFG_48;
                prediv_reg = SAIF_SYSCFG_52;
                break;
            default:
                break;
        }

        fbdiv_mask_high = fbdiv_mask_low + 11;

        Reg32 dacpd_dsmpd = reg(sys_con_base, dacpd_dsmpd_reg);

        Reg32 dacpd = bit_masked(dacpd_dsmpd, dacpd_bit);
        Reg32 dsmpd = bit_masked(dacpd_dsmpd, dsmpd_bit);

        Reg32 fbdiv = reg_masked_read(sys_con_base, fbdiv_reg, fbdiv_mask_low, fbdiv_mask_high);

        Reg32 saif_syscfg = reg(sys_con_base, frac_postdiv1_reg);
        Reg32 postdiv1_shift = masked(saif_syscfg, 28, 29);
        Reg32 postdiv1 = 1 << postdiv1_shift;

        Reg32 prediv = reg_masked_read(sys_con_base, prediv_reg, 0, 5);

        if (dacpd == 1 && dsmpd == 1) {
            return (osc * fbdiv) / (prediv * postdiv1);
        }

        Reg32 frac = masked(saif_syscfg, 0, 23);

        if (dacpd == 0 && dsmpd == 0) {
            Reg64 numerator = (static_cast<Reg64>(fbdiv) << 24) + static_cast<Reg64>(frac);

            return (osc * numerator) / (static_cast<Reg64>(prediv) * static_cast<Reg64>(postdiv1) * (1ULL << 24));
        }

        return 0;
    }

    static void pll(const unsigned int &pll, const unsigned int &rate) {
        if (pll != 0 || rate != 1500000000) {  // NOTE: Only some rates for PLL 0 are allowed for now
            return;
        }

        reg_bit_write(sys_con_base, SAIF_SYSCFG_32, 1, 27);  // PD 1
        reg_bit_write(sys_con_base, SAIF_SYSCFG_24, 1, 24);  // DACPD 1
        reg_bit_write(sys_con_base, SAIF_SYSCFG_24, 1, 25);  // DSMPD 1

        reg_masked_write(sys_con_base, SAIF_SYSCFG_36, 2, 0, 5);     // PREDIV 2
        reg_masked_write(sys_con_base, SAIF_SYSCFG_28, 125, 0, 11);  // FBDIV 125
        reg_masked_write(sys_con_base, SAIF_SYSCFG_32, 0, 28, 29);   // POSTDIV 1

        reg_bit_write(sys_con_base, SAIF_SYSCFG_32, 0, 27);  // PD 0
    }

    static Hertz cpu_clock() {
        Reg8 clk_mux_sel = reg_masked_read(sys_crg_base, CPU_ROOT, 24, 29);

        Hertz base_clock = clk_mux_sel == 1U ? pll(0) : osc;

        Reg32 clk_cpu_div = reg_masked_read(sys_crg_base, CPU_DIV, 0, 23);

        return base_clock / clk_cpu_div;
    }

    static Hertz ddr_clock() {
        Reg8 clk_mux_sel = reg_masked_read(sys_crg_base, DDR_BUS, 24, 29);

        if (clk_mux_sel == 0) {
            return osc / 2;
        }

        return pll(1) >> clk_mux_sel;
    }

   private:
    inline static void cpu_mux(Reg8 mux) { reg_masked_write(sys_crg_base, CPU_ROOT, mux, 24, 29); }

    inline static void cpu_div(Reg8 div) { reg_masked_write(sys_crg_base, CPU_DIV, div, 0, 23); }

    static void enable(unsigned long base, unsigned int clock) { reg(base, clock) |= CLOCK_ENABLE; }

    static void divisor(unsigned int div, unsigned int value) {
        // TODO: Replace with a masked write
        reg(sys_crg_base, div) &= ~0xFFFFFF;
        reg(sys_crg_base, div) |= value;
    }

    static volatile Reg32 &reg(unsigned long base, unsigned int offset) {
        return reinterpret_cast<volatile Reg32 *>(base)[offset / sizeof(Reg32)];
    }

    static constexpr Reg32 mask(unsigned int mask_low = 0, unsigned int mask_high = 31) {
        return ((1ULL << (mask_high - mask_low + 1)) - 1) << mask_low;
    }

    static inline Reg32 bit_masked(Reg32 value, unsigned int bit) { return (value & (1 << bit)) >> bit; }

    static inline Reg32 masked(Reg32 value, unsigned int mask_low = 0, unsigned int mask_high = 31) {
        return (value & mask(mask_low, mask_high)) >> mask_low;
    }

    // TODO: Use reg_read_masked
    static volatile Reg32 reg_masked_read(unsigned long base,
                                          unsigned int offset,
                                          unsigned int mask_low = 0,
                                          unsigned int mask_high = 31) {
        volatile Reg32 *address = &reinterpret_cast<volatile Reg32 *>(base)[offset / sizeof(Reg32)];

        return masked(*address, mask_low, mask_high);
    }

    static volatile Reg32 reg_bit_read(unsigned long base, unsigned int offset, unsigned int bit) {
        volatile Reg32 *address = &reinterpret_cast<volatile Reg32 *>(base)[offset / sizeof(Reg32)];

        return bit_masked(*address, bit);
    }

    static void reg_masked_write(unsigned long base,
                                 unsigned int offset,
                                 Reg32 value,
                                 unsigned int mask_low = 0,
                                 unsigned int mask_high = 31) {
        const unsigned int mask_value = mask(mask_low, mask_high);

        volatile Reg32 *address = &reinterpret_cast<volatile Reg32 *>(base)[offset / sizeof(Reg32)];

        Reg32 current = *address;
        current &= ~mask_value;
        current |= (value << mask_low) & mask_value;
        *address = current;
    }

    static void reg_bit_write(unsigned long base, unsigned int offset, Reg32 value, unsigned int bit) {
        volatile Reg32 *address = &reinterpret_cast<volatile Reg32 *>(base)[offset / sizeof(Reg32)];

        Reg32 current = *address;
        current &= ~(1 << bit);
        current |= value << bit;
        *address = current;
    }
};

__END_SYS
