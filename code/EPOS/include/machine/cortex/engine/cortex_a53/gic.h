// EPOS ARM Cortex GIC Mediator Declarations
// Used on Xilinx Ultrascale+ ZU3EG - FZ3

#ifndef __cortex_gic_h
#define __cortex_gic_h

#include <architecture/cpu.h>
#define __ic_common_only__
#include <machine/ic.h>
#undef __ic_common_only__

#include <system/memory_map.h>

__BEGIN_SYS

class GIC: public IC_Common
{
protected:
    typedef CPU::Reg32 Reg32;

public:
    // IRQs
    static const unsigned int IRQS = 92; //TODO
    static const unsigned int INT_MAX = 1024;
    static const unsigned int SGIS = 16;
    static const unsigned int PPIS = 16;
    static const unsigned int INT_REGS = INT_MAX / 32;
    static const unsigned int PRIO_BITS = 8;
    static const unsigned int TARGET_BITS = 8;
    static const unsigned int CFG_BITS = 2;
    static const unsigned int SEC_BITS = 2;
    static const unsigned int SGI_BITS = 8;
    static const unsigned int SGI_REGS = SGIS * SGI_BITS / 32;
    
    typedef Interrupt_Id IRQ; //TODO
    enum {
        IRQ_SOFTWARE0           = 0,
        IRQ_SOFTWARE1           = 1,
        IRQ_SOFTWARE2           = 2,
        IRQ_SOFTWARE3           = 3,
        IRQ_SOFTWARE4           = 4,
        IRQ_SOFTWARE5           = 5,
        IRQ_SOFTWARE6           = 6,
        IRQ_SOFTWARE7           = 7,
        IRQ_SOFTWARE8           = 8,
        IRQ_SOFTWARE9           = 9,
        IRQ_SOFTWARE10          = 10,
        IRQ_SOFTWARE11          = 11,
        IRQ_SOFTWARE12          = 12,
        IRQ_SOFTWARE13          = 13,
        IRQ_SOFTWARE14          = 14,
        IRQ_SOFTWARE15          = 15,
        IRQ_GLOBAL_TIMER        = 27,
        IRQ_NFIQ                = 28,
        IRQ_PRIVATE_TIMER       = 29,
        // HAVE TO FIX NUMBERS FROM HERE ON
        IRQ_NS_PRIV_TIMER       = 30,
        IRQ_NIRQ                = 31,
        IRQ_APU0                = 32,
        IRQ_APU1                = 33,
        IRQ_L2                  = 34,
        IRQ_OCM                 = 35,
        IRQ_PMU0                = 37,
        IRQ_PMU1                = 38,
        IRQ_XADC                = 39,
        IRQ_DEVC                = 40,
        IRQ_SWDT                = 41,
        IRQ_TTC0_0              = 42,
        IRQ_TTC0_1              = 43,
        IRQ_TTC0_2              = 44,
        IRQ_DMAC_ABORT          = 45,
        IRQ_DMAC0               = 46,
        IRQ_DMAC1               = 47,
        IRQ_DMAC2               = 48,
        IRQ_DMAC3               = 49,
        IRQ_SMC                 = 50,
        IRQ_QSPI                = 51,
        IRQ_GPIO                = 52,
        IRQ_USB0                = 53,
        IRQ_ETHERNET0           = 54,
        IRQ_ETHERNET0_WAKEUP    = 55,
        IRQ_SDIO0               = 56,
        IRQ_I2C0                = 57,
        IRQ_SPI0                = 58,
        IRQ_UART0               = 59,
        IRQ_CAN0                = 60,
        IRQ_PL0                 = 61,
        IRQ_PL1                 = 62,
        IRQ_PL2                 = 63,
        IRQ_PL3                 = 64,
        IRQ_PL4                 = 65,
        IRQ_PL5                 = 66,
        IRQ_PL6                 = 67,
        IRQ_PL7                 = 68,
        IRQ_TTC1_0              = 69,
        IRQ_TTC1_1              = 70,
        IRQ_TTC1_2              = 71,
        IRQ_DMAC4               = 72,
        IRQ_DMAC5               = 73,
        IRQ_DMAC6               = 74,
        IRQ_DMAC7               = 75,
        IRQ_USB1                = 76,
        IRQ_ETHERNET1           = 76,
        IRQ_ETHERNET1_WAKEUP    = 78,
        IRQ_SDIO1               = 79,
        IRQ_I2C1                = 80,
        IRQ_SPI1                = 81,
        IRQ_UART1               = 82,
        IRQ_CAN1                = 83,
        IRQ_PL8                 = 84,
        IRQ_PL9                 = 85,
        IRQ_PL10                = 86,
        IRQ_PL11                = 87,
        IRQ_PL12                = 88,
        IRQ_PL13                = 89,
        IRQ_PL14                = 90,
        IRQ_PL15                = 91,
        IRQ_PARITY              = 92
    };

    // GIC offsets from private peripheral space //TODO
    enum {
        GICCPU                      = 0x0100,
        GICDIST                     = 0x1000
    };

    // CPU Interface Registers offsets
    enum {                                      // Description                          Type    Value after reset
        ICCCTLR                     = 0x0000,    // CPU Interface Control Reg            r/w     0x00000000
        ICCPMR                      = 0x0004,    // Interrupt Priority Mask Reg          r/w     0x00000000
        ICCBPR                      = 0x0008,    // Binary Point Reg                     r/w     0x0000000?
        ICCIAR                      = 0x000c,    // Interrupt Acknowledge Reg            r/o     0x000003ff
        ICCEOIR                     = 0x0010,    // End Of Interrupt Reg                 w/o     -
        ICCRPR                      = 0x0014,    // Running Priority Reg                 r/o     0x000000ff
        ICCHPPR                     = 0x0018,    // Highest Priority Pend. Int. Reg      r/o     0x000003ff
        ICCABPR                     = 0x001c,    // Aliased Binary Point Reg             r/w     0x0000000?
        ICCAIAR                     = 0x0020,    // Aliased Interrupt Acknowledge Reg    r/o     0x000003ff
        ICCAEOIR                    = 0x0024,    // Aliased End Of Interrupt Reg         w/o     -
        ICCAHPPR                    = 0x0028,    // Aliased Highest Prio. Pend. Int. Reg r/o     0x000003ff
        ICCAPR0                     = 0x00d0,    // Active Priorities Regs[4]            r/w     0
        ICCNSAPR0                   = 0x00e0,    // Non-Secure (NS) APR[4]               r/w     0
        ICCIIDR                     = 0x00fc,    // CPU Interface Identification Reg     r/o     Implementation Defined
        ICCDIR                      = 0x1000,    // Dectivate Int Register               w/o     -
    };

    // Useful offsets and masks
    enum {
        INT_ID_MASK                 = 0x3ff,        // len = 10
        INT_ID_OFF                  = 0x0,
        IAR_CPU_MASK                = 0x7,          // len = 3 -> 0b111
        IAR_CPU_OFF                 = 10,
        TYPER_ITLINENUM_MASK        = 0x1f,         // len = 5
        TYPER_ITLINENUM_OFF         = 0
    };

    // Useful bits in ICCICR
    enum {                                          // Description                  Type    Value after reset
        ICC_CTLR_EN                 = 0x1 << 0,
        ICC_CTLR_FIQ_Bypass_DIS     = 0x1 << 5,
        ICC_CTLR_IRQ_Bypass_DIS     = 0x1 << 6,
        ICC_CTLR_EOI_NS             = 0x1 << 9,
        ICC_CTLR_ENA                = 0x1 << 1,     // Enable secure signaling      r/w     0
        ICC_CTLR_ARE_NS             = 0x1 << 4      // Acknowledge control          r/w     0        
    };

    // Distributor Registers offsets
    enum {                                      // Description                              Type    Value after reset
        ICDCTLR                     = 0x0000,   // Distributor Control                      r/w     0
        ICDTYPER                    = 0x0004,   // Interrupt Controller Type                r/o     Implementation Defined
        ICDIIDR                     = 0x0008,   // Implementer Identification Register      r/o     Implementation Defined
        ICDSTATUSR                  = 0x0010,   // Status Register                          r/o     0
        ICDSETSPI_NSR               = 0x0040,   // Set SPI Non-Secure (NS) Register         r/w     0
        ICDCLRSPI_NSR               = 0x0048,   // Clear SPI NS Register                    r/w     0
        ICDSETSPI_SR                = 0x0050,   // Set SPI S Register                       r/w     0
        ICDCLRSPI_SR                = 0x0058,   // Clear SPI S Register                     r/w     0
        ICDIGROUPR0                 = 0x0080,   // Interrupt Group Registers[INT_REGS]      r/w     0
        ICDISENABLER0               = 0x0100,   // Interrupt Set-Enable Registers[INT_REGS] r/w     Implementation Defined
        ICDICENABLER0               = 0x0180,   // Interrupt Clear Enable Reg[INT_REGS]     r/w     Implementation Defined
        ICDISPENDR0                 = 0x0200,   // Int. Set-Pending Registers[INT_REGS]     r/w     0
        ICDICPENDR0                 = 0x0280,   // Int. Clear-Pending Registers[INT_REGS]   r/w     0
        ICDISACTIVER0               = 0x0300,   // GICv2 Int. Set-Active Regs[INT_REGS]     r/w     0
        ICDICACTIVER0               = 0x0380,   // Int. Clear-Active Registers[INT_REGS]    r/w     0
        ICDIPRIORITYR0              = 0x0400,   // Int. Priority Regs[INT_REGS*PRIO_BITS]   r/w     0
        ICDITARGETSR0               = 0x0800,   // Int. Targets Regs[TARGET_BITS*INT_REGS]  r/w     0 (r/o for position first 7 INT_MAX 0x800 to 0x81C)
        ICDICFGR0                   = 0x0c00,   // Int. Config Regs[INT_REGS * CFG_BITS]    r/w     0
        ICDIGPRMODR0                = 0x0d00,   // Int. GPR Mod Regs[INT_REGS]              r/w     0
        ICDNSACR0                   = 0x0e00,   // NS Access Ctrl Regs[SEC_BITS*INT_REGS]   r/w     0
        ICDSGIR                     = 0x0f00,   // SW Generated Int. (SGI) Register         w/o     -
        ICDCPENDSGIR0               = 0x0f10,   // SGI Clear-Pending Regs[SGI_REGS]         r/w     0
        ICDSPENDSGIR0               = 0x0f20,   // SGI Set-Pending Regs[SGI_REGS]           r/w     0
        ICDIROUTER0                 = 0x6000,   // Int. Route Regs[INT_MAX]                    r/w     0
        ICDIDR0                     = 0xffd0    // ID Regs [0xffff - 0xffd0]    
    };

    // Useful bits in ICDDCR //TODO
    enum {                                      // Description                  Type    Value after reset
        DIST_EN_S                   = 1 << 0    // Enable secure interrupts     r/w     0
    };
};


class GIC_Distributor: public GIC
{
    // This is a hardware object.
    // Use with something like "new (Memory_Map::GIC_DIST_BASE) GIC_Distributor".

public:
    
    // TODO check enables and disables
    void enable() {
        for (unsigned int i = 0; i < INT_MAX-2; i++) {//(INT_MAX/32); i++) {
            //32 bits for every register, i indicates the byte that must be accessed
            // gic_dist(ICDISENABLER0 + i * sizeof(Reg32)) = -1;
            // gic_dist(ICDISACTIVER0 + i * sizeof(Reg32)) = -1;
            enable(i);
        }
    }

    void enable(Interrupt_Id i) {
        //32 bits for every register, sizeof(Reg32) to offset i as the respective byte to be accessed.
        // 32 / sizeof(Reg32) == 8
        gic_dist(ICDISENABLER0 + i/8) |= 1 << (i % 32);
        Reg32 tgt_ind = (TARGET_BITS*i) / (sizeof(Reg32) * 8);
        Reg32 tgt_off = (TARGET_BITS*i) % (sizeof(Reg32) * 8);
        // shift by cpu number (here assuming 0 at first) and shift by the offset to reach the interrupt number
        gic_dist(ICDITARGETSR0+sizeof(Reg32)*tgt_ind) |= 0x1 << 0 << tgt_off;

        Reg32 prio_ind = (i*PRIO_BITS)/(sizeof(Reg32)*8);
        Reg32 prio_off = (i*PRIO_BITS)%((sizeof(Reg32)*8));
        Reg32 prio_mask = ((1 << PRIO_BITS)-1) << prio_off;
        //0 is max priority, so no needs to make an or...
        gic_dist(ICDIPRIORITYR0 + prio_ind*sizeof(Reg32)) = gic_dist(ICDIPRIORITYR0 + prio_ind*sizeof(Reg32)) & ~prio_mask;// | 0 << prio_off; 
        // gic_dist(ICDISACTIVER0 + i/8) |= 1 << (i % 32);
        // gic_dist(ICDITARGETSR0 + i) |= 1 << (i % 4);
        // enable();
    }

    void disable() {
        for (unsigned int i = 0; i < (INT_MAX/32); i++)
            gic_dist(ICDICENABLER0 + i * sizeof(Reg32)) = ~0;
    }

    void disable(Interrupt_Id i) {
        gic_dist(ICDICENABLER0 + i/8) |= 1 << (i % 32);
    }


    void send_sgi(unsigned int cpu, Interrupt_Id i) {
        Reg32 target_list = 1UL << cpu;
        Reg32 filter_list = 0;
        gic_dist(ICDSGIR) = ((filter_list << 24) | (target_list << 16) | (i & 0x0f));
    }

    void send_sgi(Interrupt_Id i, Reg32 target_list, Reg32 filter_list) {
        Reg32 aux = i & 0x0f;
        target_list = target_list & 0x0f;
        filter_list = filter_list & 0x0f;
        aux = aux | (target_list << 16);
        aux = aux | (filter_list << 24);
        gic_dist(ICDSGIR) = aux;
    }

    // TODO view method set_trgt from bao files on Notion
    void smp_init(unsigned int cores) {
        // Interrupt_Id i = 0; // reset cores

        // Reg32 target_list = 0x0;
        // if(cores <= CPU::cores())
        //     for(unsigned int core = 0; core < cores; core++)
        //         target_list |= 1 << (core);
        // else
        //     for(unsigned int core = 0; core < CPU::cores(); core++)
        //         target_list |= 1 << (core);

        // Reg32 filter_list = 1; // except the current core

        // send_sgi(i, target_list, filter_list);
    }

    void init() {
        // Enable distributor
        Reg32 int_num = gic_num_int();

        /* Bring distributor to known state */
        for(unsigned int i = ((PPIS + SGIS)/32); i < int_num/32; i++){
            /**
             * Make sure all interrupts are not enabled, non pending,
             * non active.
             */
            //32 bits for every register, i indicates the byte that must be accessed
            gic_dist(ICDICENABLER0 + i * sizeof(Reg32)) = -1;
            gic_dist(ICDICPENDR0 + i * sizeof(Reg32)) = -1;
            gic_dist(ICDICACTIVER0 + i * sizeof(Reg32)) = -1;
        }

        /* All interrupts have lowest priority possible by default */
        for(unsigned int i = 0; i < PRIO_BITS*int_num/32; i++)
            gic_dist(ICDIPRIORITYR0 + i * sizeof(Reg32)) = -1;

        /* No CPU targets for any interrupt by default */
        for(unsigned int i = 0; i < TARGET_BITS*int_num/32; i++)
            gic_dist(ICDITARGETSR0 + i * sizeof(Reg32)) = 0;

        /* No CPU targets for any interrupt by default */
        for(unsigned int i = 0; i < int_num*CFG_BITS/32; i++)
            gic_dist(ICDICFGR0 + i * sizeof(Reg32)) = 0xAAAAAAAA;

        /* No need to setup gicd->NSACR as all interrupts are  setup to group 1 */

        /* Enable distributor */
        gic_dist(ICDCTLR) = gic_dist(ICDCTLR) | DIST_EN_S;
    }

protected:
    Reg32 gic_num_int(){
        Reg32 typer = gic_dist(ICDTYPER);
        typer = (typer >> TYPER_ITLINENUM_OFF) & TYPER_ITLINENUM_MASK;
        typer += 1;
        typer = typer * 32;
        return typer;
    }

    volatile Reg32 & gic_dist(unsigned int o) { return reinterpret_cast<volatile Reg32 *>(Memory_Map::GICD_BASE)[o / sizeof(Reg32)]; }

    // The versions bellow are dynamic and work on any A9, but are quite inefficient
    //    static volatile Reg32 & gic_dist(unsigned int offset) {
    //        register Reg32 base asm("r0");
    //        ASM("mrc p15, 4, %0, c15, c0, 0" : "=r"(base)); // get private peripheral space base into r0
    //        return reinterpret_cast<volatile Reg32 *>(base + GICDIST)[offset / sizeof(Reg32)];
    //    }
};


class GIC_CPU: public GIC
{
public:
    Interrupt_Id int_id() {
        Reg32 icciar = gic_cpu(ICCIAR);
        Reg32 id = icciar & INT_ID_MASK;
        // Reg32 src = (icciar >> IAR_CPU_OFF) & IAR_CPU_MASK;

        // For every read of a valid interrupt from the ICCIAR, the ISR must perform a matching write to the ICCEOIR
        if(id <= 1022) {
            gic_cpu(ICCEOIR) = icciar;
            // only necessary when bit 9 of the ICC CTLR is active, i.e., ns eoi
            // gic_cpu(ICCDIR) = icciar;
        }
        return id;
    }

    void init() {

        for(unsigned int i =0; i < ((PPIS + SGIS)/32); i++){
            /**
             * Make sure all private interrupts are not enabled, non pending,
             * non active.
             */
            gic_dist(ICDICENABLER0 + i * sizeof(Reg32)) = -1;
            gic_dist(ICDCPENDSGIR0 + i * sizeof(Reg32)) = -1;
            gic_dist(ICDICACTIVER0 + i * sizeof(Reg32)) = -1;
        }

        /* Clear any pending SGIs. */
        for(unsigned int i = 0; i < (SGIS*SGI_BITS/32); i++){
            gic_dist(ICDCPENDSGIR0 + i * sizeof(Reg32)) = -1;
        }

        for(unsigned int i = 0; i < ((PPIS + SGIS)*TARGET_BITS/32); i++){
           gic_dist(ICDIPRIORITYR0 + i * sizeof(Reg32)) = -1;
        }

        for(unsigned int i = 0; i < ((PPIS + SGIS)*PRIO_BITS/32); i++){
           gic_dist(ICDIPRIORITYR0 + i * sizeof(Reg32)) = -1;
        }

        gic_cpu(ICCPMR) = 0xf0;
        gic_cpu(ICCCTLR) |= ICC_CTLR_EN; // | ICC_CTLR_EOI_NS | ICC_CTLR_IRQ_Bypass_DIS | ICC_CTLR_FIQ_Bypass_DIS;

        // gic_cpu(ICCDIR) = 0;
    }

protected:
    volatile Reg32 & gic_cpu(unsigned int o) { return reinterpret_cast<volatile Reg32 *>(Memory_Map::GICC_BASE)[o / sizeof(Reg32)]; }
    volatile Reg32 & gic_dist(unsigned int o) { return reinterpret_cast<volatile Reg32 *>(Memory_Map::GICD_BASE)[o / sizeof(Reg32)]; }
    
    // The versions bellow are dynamic and work on any A9, but are quite inefficient
    //    static volatile Reg32 & gic_cpu(unsigned int offset) {
    //        register Reg32 base asm("r0");
    //        ASM("mrc p15, 4, %0, c15, c0, 0" : "=r"(base)); // get private peripheral space base into r0
    //        return reinterpret_cast<volatile Reg32 *>(base + GICCPU)[offset / sizeof(Reg32)];
    //    }
};

__END_SYS

#endif
