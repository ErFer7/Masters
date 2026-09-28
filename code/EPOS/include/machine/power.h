// EPOS Power Management Unit Mediator Common package

#ifndef __power_h
#define __power_h

#include <system/config.h>

__BEGIN_SYS

class Power_Common {
   protected:
    Power_Common() {}
};

__END_SYS

#endif

#if defined(__Power_H) && !defined(__power_common_only__)
#include __Power_H
#endif
