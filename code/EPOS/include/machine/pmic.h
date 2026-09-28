// EPOS PMIC Mediator Common package

#ifndef __pmic_h
#define __pmic_h

#include <system/config.h>

__BEGIN_SYS

class PMIC_Common {
  public:
    typedef unsigned int Milivolts;

  protected:
    PMIC_Common() {}

  public:
    static unsigned int cpu_voltage();
    static void cpu_voltage(Milivolts voltage, bool force = false);
};

__END_SYS

#endif

#if defined(__PMIC_H) && !defined(__pmic_common_only__)
#include __PMIC_H
#endif
