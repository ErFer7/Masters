// EPOS Timer Mediator Common Package

#ifndef __timer_h
#define __timer_h

#include <machine/ic.h>

__BEGIN_SYS

class Timer_Common
{
public:
    enum Channel : unsigned int {
        SCHEDULER,
        ALARM,
        USER,
        USER1 = USER,
        USER2,
        USER3,
        USER4,
        USER5
    };

    typedef long Tick;
    typedef IC_Common::Interrupt_Handler Handler;

protected:
    Timer_Common() {}

public:
    Tick read();
    Percent reset();

    void enable();
    void disable();

    PPB accuracy();
    Hertz frequency();
    void frequency(Hertz f);

    void handler(Handler handler);

    static Microsecond time(Tick ticks, Hertz frequency) { return (Microsecond(ticks) * 1000000ULL + (frequency / 2)) / frequency; }
    static Tick ticks(Microsecond time, Hertz frequency) { return (static_cast<unsigned long long>(time) * frequency + 500000ULL) / 1000000ULL; }
};

__END_SYS

#endif

#if defined(__TIMER_H) && !defined(__timer_common_only__)
#include __TIMER_H
#endif
