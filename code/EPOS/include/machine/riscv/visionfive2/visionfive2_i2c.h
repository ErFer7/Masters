// EPOS VisionFive 2 (RISC-V) I2C Mediator Declarations

#pragma once

#include <architecture.h>
#include <machine/i2c.h>

__BEGIN_SYS

class I2C : public I2C_Common {
    typedef CPU::Reg32 Reg32;
    typedef CPU::Reg16 Reg16;
    typedef CPU::Reg8 Reg8;

    static const int DEFAULT_TIMEOUT = 10000000;

   public:
    enum {
        IC_ENABLE = 0x6c,         // Enable Register
        IC_CON = 0x00,            // Control Register
        IC_TAR = 0x04,            // Target Address Register
        IC_SS_SCL_HCNT = 0x14,    // Standard Speed I2C Clock SCL High Count
        IC_SS_SCL_LCNT = 0x18,    // Standard Speed I2C Clock SCL Low Count
        IC_INTR_MASK = 0x30,      // Interrupt Mask Register
        IC_RX_TL = 0x38,          // Receive FIFO Threshold
        IC_TX_TL = 0x3c,          // Transmit FIFO Threshold
        IC_ENABLE_STATUS = 0x9c,  // Enable Status Register
        IC_STATUS = 0x70,         // Status Register
        IC_DATA_CMD = 0x10,       // Data Buffer and Command Register
    };

    // IC_CON Bits
    enum : Reg32 {
        IC_CON_SLAVE_DISABLE = 1U << 6U,
        IC_CON_RESTART_EN = 1U << 5U,
        IC_CON_IC_10BITADDR_MASTER = 1U << 4U,
        IC_CON_IC_10BITADDR_SLAVE = 1U << 3U,
        IC_CON_SPEED_STD = 1U << 1U,  // Standard Speed (100 kbps)
        IC_CON_MASTER_MODE = 1U << 0U,
    };

    // IC_DATA_CMD Bits
    enum {
        IC_DATA_CMD_READ = 1U << 8U,  // 1 = Read, 0 = Write
        IC_DATA_CMD_STOP = 1U << 9U,
    };

    // IC_STATUS Bits
    enum {
        IC_STATUS_ACTIVITY = 1U << 0U,
        IC_STATUS_TFNF = 1U << 1U,  // Transmit FIFO Not Full
        IC_STATUS_RFNE = 1U << 3U,  // Receive FIFO Not Empty
    };

   public:
    // The most recent versions of U-boot already initialize the I2C
    I2C() {}

   public:
    static bool enable() {
        reg(IC_ENABLE) = 1;

        volatile int timeout = DEFAULT_TIMEOUT;

        while (((reg(IC_ENABLE_STATUS) & 1) == 0) && (--timeout > 0));

        if (timeout <= 0) {
            return false;
        }

        return true;
    }

    static bool disable() {
        reg(IC_ENABLE) = 0U;

        volatile int timeout = DEFAULT_TIMEOUT;

        while (((reg(IC_ENABLE_STATUS) & 1) != 0) && (--timeout > 0));

        if (timeout <= 0) {
            return false;
        }

        return true;
    }

    static void config(const Role &role) {}

    static bool get(char slave_address, char *data, bool stop = true) { return read(slave_address, data, 1, stop); }

    static bool put(unsigned char slave_address, char data, bool stop = true) {
        return write(slave_address, &data, 1, stop);
    }

    static bool read(char slave_address, char *data, unsigned int size, bool stop = true) {
        if (reg(IC_TAR) != slave_address) {
            disable();
            reg(IC_TAR) = slave_address;
            enable();
        }

        for (unsigned int i = 0; i < size; i++) {
            bool last = (i == size - 1);
            Reg32 cmd = IC_DATA_CMD_READ;

            if (last && stop) cmd |= IC_DATA_CMD_STOP;

            if (!wait_tx_not_full()) return false;

            reg(IC_DATA_CMD) = cmd;

            if (!wait_rx_not_empty()) return false;

            data[i] = reg(IC_DATA_CMD) & 0xFF;
        }

        return true;
    }

    static bool write(unsigned char slave_address, const char *data, unsigned int size, bool stop = true) {
        if (reg(IC_TAR) != slave_address) {
            disable();
            reg(IC_TAR) = slave_address;
            enable();
        }

        for (unsigned int i = 0; i < size; i++) {
            bool last = (i == size - 1);
            Reg32 cmd = data[i] & 0xFF;

            if (last && stop) cmd |= IC_DATA_CMD_STOP;

            if (!wait_tx_not_full()) return false;

            reg(IC_DATA_CMD) = cmd;
        }

        if (stop) {
            while (reg(IC_STATUS) & IC_STATUS_ACTIVITY);
        }

        return true;
    }

    static void flush() {}
    static bool ready_to_get() { return (reg(IC_STATUS) & IC_STATUS_RFNE); }
    static bool ready_to_put() { return (reg(IC_STATUS) & IC_STATUS_TFNF); }

    static void int_enable(bool receive = true, bool transmit = true, bool line = true, bool modem = true) {}
    static void int_disable(bool receive = true, bool transmit = true, bool line = true, bool modem = true) {}

    static void loopback(bool on = true) {}
    static void reset() {}

   private:
    static volatile Reg32 &reg(unsigned int o) {
        return reinterpret_cast<volatile Reg32 *>(Memory_Map::I2C5_BASE)[o / sizeof(Reg32)];
    }

    static bool wait_tx_not_full() {
        volatile int timeout = DEFAULT_TIMEOUT;

        while (!(reg(IC_STATUS) & IC_STATUS_TFNF)) {
            if (--timeout <= 0) {
                db<I2C>(WRN) << "Failed I2C: " << reg(0x80) << endl;

                return false;
            }
        }

        return true;
    }

    static bool wait_rx_not_empty() {
        volatile int timeout = DEFAULT_TIMEOUT;

        while (!(reg(IC_STATUS) & IC_STATUS_RFNE)) {
            if (--timeout <= 0) {
                db<I2C>(WRN) << "Failed I2C: " << reg(0x80) << endl;

                return false;
            }
        }

        return true;
    }
};

__END_SYS
