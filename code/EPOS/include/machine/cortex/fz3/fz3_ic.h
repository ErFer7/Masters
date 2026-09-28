// EPOS Xilinx Ultrascale+ ZU3EG - FZ3 - (ARM Cortex-A53) IC Mediator Declarations

#ifndef __fz3_ic_h
#define __fz3_ic_h

#include <machine/cortex/engine/cortex_a53/gic.h>
#include <system/memory_map.h>

__BEGIN_SYS

class IC_Engine: public IC_Common
{
public:
    // Interrupts
    static const unsigned int EXCS = CPU::EXCEPTIONS;
    static const unsigned int IRQS = GIC::IRQS;
    static const unsigned int INTS = EXCS + IRQS;
    static const unsigned int INT_MASK = GIC::INT_MAX - 1;

    enum {
        INT_PREFETCH_ABORT      = CPU::EXC_PREFETCH_ABORT,
        INT_DATA_ABORT          = CPU::EXC_DATA_ABORT,
        INT_PC_ALIGNMENT_ABORT  = CPU::EXC_PC_ALIGNMENT_ABORT,
        INT_SP_ALIGNMENT_ABORT  = CPU::EXC_SP_ALIGNMENT_ABORT,

        INT_TIMER0              = EXCS + GIC::IRQ_GLOBAL_TIMER, //IRQ_GLOBAL_TIMER,
        INT_SYS_TIMER           = EXCS + GIC::IRQ_GLOBAL_TIMER, //IRQ_GLOBAL_TIMER,
        INT_USR_TIMER           = INT_TIMER0,
        INT_TSC_TIMER           = INT_TIMER0,
        INT_GPIOA               = EXCS + GIC::IRQ_GPIO,
        INT_GPIOB               = EXCS + GIC::IRQ_GPIO,
        INT_GPIOC               = EXCS + GIC::IRQ_GPIO,
        INT_GPIOD               = EXCS + GIC::IRQ_GPIO,
        INT_UART0               = EXCS + GIC::IRQ_UART0,
        INT_UART1               = EXCS + GIC::IRQ_UART1,
        INT_USB0                = EXCS + GIC::IRQ_USB0,
        INT_SPI                 = EXCS + GIC::IRQ_SPI0,
        INT_PMU                 = EXCS + GIC::IRQ_PMU0,
        INT_RESCHEDULER         = EXCS + GIC::IRQ_SOFTWARE1
    };

public:
    static void enable() { 
        gic_distributor()->enable();
    }

    static void enable(Interrupt_Id i) {
        if((i >= EXCS) && (i <= INTS)) {
            gic_distributor()->enable(int2irq(i));
        }
    }

    static void disable() { gic_distributor()->disable(); }

    static void disable(unsigned int i) {
        if((i >= EXCS) && (i <= INTS))
            gic_distributor()->disable(int2irq(i));
    }

    static Interrupt_Id int_id() {
        Interrupt_Id id = gic_cpu()->int_id();
        if (id == INT_MASK) {
            id = CPU::esr_el1() >> CPU::EC_OFFSET;
            db<Setup>(WRN)<< "id of exc = " << id << endl; 
            if(id) {
                if((id & CPU::EXC_PREFETCH_ABORT) == id) return INT_PREFETCH_ABORT;
                if((id & CPU::EXC_DATA_ABORT) == id) return INT_DATA_ABORT;
                if((id & CPU::EXC_DATA_ABORT) == id) return INT_DATA_ABORT;
                if((id & CPU::EXC_PC_ALIGNMENT_ABORT) == id) return INT_PC_ALIGNMENT_ABORT;
                if((id & CPU::EXC_SP_ALIGNMENT_ABORT) == id) return INT_SP_ALIGNMENT_ABORT;
            }
            return CPU::EXC_UNKNOWN;
        } else
            return irq2int(id);
    }

    static Interrupt_Id irq2int(Interrupt_Id i) { return i + EXCS; }
    static Interrupt_Id int2irq(Interrupt_Id i) { return i - EXCS; }

    static void ipi(unsigned int cpu, Interrupt_Id id) { 
        gic_distributor()->send_sgi(cpu, int2irq(id)); 
    }

    static void init() { 
        gic_distributor()->init();
        gic_cpu()->init();
    };

private:
    static GIC_CPU * gic_cpu() { return reinterpret_cast<GIC_CPU *>(Memory_Map::GICC_BASE); }
    static GIC_Distributor * gic_distributor() { return reinterpret_cast<GIC_Distributor *>(Memory_Map::GICD_BASE); }
};

__END_SYS

#endif
