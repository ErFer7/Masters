// EPOS TSC Mediator Common Package

#ifndef __tsc_h
#define __tsc_h

#include <system/config.h>

__BEGIN_SYS

class TSC_Common
{
public:
    typedef unsigned long long Time_Stamp;

protected:
    TSC_Common() {}

public:
    static Microsecond time(Time_Stamp ts, Hertz frequency) { return (Microsecond(ts) * 1000000ULL + (frequency / 2)) / frequency; }
    static Time_Stamp ts(Microsecond time, Hertz frequency) { return (Time_Stamp(time) * frequency + 500000ULL) / 1000000ULL; }
};

__END_SYS

#endif

#if defined(__TSC_H) && !defined(__tsc_common_only__)
#include __TSC_H
#endif
