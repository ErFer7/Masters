// EPOS Xilinx Ultrascale+ ZU3EG - FZ3 - (ARM Cortex-A53) Timer Mediator Declarations

#ifndef __fz3_timer_h
#define __fz3_timer_h

#define __ic_common_only__
#include <machine/ic.h>
#undef __ic_common_only__
#include <machine/timer.h>
#include <machine/cortex/engine/cortex_a53/ultrascale_timer.h>
#include <system/memory_map.h>
#include <utility/convert.h>

__BEGIN_SYS

class System_Timer_Engine: public Timer_Common
{
    friend Timer; // for init()

private:
    static const unsigned int UNIT = 0;
    static const unsigned int FREQUENCY = Traits<Timer>::FREQUENCY;

    typedef Virtual_Timer Timer_Base;
    typedef IC_Common::Interrupt_Id Interrupt_Id;

public:
    typedef Timer_Base::Count Count;

public:
    Count read() { return timer()->read(); }

    static void reset() {
        disable();
        _count = timer()->clock() / FREQUENCY;
        timer()->config(UNIT, _count);
        enable();
    }

    static void enable() { timer()->enable(); }
    static void disable() { timer()->disable(); }

    static Hertz clock() { return timer()->clock(); }

protected:
    static void eoi(Interrupt_Id id) { timer()->config(UNIT, _count); }

private:
    static void init() {
        reset();
    }

private:
    static Timer_Base * timer() { return reinterpret_cast<Timer_Base *>( Memory_Map::TIMER0_BASE ); }

    
private:
    static Count _count;
};

__END_SYS

#endif
