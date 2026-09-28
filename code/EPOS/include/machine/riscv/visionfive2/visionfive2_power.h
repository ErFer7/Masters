// EPOS VisionFive 2 (RISC-V) Power Management Unit Mediator Declarations

#pragma once

#include <architecture/cpu.h>
#include <architecture/tsc.h>
#include <machine/power.h>

__BEGIN_SYS

class Power : public Power_Common {
   private:
    typedef CPU::Reg32 Reg32;

   private:
    static const unsigned char PMIC_I2C_ADDR = 0x36;

    enum Regs { SW_TURN_OFF_POWER_MODE = 0x10, SW_ENCOURAGE = 0x44, CURRENT_POWER_MODE = 0x80 };

    enum PowerMode {
        SYSTOP_POWER_MODE = 1u,
        CPU_POWER_MODE = 1u << 1,
        GPUA_POWER_MODE = 1u << 2,
        VDEC_POWER_MODE = 1u << 3,
        VOUT_POWER_MODE = 1u << 4,
        ISP_POWER_MODE = 1u << 5,
        VENC_POWER_MODE = 1u << 6
    };

    // Power mode masks
    static const Reg32 DEVICE_POWERDOWN_MASK = 0xFC;
    static const Reg32 SW_MODE_ENCOURAGE_ON = 0xFF;
    static const Reg32 SW_MODE_ENCOURAGE_DIS_LO = 0x0A;
    static const Reg32 SW_MODE_ENCOURAGE_DIS_HI = 0xA0;
    static const Reg32 SYSTOP_CPU_PD_MASK = 0x03;

    static const unsigned int TIMEOUT_COUNT = 100000;

   public:
    Power() {}

    static unsigned int shutdown_power_domain() {
        unsigned int ret = 0;

        Reg32 current_power_mode = reg(CURRENT_POWER_MODE);
        current_power_mode &= DEVICE_POWERDOWN_MASK;

        if (current_power_mode != 0) {
            reg(SW_TURN_OFF_POWER_MODE) = current_power_mode;
            reg(SW_ENCOURAGE) = SW_MODE_ENCOURAGE_ON;
            reg(SW_ENCOURAGE) = SW_MODE_ENCOURAGE_DIS_LO;
            reg(SW_ENCOURAGE) = SW_MODE_ENCOURAGE_DIS_HI;

            ret = wait_powerdown_state(SYSTOP_CPU_PD_MASK);
        }

        return ret;
    }

   private:
    static volatile Reg32 &reg(unsigned int o) {
        return reinterpret_cast<volatile Reg32 *>(Memory_Map::POWER_BASE)[o / sizeof(Reg32)];
    }

    static int wait_powerdown_state(const Reg32 mask) {
        int count = 0;
        Reg32 value = 0;

        do {
            value = reg(CURRENT_POWER_MODE);

            if (value == mask) {
                return 0;
            }

            TSC::usleep(2);

            count += 1;

            if (count == TIMEOUT_COUNT) {
                db<Power>(WRN) << "Timeout while waiting for a powerdown" << endl;
                return 1;
            }

        } while (true);
    }
};

__END_SYS
