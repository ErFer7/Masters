#pragma once

#include <architecture.h>
#include <machine/ic.h>
#include <network/ethernet.h>

#include <machine/cache.h>

__BEGIN_SYS

class DWC_Ether_QoS : public NIC<Ethernet> {
    static constexpr unsigned long Base = 0x16030000;
    static constexpr unsigned int k_number_of_descriptors = 10;

    static auto &Reg64(unsigned long base, unsigned int offset) {
        return *reinterpret_cast<volatile unsigned long *>(base + offset);
    }

    static auto &Reg32(unsigned long base, unsigned int offset) {
        return *reinterpret_cast<volatile unsigned int *>(base + offset);
    }

    class MDIO {
        enum Register { BASE = 0x200, DATA = 0x204 };
        enum Bit {
            CLOCK_250_300 = 0x5 << 8,
            WRITE = 0x1 << 2,
            READ = 0x3 << 2,
            BUSY = 1,
        };

      public:
        static void wait() {
            while (Reg32(Base, BASE) & BUSY)
                ;
        }

        static void set(unsigned char phy, unsigned char dev, unsigned short data) {
            write(phy, dev, read(phy, dev) | data);
        }

        static void clear(unsigned char phy, unsigned char dev, unsigned short data) {
            write(phy, dev, read(phy, dev) & ~data);
        }

        static void write(unsigned char phy, unsigned char dev, unsigned short data) {
            Reg32(Base, DATA) = data;
            Reg32(Base, BASE) = BUSY | WRITE | CLOCK_250_300 | ((phy & 0x1F) << 21) | ((dev & 0x1F) << 16);
            wait();
        }
        static unsigned short read(unsigned char phy, unsigned char dev) {
            Reg32(Base, BASE) = BUSY | READ | CLOCK_250_300 | ((phy & 0x1F) << 21) | ((dev & 0x1F) << 16);
            wait();
            return Reg32(Base, DATA) & 0xFFFF;
        }

        static void write45(unsigned char phy, unsigned short reg, unsigned short data) {
            write(phy, 0x1E, reg);
            write(phy, 0x1F, data);
        }

        static void set45(unsigned char phy, unsigned short reg, unsigned short data) {
            write45(phy, reg, read45(phy, reg) | data);
        }

        static void clear45(unsigned char phy, unsigned short reg, unsigned short data) {
            write45(phy, reg, read45(phy, reg) & ~data);
        }

        static unsigned short read45(unsigned char phy, unsigned short reg) {
            return write(phy, 0x1E, reg), read(phy, 0x1F);
        }
    };

    class PHY {
        enum Register {
            BASIC_CONTROL = 0x0,
            BASIC_STATUS = 0x1,
            CHIP_CONFIG = 0xA001,
            RGMII_CONFIG1 = 0xA003,
            PAD_DRIVE_STRENGTH_CFG = 0xA010,
            SYNC_E_CFG = 0xA012,
            STATUS = 0x11,
            PHY_ID_1 = 0x2,
            PHY_ID_2 = 0x3,
        };
        enum Bit {
            BASIC_CONTROL_RESET = 1 << 15,
            BASIC_CONTROL_AUTO_NEGOTIATION_ENABLE = 1 << 12,
            BASIC_CONTROL_RE_AUTO_NEGOTIATION = 1 << 9,
            STATUS_SPEED_MASK = 3 << 14,
            STATUS_SPEED_1000 = 2 << 14,
            STATUS_SPEED_100 = 1 << 14,
            STATUS_SPEED_10 = 0,
            STATUS_FULL_DUPLEX = 1 << 13,
            STATUS_LINK = 1 << 10,
            BASIC_STATUS_AUTO_NEGOTIATION_COMPLETE = 1 << 5,
            RGMII_CONFIG1_TX_CLK_SEL = 1 << 14,
            CHIP_CONFIG_RXC_DELAY_ENABLE = 1 << 8,
        };

      public:
        static void init() {
            MDIO::set(phy, BASIC_CONTROL, BASIC_CONTROL_AUTO_NEGOTIATION_ENABLE | BASIC_CONTROL_RE_AUTO_NEGOTIATION);
            while (!(MDIO::read(phy, BASIC_STATUS) & BASIC_STATUS_AUTO_NEGOTIATION_COMPLETE))
                ;

            MDIO::set45(phy, RGMII_CONFIG1, RGMII_CONFIG1_TX_CLK_SEL);
            MDIO::clear45(phy, CHIP_CONFIG, CHIP_CONFIG_RXC_DELAY_ENABLE);
            MDIO::clear45(phy, RGMII_CONFIG1, 0xF | 0xF << 10);
            MDIO::set45(phy, RGMII_CONFIG1, 10 | 10 << 10);

            MDIO::clear45(phy, PAD_DRIVE_STRENGTH_CFG, 7 << 13);
            MDIO::set45(phy, PAD_DRIVE_STRENGTH_CFG, 6 << 13);

            MDIO::clear45(phy, PAD_DRIVE_STRENGTH_CFG, 1 << 12);
            MDIO::set45(phy, PAD_DRIVE_STRENGTH_CFG, 3 << 4);
        }

      private:
        static constexpr unsigned int phy = 0;
    };

    class MAC {
      public:
        enum Bit {
            PACKET_FILTER_RECEIVE_ALL = 1 << 31,
            PACKET_FILTER_PROMISCUOUS_MODE = 1,
            CONFIGURATION_TRANSMITTER_ENABLE = 1 << 1,
            CONFIGURATION_RECEIVER_ENABLE = 1,
            RX_QUEUE_CONTROL0_QUEUE0_ENABLE = 2,
            PHY_CONTROL_STATUS_LINK_STATUS_UP = 1 << 19,
            PHY_CONTROL_STATUS_LINK_MODE_FULL_DUPLEX = 1 << 16,
            CONFIGURATION_CST = 1 << 21,

        };

        enum Register {
            CONFIGURATION = 0x0,
            PACKET_FILTER = 0x8,
            RX_QUEUE_CONTROL0 = 0xa0,
            PHY_CONTROL_STATUS = 0xf8,
            ADDRESS0_LOW = 0x304,
            ADDRESS0_HIGH = 0x300,

        };

        struct Address {
            unsigned int high;
            unsigned int low;
        };

        static Address address() { return {Reg32(Base, ADDRESS0_HIGH) & 0xFFFF, Reg32(Base, ADDRESS0_LOW)}; }
        static void address(unsigned int high, unsigned int low) {
            Reg32(Base, ADDRESS0_HIGH) = (Reg32(Base, ADDRESS0_HIGH) & 0xFFFF0000) | (high & 0xFFFF);
            Reg32(Base, ADDRESS0_LOW) = low;
        }

        static void init() {
            Reg32(Base, PACKET_FILTER) |= PACKET_FILTER_RECEIVE_ALL | PACKET_FILTER_PROMISCUOUS_MODE;
            Reg32(Base, RX_QUEUE_CONTROL0) = RX_QUEUE_CONTROL0_QUEUE0_ENABLE;
            Reg32(Base, CONFIGURATION) |= CONFIGURATION_RECEIVER_ENABLE | CONFIGURATION_TRANSMITTER_ENABLE;
            Reg32(Base, CONFIGURATION) |= CONFIGURATION_CST;
        }
    };

    class DMA {
      public:
        struct Descriptor {
            unsigned int des0;
            unsigned int des1;
            unsigned int des2;
            unsigned int des3;

            enum Bits {
                OWN = 1 << 31,
                IOC = 1 << 30,
                VALID = 1 << 24,
                FIRST = 1 << 29,
                LAST = 1 << 28,
                TX_ERROR = 1 << 15,

                RX_AVAILABLE = OWN | IOC | VALID,
            };
        };

        enum Registers {
            MODE = 0x1000,
            SYSBUS_MODE = 0x1004,
            CH0_TX_CONTROL = 0x1104,
            CH0_RX_CONTROL = 0x1108,
            CH0_TX_DESCRIPTORS_LIST_HADDR = 0x1110,
            CH0_TX_DESCRIPTORS_LIST_ADDR = 0x1114,
            CH0_RX_DESCRIPTORS_LIST_HADDR = 0x1118,
            CH0_RX_DESCRIPTORS_LIST_ADDR = 0x111c,
            CH0_TX_DESCRIPTORS_LIST_TAIL_POINTER = 0x1120,
            CH0_RX_DESCRIPTORS_LIST_TAIL_POINTER = 0x1128,
            CH0_TX_DESCRIPTORS_RING_LENGTH = 0x112c,
            CH0_RX_DESCRIPTORS_RING_LENGTH = 0x1130,
            CH0_CURRENT_APP_RX_DESCRIPTOR = 0x114c,
            CH0_CURRENT_APP_TX_DESCRIPTOR = 0x1144,
            CH0_INTERRUPT_ENABLE = 0x1134,
            CH0_INTERRUPT_STATUS = 0x1160,
        };

        enum Bits {
            CH0_INTERRUPT_ENABLE_NIE = 1 << 15,
            CH0_INTERRUPT_ENABLE_AIE = 1 << 14,
            CH0_INTERRUPT_ENABLE_RIE = 1 << 6,
            CH0_INTERRUPT_ENABLE_RBUE = 1 << 6,

            CH0_INTERRUPT_STATUS_RI = 1 << 6,
            CH0_INTERRUPT_STATUS_RBU = 1 << 7,
        };

        static void reset() {
            Reg32(Base, MODE) |= 1;
            while (Reg32(Base, MODE) & 1)
                ;
        }

        unsigned int interrupt(Buffer **buffer) {
            db<DWC_Ether_QoS>(TRC) << "DWC_Ether_QoS::DMA::interrupt() =>" << endl;

            unsigned int status = Reg32(Base, CH0_INTERRUPT_STATUS);
            unsigned int message = 0;

            if (status & CH0_INTERRUPT_STATUS_RI) {
                unsigned int i = get_received_id();
                db<DWC_Ether_QoS>(TRC) << "Received a Packet in Buffer: " << i << endl;
                *buffer = m_rx_buffers[i];
                message |= CH0_INTERRUPT_STATUS_RI;
            }

            if (status & CH0_INTERRUPT_STATUS_RBU) {
                db<DWC_Ether_QoS>(TRC) << "Receive Buffer Unavailable!" << endl;
                Descriptor *next = reinterpret_cast<Descriptor *>(Reg32(Base, CH0_CURRENT_APP_RX_DESCRIPTOR));
                next->des3 = Descriptor::RX_AVAILABLE;
                Reg32(Base, CH0_RX_DESCRIPTORS_LIST_TAIL_POINTER) = reinterpret_cast<unsigned long>(next);
            }

            Reg32(Base, CH0_INTERRUPT_STATUS) = ~0U;

            return message;
        }

        DMA() {
            db<DWC_Ether_QoS>(TRC) << "DWC_Ether_QoS::DMA()" << endl;
            init_descriptors();
            Reg32(Base, SYSBUS_MODE) |= 1 << 11;
            Reg32(Base, CH0_TX_CONTROL) |= 1;
            Reg32(Base, CH0_RX_CONTROL) |= 1;
            Reg32(Base, CH0_INTERRUPT_ENABLE) |= CH0_INTERRUPT_ENABLE_NIE | CH0_INTERRUPT_ENABLE_RIE |
                                                 CH0_INTERRUPT_ENABLE_AIE | CH0_INTERRUPT_ENABLE_RBUE;
        }

        void init_descriptors() {
            memset(m_tx_descriptors, 0, k_number_of_descriptors * sizeof(Descriptor));
            memset(m_rx_descriptors, 0, k_number_of_descriptors * sizeof(Descriptor));

            for (unsigned int i = 0; i < k_number_of_descriptors; i++) {
                auto &descriptor = m_rx_descriptors[i];
                m_rx_buffers[i] = new Buffer(nullptr, &m_rx_descriptors[i]);
                auto buffer = reinterpret_cast<unsigned long>(m_rx_buffers[i]->data());
                descriptor.des0 = static_cast<unsigned int>(buffer & 0xFFFFFFFF);
                descriptor.des1 = static_cast<unsigned int>(buffer >> 32);
                descriptor.des3 = Descriptor::RX_AVAILABLE;
                Cache::flush(&descriptor, sizeof(Descriptor));
            }

            Reg32(Base, CH0_RX_DESCRIPTORS_LIST_ADDR) = reinterpret_cast<unsigned long>(m_rx_descriptors) & 0xFFFFFFFF;
            Reg32(Base, CH0_RX_DESCRIPTORS_LIST_HADDR) = reinterpret_cast<unsigned long>(m_rx_descriptors) >> 32;
            Reg32(Base, CH0_RX_DESCRIPTORS_RING_LENGTH) = k_number_of_descriptors - 1;
            Reg32(Base, CH0_RX_DESCRIPTORS_LIST_TAIL_POINTER) =
                reinterpret_cast<unsigned long>(m_rx_descriptors + k_number_of_descriptors);

            Reg32(Base, CH0_TX_DESCRIPTORS_LIST_ADDR) =
                static_cast<unsigned int>(reinterpret_cast<unsigned long>(m_tx_descriptors) & 0xFFFFFFFF);
            Reg32(Base, CH0_TX_DESCRIPTORS_LIST_HADDR) =
                static_cast<unsigned int>(reinterpret_cast<unsigned long>(m_tx_descriptors) >> 32);
            Reg32(Base, CH0_TX_DESCRIPTORS_RING_LENGTH) = k_number_of_descriptors - 1;
        }

        unsigned int get_received_id() {
            unsigned long zero = reinterpret_cast<unsigned long>(m_rx_descriptors);
            unsigned long current = Reg32(Base, CH0_CURRENT_APP_RX_DESCRIPTOR);
            unsigned int i = (current - zero + 1) % k_number_of_descriptors;

            while (1) {
                Descriptor &d = m_rx_descriptors[i];
                Cache::flush(&d, sizeof(Descriptor));
                if (!(d.des3 & Descriptor::OWN))
                    if (!m_rx_buffers[i]->lock())
                        break;
                i = (i + 1) % k_number_of_descriptors;
            }
            return i;
        }

        void free_rx(Buffer *buffer) {
            Descriptor &d = *reinterpret_cast<Descriptor *>(buffer->shadow());
            d.des3 = Descriptor::RX_AVAILABLE;
            buffer->unlock();
        }

        int receive(void *frame, unsigned int length) {
            Reg32(Base, CH0_INTERRUPT_ENABLE) &= ~CH0_INTERRUPT_ENABLE_AIE;

            unsigned int i = get_received_id();
            Descriptor &d = m_rx_descriptors[i];

            unsigned long addr64 = (static_cast<unsigned long>(d.des1) << 32) | d.des0;
            unsigned short *addr = reinterpret_cast<unsigned short *>(addr64);
            unsigned long size = d.des3 & 0x3FFF;
            Cache::flush(addr, size);

            if (size > length)
                size = length;

            memcpy(frame, addr, size);

            free_rx(m_rx_buffers[i]);

            Reg32(Base, CH0_INTERRUPT_ENABLE) |= CH0_INTERRUPT_ENABLE_AIE;

            return size;
        }

        int send(void *frame, unsigned int length) {
            unsigned long buffer = reinterpret_cast<unsigned long>(frame);
            Cache::flush(frame, length);

            Descriptor &d = *reinterpret_cast<Descriptor *>(Reg32(Base, CH0_CURRENT_APP_TX_DESCRIPTOR));

            d.des0 = static_cast<unsigned int>(buffer & 0xFFFFFFFF);
            d.des1 = static_cast<unsigned int>(buffer >> 32);
            d.des3 = Descriptor::OWN | Descriptor::FIRST | Descriptor::LAST | (length & 0x3FFF);
            d.des2 = (length & 0x7FFF);
            Cache::flush(&d, sizeof(Descriptor));
            Reg32(Base, CH0_TX_DESCRIPTORS_LIST_TAIL_POINTER) = reinterpret_cast<unsigned long>(&d);

            while (1) {
                Cache::flush(&d, sizeof(Descriptor));
                if (!(d.des3 & Descriptor::OWN)) {
                    if (d.des3 & Descriptor::TX_ERROR)
                        return 0;
                    return length;
                }
            }
        }

      private:
        Buffer *m_rx_buffers[k_number_of_descriptors];
        Descriptor m_tx_descriptors[k_number_of_descriptors];
        Descriptor m_rx_descriptors[k_number_of_descriptors];
    };

    class MTL {
        enum Bit {
            TX_QUEUE0_OPERATION_MODE_TSF = 2,
        };

        enum Register {
            TX_QUEUE0_OPERATION_MODE = 0xd00,
        };

      public:
        static void init() { Reg32(Base, TX_QUEUE0_OPERATION_MODE) |= TX_QUEUE0_OPERATION_MODE_TSF; }
    };

  public:
    static auto *get(unsigned int unit) { return s_devices[unit]; }

    static void init(unsigned int unit) {
        db<DWC_Ether_QoS>(TRC) << "DWC_Ether_QoS::init(unit=" << unit << ")" << endl;

        s_devices[unit] = new (SYSTEM) DWC_Ether_QoS();

        // for (auto i : Traits<DWC_Ether_QoS>::IRQs) {
        //     IC::int_vector(i, interrupt);
        //     IC::enable(i);
        // }
        //
        // FIX: Remove this later. This is a temporary fix.
        for (volatile int i = 0; i < 1000000; i++);
    }

    DWC_Ether_QoS() {
        DMA::reset();
        MTL::init();
        PHY::init();
        MAC::init();
        for (unsigned int i = 0; i < k_number_of_descriptors; i++) {
            m_tx_buffers[i] = new Buffer(this, nullptr);
        }

        m_dma = new (SYSTEM) DMA();
    }

    void interrupt() {
        Buffer *buffer;
        if (m_dma->interrupt(&buffer) & DMA::CH0_INTERRUPT_STATUS_RI) {
            if (!notify(buffer->frame()->header()->prot(), buffer))
                m_dma->free_rx(buffer);
        };
    }

    static void interrupt(unsigned long id) {
        if (id == 39)
            s_devices[0]->interrupt();
    }

    int send(Buffer *buf);
    int send(const Address &dst, const Protocol &prot, const void *data, unsigned int size);
    int receive(Address *src, Protocol *prot, void *data, unsigned int size);
    Buffer *alloc(const Address &dst, const Protocol &prot, unsigned int once, unsigned int always,
                  unsigned int payload);
    void free(Buffer *buf);

    const Address &address() { return m_configuration.address; }
    void address(const Address &address) {
        m_configuration.address = address;
        m_configuration.selector = Configuration::ADDRESS;
        reconfigure(&m_configuration);
    }
    bool reconfigure(const Configuration *c) { return false; }
    const Configuration &configuration() { return m_configuration; }
    const Statistics &statistics() { return m_statistics.time_stamp = TSC::time_stamp(), m_statistics; }

  private:
    static DWC_Ether_QoS *s_devices[k_number_of_descriptors];
    DMA *m_dma;
    Buffer *m_tx_buffers[k_number_of_descriptors];
    Configuration m_configuration;
    Statistics m_statistics;
};

__END_SYS
