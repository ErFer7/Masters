#pragma once

#include "architecture/cpu.h"
#include "machine/riscv/visionfive2/visionfive2_memory_map.h"
#include "system/traits.h"

__BEGIN_SYS

extern OStream kout;

class Temperature_Sensor {
    typedef CPU::Reg32 Reg32;
    typedef unsigned long long Milicelsius;

    enum { RESET = 1, POWERDOWN = 1 << 1, RUN = 1 << 2 };

    enum { OUTPUT_MASK = 0xfff0000 };

    const static unsigned long long CONVERSION_A = 237500ULL;
    const static unsigned long long CONVERSION_B = 4094ULL;
    const static unsigned long long CONVERSION_C = 81100ULL;

   public:
    static void init() {
        reg() |= POWERDOWN;
        for (volatile int i = 0; i < 1000; i++);

        reg() = 0;
        for (volatile int i = 0; i < 10000; i++);

        reg() = RESET;
        for (volatile int i = 0; i < 1000; i++);

        reg() |= RUN;
        for (volatile int i = 0; i < 1000; i++);
    }

    static Milicelsius get_temperature() {
        return (((reg() & OUTPUT_MASK) >> 16) * CONVERSION_A / CONVERSION_B - CONVERSION_C);
    }

   private:
    static volatile Reg32 &reg() { return *reinterpret_cast<volatile Reg32 *>(Memory_Map::TEMP_BASE); }
};

__END_SYS
