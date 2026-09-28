// EPOS RISC-V VisionFive 2 (GEM) Ethernet NIC Mediator Declarations

#ifndef __riscv_gem_h
#define __riscv_gem_h

#include <architecture.h>
#include <network/ethernet.h>
#include <utility/convert.h>
#include <machine/ic.h>

__BEGIN_SYS

class Cadence_GEM
{
protected:
    typedef CPU::Reg8 Reg8;
    typedef CPU::Reg16 Reg16;
    typedef CPU::Reg32 Reg32;
    typedef CPU::Reg64 Reg64;
    typedef CPU::Log_Addr Log_Addr;
    typedef CPU::Phy_Addr Phy_Addr;
    typedef MMU::DMA_Buffer DMA_Buffer;
    typedef Ethernet::Address MAC_Address;
    typedef Ethernet::Time_Stamp Time_Stamp;

    static const bool promiscuous = Traits<GEM>::promiscuous;

public:
    // Register offsets
    enum : Reg32 {
        NWCTRL          = 0x00,  // Network Control
        NWCFG           = 0x04,  // Network Config
        NWSR            = 0x08,  // Network Status
        DMACFG          = 0x10,  // DMA Config
        TXSTATUS        = 0x14,  // TX Status
        RXQBASE         = 0x18,  // RX Q Base address
        TXQBASE         = 0x1c,  // TX Q Base address
        RXSTATUS        = 0x20,  // RX Status
        ISR             = 0x24,  // Interrupt Status
        IER             = 0x28,  // Interrupt Enable
        IDR             = 0x2c,  // Interrupt Disable
        IMR             = 0x30,  // Interrupt Mask
        SPADDR1L        = 0x88,  // Specific1 addr (lower 32 bits)
        SPADDR1H        = 0x8c,  // Specific1 addr (higher 32 bits)
    };

    // Network Control Register meaningful bits
    enum : Reg32 {
        TXSTART                 = 1 << 9, // Transmit start
        CLEAR_STATS_REGS        = 1 << 5, // Clear stats bit
        CTRL_MGMT_PORT_EN       = 1 << 4, // Control management port enable
        TX_EN                   = 1 << 3, // Transmit enable
        RX_EN                   = 1 << 2, // Receive enable
        LOCALLOOP               = 1 << 1, // Local loopback
    };

    // Network Configuration Register meaningful bits
    enum : Reg32 {
        SPEED_100               = 1 << 0,  // Speed 100
        FULL_DUPLEX             = 1 << 1,  // Full duplex
        PROMISC                 = 1 << 4,  // Promiscuous mode
        GIGABIT_EN              = 1 << 10, // Gigabit mode enable
        MDC_DIV_48              = 3 << 18, // MDC clock divider 48
        STRIP_FCS               = 1 << 17, // Strip FCS field
        RX_BUF_OFFSET           = 2 << 14, // RX buffer offset for Ethernet
        MDC_CLK_DIV_MASK        = 7 << 18, // MDC clock divider mask
        _32_DBUS_WIDTH_SIZE     = 0 << 21, // 32 bits size
        _64_DBUS_WIDTH_SIZE     = 1 << 21, // 64 bits size
        DBUS_WIDTH_MASK         = 3 << 21, // DBUS width mask
        IGNORE_FCS              = 1 << 26,
    };

    // Transmit Status Register meaningful bits
    enum : Reg32 {
        TX_STAT_COMPLETE        = 1 << 5,
        TX_STAT_USED_BIT_READ   = 1 << 0,
        TX_STAT_ALL             = 0xff,
    };

    // Receive Status Register meaningful bits
    enum : Reg32 {
        RX_STAT_OVERRUN         = 1 << 2,
        RX_STAT_FRAME_RECD      = 1 << 1,
        RX_STAT_NOBUF           = 1 << 0,
        RX_STAT_ALL             = 0x0f,
    };

    // DMA Config (DMACFG) meaningful bits
    enum : Reg32 {                              // Description
        RX_PBUF_SIZE            =  0x3 << 8,    // DMA address space for RX buffers (up to 32KB)
        TX_PBUF_SIZE            =    1 << 10,   // DMA address space for TX buffers (16 or 32KB)
        RX_BUF_SIZE             = 0xff << 16,   // DMA receive buffer size x 64 (0x18 for 1536 bytes)
        FORCE_DISCARD_ON_ERR    =    1 << 24,   // Auto Discard RX packets on lack of resource
        RX_TIMESTAMP_ON         =    1 << 28,   
        TX_TIMESTAMP_ON         =    1 << 29,   // Capture timestamp (uses 2 more words in descriptors)
        DMA_ADDR_BUS_WIDTH      =    1 << 30,   // DMA address bus width (0 = 32b, 1 = 64b)
        RX_BUF_SIZE_SHIFT       =         16
    };

    // Interrupt register meaningful bits
    enum : Reg32 {
        INTR_RX_COMPLETE        = 1 << 1,
        INTR_RX_USED_READ       = 1 << 2,
        INTR_TX_USED_READ       = 1 << 3,
        INTR_TX_UNDERRUN        = 1 << 4,
        INTR_TX_CORRUPT_AHB_ERR = 1 << 6,
        INTR_TX_COMPLETE        = 1 << 7,
        INTR_RX_OVERRUN         = 1 << 10,
        INT_ALL                 = 0x07fffeff
    };

    // Transmit and Receive Descriptors (in the Ring Buffers)
    struct Desc {
        volatile Reg32 addr;
        volatile Reg32 ctrl;
        volatile Reg32 ts_n; // timestamp nanoseconds (except bits 31:30 (1:0 of seconds)))
        volatile Reg32 ts_s; // timestamp seconds (only bits 5:2, 1:0 are in ts_n)

        enum : Reg32 {
            TS_SECONDS_MASK = 0x3c,
            TS_NANO_MASK    = 0x3fffffff
        };

        Time_Stamp time_stamp() const { return ((ts_s & TS_SECONDS_MASK) | ((ts_n & ~TS_NANO_MASK) >> 30) * 1000000000) + (ts_n & TS_NANO_MASK); }
    };

    // Receive Descriptor
    struct Rx_Desc : public Desc
    {
        enum : Reg32 {
            OWN         = 1 << 0, // Who owns the buffer? 0 => NIC, 1 => Host
            WRAP        = 1 << 1, // Wrap around to make the buffer pool a circular list (set on the last buffer)
            EOF         = 1 << 15,
            SOF         = 1 << 14,
            SIZE_MASK   = 0x3fff
        };

        void size(unsigned int size) { ctrl = (ctrl & ~SIZE_MASK) | (size & SIZE_MASK); }

        friend OStream & operator<<(OStream & os, const Rx_Desc & d) {
            os << "{" << hex << d.addr << dec << ","
                << (d.ctrl & SIZE_MASK) << "," << hex << d.ctrl << dec << "," << d.time_stamp() << "}";
            return os;
        }
    };

    // Transmit Descriptor
    struct Tx_Desc : public Desc
    {
        enum {
            OWN         = 1U << 31, // Who owns the buffer? 0 => NIC, 1 => Host
            WRAP        = 1  << 30, // Wrap around to make the buffer pool a circular list (set on the last buffer)
            LAST_BUF    = 1  << 15, // Last buffer of frame (our buffers always can contain a full frame, so LAST_BUF is always true)
            SIZE_MASK   = 0x1fff
        };

        void size(unsigned int size) {  ctrl = (ctrl & ~SIZE_MASK) | (size & SIZE_MASK); }

        friend OStream & operator<<(OStream & os, const Tx_Desc & d) {
            os << "{" << hex << d.addr << dec << ","
                << (d.ctrl & SIZE_MASK) << "," << hex << d.ctrl << dec << "," << d.time_stamp() << "}";
            return os;
        }
    };

public:
    void configure(const MAC_Address & mac, Phy_Addr tx_ring, Phy_Addr rx_ring) {
        // Initialize the controller
        reg(NWCTRL) = 0;
        reg(NWCTRL) = CLEAR_STATS_REGS;
        reg(NWCFG) = _32_DBUS_WIDTH_SIZE; // 32-bit data bus
        reg(TXSTATUS) = TX_STAT_ALL;
        reg(RXSTATUS) = RX_STAT_ALL;
        reg(IDR) = INTR_RX_COMPLETE;
        reg(TXQBASE) = 0;
        reg(RXQBASE) = 0;

        // Set the MAC address
        mac_address(mac);

        // Set the descriptors rings pointers
        reg(TXQBASE) = tx_ring;
        reg(RXQBASE) = rx_ring;

        // Set full duplex and let the NIC handler FCS
        reg(NWCFG) |= (STRIP_FCS | FULL_DUPLEX | GIGABIT_EN);

        // Capture timestamps in descriptors
        reg(DMACFG) = RX_TIMESTAMP_ON | TX_TIMESTAMP_ON | RX_PBUF_SIZE | TX_PBUF_SIZE;

        // Enable TX and RX
        reg(NWCTRL) |= TX_EN | RX_EN;

        // Enable interrupts
        reg(IER) |= INTR_RX_COMPLETE | INTR_TX_CORRUPT_AHB_ERR | INTR_RX_OVERRUN | INTR_TX_UNDERRUN;

        // Promiscuous mode (receive all packets independently of the destination addresses)
        if(promiscuous)
            reg(NWCFG) |= PROMISC;
        else
            reg(NWCFG) &= ~PROMISC;
    };

    void int_enable() { reg(IER) |= INTR_RX_COMPLETE; }
    void int_disable() { reg(IDR) |= INTR_RX_COMPLETE; }

    void start_tx() { reg(NWCTRL) |= TXSTART; }
    void complete_tx() { reg(TXSTATUS) |= TX_STAT_COMPLETE; };
    void tx_used_read_write() { reg(TXSTATUS) |= TX_STAT_USED_BIT_READ; };
    void rx_used_read_write() { reg(RXSTATUS) |= RX_STAT_NOBUF; };
    void rx_overrun_write() { reg(RXSTATUS) |= RX_STAT_OVERRUN; };
    void complete_rx() { reg(ISR) = INTR_RX_COMPLETE; reg(RXSTATUS) |= RX_STAT_FRAME_RECD; };

    Reg32 isr() { return reg(ISR); };
    void clear_isr() { reg(ISR) = 0; };

    void mac_address(const MAC_Address & mac) {
        reg(SPADDR1L) = (mac[0] << 0) | (mac[1] << 8) | (mac[2] << 16) | (mac[3] << 24);
        reg(SPADDR1H) = (mac[4] << 0) | (mac[5] << 8);
    }

    MAC_Address mac_address() {
        MAC_Address mac;
        mac[0] = reg(SPADDR1L) >> 0;
        mac[1] = reg(SPADDR1L) >> 8;
        mac[2] = reg(SPADDR1L) >> 16;
        mac[3] = reg(SPADDR1L) >> 24;
        mac[4] = reg(SPADDR1H) >> 0;
        mac[5] = reg(SPADDR1H) >> 8;
        return mac;
    };

    // Maximum RX DMA transfer size
    // Value will be rounded up to the next multiple of 64 bytes
    // TX DMA transfer size is set in the descriptor
    void dma_rx_buffer_size(unsigned int size) { reg(DMACFG) = (reg(DMACFG) & ~RX_BUF_SIZE) | ((((size + 63) / 64) << RX_BUF_SIZE_SHIFT) & RX_BUF_SIZE); };

private:
    static volatile Reg32 & reg(unsigned int o) { return reinterpret_cast<volatile Reg32 *>(Memory_Map::ETH_BASE)[o / sizeof(Reg32)]; }
};


// Cadence GEM Ethernet NIC, currenlty validated only for SiFive-U
class GEM: public NIC<Ethernet>, private Cadence_GEM
{
    friend class Machine_Common;

private:
    typedef IC::Interrupt_Id Interrupt_Id;

    // Transmit and Receive Ring sizes
    static const unsigned int UNITS = Traits<GEM>::UNITS;
    static const unsigned int TX_BUFS = Traits<GEM>::SEND_BUFFERS;
    static const unsigned int RX_BUFS = Traits<GEM>::RECEIVE_BUFFERS;

    // Size of the DMA Buffer that will host the ring buffers
    static const unsigned int DMA_BUFFER_SIZE = RX_BUFS * sizeof(Rx_Desc) + TX_BUFS * sizeof(Tx_Desc) + RX_BUFS * sizeof(Buffer) + TX_BUFS * sizeof(Buffer);

    // Interrupt dispatching binding
    struct Device {
        GEM * device;
        Interrupt_Id interrupt;
    };

protected:
    GEM(unsigned int unit, DMA_Buffer * dma_buf);

public:
    ~GEM();

    int send(const Address & dst, const Protocol & prot, const void * data, unsigned int size);
    int receive(Address * src, Protocol * prot, void * data, unsigned int size);

    Buffer * alloc(const Address & dst, const Protocol & prot, unsigned int once, unsigned int always, unsigned int payload);
    int send(Buffer * buf);
    void free(Buffer * buf);

    const Address & address() { return _configuration.address; }
    void address(const Address & address) { _configuration.address = address; _configuration.selector = Configuration::ADDRESS; reconfigure(&_configuration); }

    bool reconfigure(const Configuration * c);
    const Configuration & configuration() { return _configuration; }

    const Statistics & statistics() { _statistics.time_stamp = TSC::time_stamp(); return _statistics; }

    void attach(Observer * o, const Protocol & p) {
        NIC<Ethernet>::attach(o, p);
        int_enable();
    }

    void detach(Observer * o, const Protocol & p) {
        NIC<Ethernet>::detach(o, p);
        if(!observers())
            int_disable();
    }

    static GEM * get(unsigned int unit = 0) { return get_by_unit(unit); }

private:
    void receive();
    void handle_int();

    static void int_handler(Interrupt_Id interrupt);

    static GEM * get_by_unit(unsigned int unit) {
        assert(unit < UNITS);
        return _devices[unit].device;
    }

    static GEM * get_by_interrupt(Interrupt_Id interrupt) {
        for(unsigned int i = 0; i < UNITS; i++)
            if(_devices[i].interrupt == interrupt)
                return _devices[i].device;
        db<PCNet32>(WRN) << "GEM::get_by_interrupt(" << interrupt << ") => no device bound!" << endl;
        return 0;
    };

    static void init(unsigned int unit);

private:
    Configuration _configuration;
    Statistics _statistics;

    DMA_Buffer * _dma_buf;

    int _rx_cur;
    Rx_Desc * _rx_ring;
    Phy_Addr _rx_ring_phy;

    int _tx_cur;
    Tx_Desc * _tx_ring;
    Phy_Addr _tx_ring_phy;

    Buffer * _rx_buffer[RX_BUFS];
    Buffer * _tx_buffer[TX_BUFS];

    static Device _devices[UNITS];
};

__END_SYS

#endif
