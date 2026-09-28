
// EPOS DVFS Mediator Common package

#pragma once

#include <architecture/cpu.h>
#include <machine/clock_tree.h>
#include <machine/dvfs.h>
#include <machine/pmic.h>
#include <system/config.h>

__BEGIN_SYS

extern OStream kout;

class DVFS : DVFS_Common {
   private:
    typedef PMIC::Milivolts Milivolts;
    typedef CPU::Reg8 Reg8;

    static const int MAX = 3;
    static const int MIN = 0;

    static constexpr Hertz FREQUENCY_LEVELS[] = {375000000, 500000000, 750000000, 1500000000};
    static constexpr Milivolts VOLTAGE_LEVELS[] = {800, 800, 800, 1040};

   protected:
    DVFS() {}

   public:
    static void init() {
        PMIC::cpu_voltage(VOLTAGE_LEVELS[MAX], true);

        Clock_Tree::cpu_div(2);
        Clock_Tree::cpu_mux(0);

        TSC::usleep(5000);
        Clock_Tree::pll(0, 1500000000);
        TSC::usleep(5000);

        Clock_Tree::cpu_mux(1);
        TSC::usleep(5000);

        Clock_Tree::cpu_div(1);

        _dvfs_level = MAX;

        db<DVFS>(INF) << "DVFS::init() => Current clock and voltage: " << Clock_Tree::cpu_clock() << "Hz, "
                      << PMIC::cpu_voltage() << "mV" << endl;
    }

    static int dvfs_level() { return _dvfs_level; }

    // TODO: Improve this
    static void dvfs_level(int level, bool force_vdd_change = false) {
        if (level == _dvfs_level && !force_vdd_change) {
            return;
        }

        Hertz frequency = FREQUENCY_LEVELS[level];
        Milivolts voltage = VOLTAGE_LEVELS[level];

        Reg8 div = FREQUENCY_LEVELS[MAX] / frequency;

        if (div == 0U) div = 1U;
        if (div > 7U) div = 7U;

        if (level > _dvfs_level) {
            scale_up(level, div, voltage, force_vdd_change);
        } else if (level < _dvfs_level) {
            scale_down(level, div, voltage, force_vdd_change);
        }
    }

    static void dvfs_frequency(Hertz frequency, bool force_vdd_change = false);

   private:
    inline static void scale_up(int level, Reg8 div, Milivolts voltage, bool force_vdd_change = false) {
        PMIC::cpu_voltage(voltage, force_vdd_change);
        Clock_Tree::cpu_div(div);

        _dvfs_level = level;
    }

    inline static void scale_down(int level, Reg8 div, Milivolts voltage, bool force_vdd_change = false) {
        Clock_Tree::cpu_div(div);
        PMIC::cpu_voltage(voltage, force_vdd_change);

        _dvfs_level = level;
    }

   private:
    static inline int _dvfs_level = -1;
};

__END_SYS
