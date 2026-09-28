// EPOS DVFS Mediator Common package

#ifndef __dvfs_h
#define __dvfs_h

#include <system/config.h>

__BEGIN_SYS

class DVFS_Common {
  protected:
    DVFS_Common() {}

  public:
    static void init() {};
    static int dvfs_level() { return 0; }
    static void dvfs_level(int level, bool force_vdd_change = false) {}
    static void dvfs_frequency(Hertz frequency, bool force_vdd_change = false) {}
};

__END_SYS

#endif

#if defined(__DVFS_H) && !defined(__dvfs_common_only__)
#include __DVFS_H
#endif
