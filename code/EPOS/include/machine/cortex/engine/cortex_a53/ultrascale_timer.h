// EPOS ARM Cortex-A53 Xilinx Ultrascale+ ZU3EG - FZ3 - Timer Mediator Declarations

#ifndef __ultrascale_timer_h
#define __ultrascale_timer_h

#include <architecture/cpu.h>
#define __timer_common_only__
#include <machine/timer.h>
#undef __timer_common_only__

__BEGIN_SYS

// Extracted from https://docs.xilinx.com/r/en-US/ug1085-zynq-ultrascale-trm/System-Timer
// More details in https://developer.arm.com/documentation/ddi0500/j/Generic-Timer/Generic-Timer-register-summary/AArch64-Generic-Timer-register-summary?lang=en
class Physical_Timer : public Timer_Common
{
    // This is a hardware object

private:
    typedef CPU::Reg64 Reg;
    typedef CPU::Reg32 Reg32;
    typedef IC_Common::Interrupt_Id Interrupt_Id;

public:
    typedef CPU::Reg64 Count;

    static const unsigned int CLOCK = 200000000;

    // USEFUL BITS
    enum {                                      // Description
        PT_EN       = 0x1 << 0,     // Enable Bit
        PT_IMASK    = 0x1 << 1,     // Timer Int Mask Bit
        PT_STATUS   = 0x1 << 2,     // Timer Status Bit
        PT_REG_MASK = 0x7           // first 3 bits - 0b111
    };

public:
    void config(unsigned int unit, const Count & count) {
        Count c = this->count();
        c += count;
        ASM("msr cntp_cval_el0, %0" : : "r"(c) : );
    }

    Count count() {
        Count t;
        ASM("mrs    %0, cntpct_el0" : "=r"(t) : : );
        return t;
    }

    void enable() {
        Reg32 ctl;
        ASM("mrs %0, cntp_ctl_el0" : "=r"(ctl) : : );
        ctl = PT_EN;
        ASM("msr cntp_ctl_el0, %0" : : "r"(ctl) : );
    }
    
    void disable() {
        Reg32 ctl;
        ASM("mrs %0, cntp_ctl_el0" : "=r"(ctl) : : );
        ctl &= PT_REG_MASK - PT_EN;
        ASM("msr cntp_ctl_el0, %0" : : "r"(ctl) : );
    }

    void eoi() {} // TODO: implement at first need

    void set(const Count & count) {
        ASM("msr cntp_cval_el0, %0" : : "r"(count) : );
    }

    Hertz clock() { return CLOCK; }
};

class Virtual_Timer : public Timer_Common
{
private:
    typedef CPU::Reg Reg;
    typedef CPU::Reg32 Reg32;
    typedef IC_Common::Interrupt_Id Interrupt_Id;

public:
    typedef CPU::Reg Count;

    static const unsigned int CLOCK = 200000000;

    // USEFUL BITS
    enum {                                      // Description
        PT_EN       = 0x1 << 0,     // Enable Bit
        PT_IMASK    = 0x1 << 1,     // Timer Int Mask Bit
        PT_STATUS   = 0x1 << 2,     // Timer Status Bit
        PT_REG_MASK = 0x7           // first 3 bits - 0b111
    };

    // check cntv_tval_el0 to get the difference between cntvct_el0 and cntv_cval_el0

public:
    void config(unsigned int unit, const Count & count) {
        Count c = this->count();
        c += count;
        ASM("msr cntv_cval_el0, %0" : : "r"(c) : );
    }

    Count count() {
        Count t;
        ASM("mrs    %0, cntvct_el0" : "=r"(t) : : );
        return t;
    }

    void enable() {
        Reg32 ctl;
        ASM("mrs %0, cntv_ctl_el0" : "=r"(ctl) : : );
        ctl = PT_EN;
        ASM("msr cntv_ctl_el0, %0" : : "r"(ctl) : );
    } // TODO: implement at first need
    
    void disable() {
        Reg32 ctl;
        ASM("mrs %0, cntv_ctl_el0" : "=r"(ctl) : : );
        ctl &= PT_REG_MASK - PT_EN;
        ASM("msr cntv_ctl_el0, %0" : : "r"(ctl) : );
    }

    void eoi() {} // TODO: implement at first need

    void set(const Count & count) {
        ASM("msr cntv_cval_el0, %0" : : "r"(count) : );
    }

    Hertz clock() { return CLOCK; }
};

__END_SYS

#endif
