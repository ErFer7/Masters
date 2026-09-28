// EPOS Xilinx Ultrascale+ ZU3EG - FZ3 - (Cortex-A53) SETUP

#include <architecture.h>
#include <machine.h>
#include <system/memory_map.h>
#include <utility/elf.h>
#include <utility/string.h>


extern "C" unsigned int __bss_start;    // defined by GCC
extern "C" unsigned int _end;           // defined by GCC


extern "C" {

    void _start();
    void __init_begin();

    // SETUP entry point is the Vector Table and resides in the .init section (not in .text), so it will be linked first and will be the first function after the ELF header in the image.
    void _entry() __attribute__ ((used, naked, section(".init")));
    void _el1_entry() __attribute__ ((naked));
    void _vector_table() __attribute__ ((naked));
    void _reset() __attribute__ ((naked)); // so it can be safely reached from the vector table
    void _setup(); // just to create a Setup object

    // LD eliminates this variable while performing garbage collection, that's why the used attribute.
    char __boot_time_system_info[sizeof(EPOS::S::System_Info)] __attribute__ ((used)) = "<System_Info placeholder>"; // actual System_Info will be added by mkbi!
}

__BEGIN_SYS

extern OStream kout, kerr;

class Setup
{
private:
    // Physical memory map
    static const unsigned long RAM_BASE         = Memory_Map::RAM_BASE;
    static const unsigned long RAM_TOP          = Memory_Map::RAM_TOP;
    static const unsigned long MIO_BASE         = Memory_Map::MIO_BASE;
    static const unsigned long MIO_TOP          = Memory_Map::MIO_TOP;
    static const unsigned long IMAGE            = Memory_Map::IMAGE;
    static const unsigned long SETUP            = Memory_Map::SETUP;
    static const unsigned long FLAT_PAGE_TABLE  = Memory_Map::FLAT_PAGE_TABLE;
    static const unsigned long FREE_BASE        = Memory_Map::FREE_BASE;
    static const unsigned long FREE_TOP         = Memory_Map::FREE_TOP;

    // Architecture Imports
    typedef CPU::Reg Reg;
    typedef CPU::Reg32 Reg32;
    typedef CPU::Reg64 Reg64;
    typedef CPU::Phy_Addr Phy_Addr;
    typedef CPU::Log_Addr Log_Addr;

public:
    Setup();

private:
    void say_hi();
    void setup_flat_paging();
    void enable_paging();
    void call_next();

private:
    System_Info * si;

    static volatile bool paging_ready;
};

volatile bool Setup::paging_ready = false;

Setup::Setup()
{
    if(CPU::id() == 0) { // bootstrap CPU (BSP)


        db<Setup>(WRN) <<"BEGIN"<<endl;
        // Initialize the display so we can print
        Display::init();
        // while(1);

        kerr << endl;
        kout << endl;


        // Recover pointers to the boot image and to the System Info
        si = reinterpret_cast<System_Info *>(&__boot_time_system_info);
        if(si->bm.n_cpus > Traits<Machine>::CPUS)
            si->bm.n_cpus = Traits<Machine>::CPUS;

        db<Setup>(TRC) << "Setup(si=" << reinterpret_cast<void *>(si) << ",sp=" << CPU::sp() << ")" << endl;
        db<Setup>(INF) << "Setup:si=" << *si << endl;

        // Print basic facts about this EPOS instance
        say_hi();

        // Configure a flat memory model for the single task in the system
        // setup_flat_paging();

        // Enable paging
        // enable_paging();

        // Signalizes other CPUs that paging is up
        // paging_ready = true;

    } else { // additional CPUs (APs)

        while (1);
        // Wait for the Boot CPU to setup page tables
        // while(!paging_ready);
        // enable_paging();

    }

    if(CPU::id() == 0) {
        CPU::nAA_disable();
    }
    // SETUP ends here, so let's transfer control to the next stage (INIT or APP)
    call_next();
}

void Setup::setup_flat_paging()
{
    db<Setup>(TRC) << "Setup::setup_flat_paging()" << endl;

#ifdef __armv7__

    enum Section_Flags : unsigned long {
        ID   = 0b10 << 0,   // memory section identifier
        B    = 1 << 2,      // bufferable
        C    = 1 << 3,      // cacheable
        XN   = 1 << 4,      // execute never
        AP0  = 1 << 10,
        AP1  = 1 << 11,
        TEX0 = 1 << 12,
        TEX1 = 1 << 13,
        TEX2 = 1 << 14,
        AP2  = 1 << 15,
        S    = 1 << 16,     // shareable
        nG   = 1 << 17,     // non-global (entry in the TLB)
        nS   = 1 << 19,     // non-secure
        FLAT_MEMORY_MEM = (nS | S | AP1 | AP0 |       C | B | ID),
        FLAT_MEMORY_DEV = (nS | S | AP1 | AP0 |       C |     ID),
        FLAT_MEMORY_PER = (nS | S | AP1 | AP0 |  XN |     B | ID)
    };

    CPU::Reg * pt = reinterpret_cast<CPU::Reg *>(FLAT_PAGE_TABLE);
    for(CPU::Reg i = MMU_Common<12,0,20>::directory(RAM_BASE); i < MMU_Common<12,0,20>::directory(RAM_BASE) + MMU_Common<12,0,20>::pages(RAM_TOP - RAM_BASE); i++)
        pt[(i & 0x3ff)] = (i << 20) | Section_Flags::FLAT_MEMORY_MEM; // the mask prevents GCC warning about integer overflow and aggressive loop optimization
    pt[MMU_Common<12,0,20>::directory(MIO_BASE)] = (MMU_Common<12,0,20>::directory(MIO_BASE) << 20) | Section_Flags::FLAT_MEMORY_DEV;
    for(CPU::Reg i = MMU_Common<12,0,20>::directory(MIO_BASE) + 1; i < MMU_Common<12,0,20>::directory(MIO_BASE) + MMU_Common<12,0,20>::pages(MIO_TOP - MIO_BASE); i++)
        pt[i] = (i << 20) | Section_Flags::FLAT_MEMORY_PER;

#else

    // static const unsigned long PAGE_SIZE = 1 << 11;
    // static const unsigned long PAGE_SIZE        = 16 * 1024; // 16 KB

    // TTBR bits
    // enum Page_Flags : unsigned long {
    //     INVALID             = 0UL   << 0,
    //     RESERVED            = 0b01  << 0,
    //     PAGE_DESCRIPTOR     = 0b11  << 0,
    //     BLOCK_DESCRIPTOR    = 0b01  << 0,
    //     SEL_MAIR_ATTR0      = 0b00  << 2,
    //     SEL_MAIR_ATTR1      = 0b01  << 2,
    //     SEL_MAIR_ATTR2      = 0b10  << 2,
    //     SEL_MAIR_ATTR3      = 0b11  << 2,
    //     SEL_MAIR_ATTR4      = 0b100 << 2,
    //     SEL_MAIR_ATTR5      = 0b101 << 2,
    //     SEL_MAIR_ATTR6      = 0b110 << 2,
    //     SEL_MAIR_ATTR7      = 0b111 << 2,
    //     NON_SHAREABLE       = 0UL   << 8,
    //     UNPREDICTABLE       = 0b01  << 8,
    //     OUTER_SHAREABLE     = 0b10  << 8,
    //     INNER_SHAREABLE     = 0b11  << 8,
    //     NS_LEVEL1           = 0b1   << 5, // Output address is in secure address
    //     AP1                 = 0b1   << 6,
    //     AP2                 = 0b1   << 7,
    //     RW_SYS              = 0b0   << 6,
    //     RW_USR              = AP1,
    //     RO_SYS              = AP2,
    //     RO_USR              = (AP2 | AP1),
    //     ACCESS              = 0b1   << 10,
    //     nG                  = 1     << 11,
    //     CONTIGUOUS          = 0b1ULL<< 52, // Output memory is contiguous
    //     EL1_XN              = 0b1ULL<< 53, // el1 cannot execute
    //     XN                  = 0b1ULL<< 54, // el0 cannot execute
    //     DEV                 = SEL_MAIR_ATTR0,
    //     CWT                 = SEL_MAIR_ATTR1,
    //     CWB                 = SEL_MAIR_ATTR2,
    //     CD                  = SEL_MAIR_ATTR3,

    //     FLAT_MEM_PT         = (PAGE_DESCRIPTOR  | ACCESS),
    //     FLAT_MEM_BLOCK      = (BLOCK_DESCRIPTOR | ACCESS), //INNER_SHAREABLE | SEL_MAIR_ATTR0 | ACCESS),

    //     PTE                 = (PAGE_DESCRIPTOR | ACCESS),

    //     APP                 = (nG | INNER_SHAREABLE | RW_SYS | CWT | PTE),          // S, RWX  All, Normal WT
    //     APPD                = (nG | INNER_SHAREABLE | RW_SYS | CWT | XN  | PTE),    // S, RWnX All, Normal WT
    //     APPC                = (nG | INNER_SHAREABLE | RW_SYS | CWT | PTE),          // S, RnWX All, Normal WT
    //     SYS                 = (nG | INNER_SHAREABLE | RW_SYS | PTE),                // S, RWX  SYS, Normal WT
    //     IO                  = (nG | RW_USR | DEV | PTE),                            // Device Memory = Shareable, RWX, SYS
    //     DMA                 = (nG | RO_SYS | DEV | PTE),                            // Device Memory no cacheable / Old Peripheral = Shareable, RWX, B ?

    //     PT_MASK             = ((PAGE_SIZE - 1) | (0xfUL << 52)),
    //     PD_FLAGS            = (PAGE_DESCRIPTOR | XN | EL1_XN),
    //     PD_MASK             = ((PAGE_SIZE - 1) | (0xfUL << 52))
    // };

    // Single-level mapping, 32 MB blocks
    // static const unsigned long BLOCK_SIZE       = 32 * 1024 * 1024; // 32 MB
    // static const unsigned long PD_ENTRIES       = 128; //(Math::max(RAM_TOP, MIO_TOP) - Math::min(RAM_BASE, MIO_BASE)) / 32 * 1024 * 1024; // 128 for 4GB

    // CPU::Reg mem = RAM_BASE;
    // CPU::Reg * pd = reinterpret_cast<CPU::Reg *>(FLAT_PAGE_TABLE);

    // for(unsigned int i = 0; i < PD_ENTRIES; i++, mem += BLOCK_SIZE) {
    //     if (i < 0) {
    //         pd[i] = reinterpret_cast<CPU::Reg>(mem) | Page_Flags::FLAT_MEM_BLOCK | Page_Flags::INNER_SHAREABLE | Page_Flags::SEL_MAIR_ATTR4; //INNER_SHAREABLE | ACCESS;
    //     } else if (i < 10) {
    //         pd[i] = reinterpret_cast<CPU::Reg>(mem) | Page_Flags::FLAT_MEM_BLOCK | Page_Flags::SEL_MAIR_ATTR3; //INNER_SHAREABLE | ACCESS;
    //     } else {
    //         pd[i] = reinterpret_cast<CPU::Reg>(mem) | Page_Flags::FLAT_MEM_BLOCK | Page_Flags::SEL_MAIR_ATTR0; // | SEL_MAIR_ATTR0;
    //     }
    // }

   // Two-level mapping, 16KB granule
   // To used this code, remember to adjust FLAT_PAGE_TABLE to account for all the associated page tables (e.g. (RAM_TOP - (PD_ETRIES + 1) * 16 * 1024) & ~(0x3fff)
   // static const unsigned long PAGE_SIZE        = 16 * 1024; // 16 KB
   // static const unsigned long PD_ENTRIES       = 128; //(Math::max(RAM_TOP, MIO_TOP) - Math::min(RAM_BASE, MIO_BASE)) / 32 * 1024 * 1024; // 128 for 4GB
   // static const unsigned long PT_ENTRIES       = PAGE_SIZE / sizeof(long); // 2 K entries
   // static const unsigned long PD_MASK          = ((PAGE_SIZE - 1) | (0xfUL << 52));

   // CPU::Reg mem = RAM_BASE;
   // CPU::Reg * pts = reinterpret_cast<CPU::Reg *>(FLAT_PAGE_TABLE);

   // CPU::Reg * pd = reinterpret_cast<CPU::Reg *>(pts); // PD is the first page in pts_mem
   // pts += PAGE_SIZE;
   // for(unsigned int i = 0; i < PD_ENTRIES; i++, pts += PT_ENTRIES) {
   //      pd[i] = reinterpret_cast<CPU::Reg>(pts) | Page_Flags::FLAT_MEM_PT | Page_Flags::INNER_SHAREABLE;
   // }

   //  for(unsigned int i = 0; i < PD_ENTRIES; i++) {
   //      CPU::Reg * pt = reinterpret_cast<CPU::Reg *>(pd[i] & ~PD_MASK);
   //      db<Setup>(TRC) << "PT[" << i << "]: " << hex << pt << endl;
   //      for(unsigned int j = 0; j < PT_ENTRIES; j++, mem += PAGE_SIZE) {
   //          // db<Setup>(WRN) << "PT[" << i << "]: " << " - j = " << j << " - " << hex << pt << " - mem to map = " << mem << endl;
   //          if(i < 29) {
   //              pt[j] = reinterpret_cast<CPU::Reg>(mem) | Page_Flags::FLAT_MEM_BLOCK | Page_Flags::INNER_SHAREABLE | Page_Flags::SEL_MAIR_ATTR0;
   //          } else if (i < 31) {
   //              pt[j] = reinterpret_cast<CPU::Reg>(mem) | Page_Flags::FLAT_MEM_BLOCK | Page_Flags::INNER_SHAREABLE | Page_Flags::SEL_MAIR_ATTR0;
   //          } else {
   //              pt[j] = reinterpret_cast<CPU::Reg>(mem) | Page_Flags::FLAT_MEM_BLOCK | Page_Flags::INNER_SHAREABLE | Page_Flags::SEL_MAIR_ATTR0;
   //          }
   //      }
   // }
   //  // db<Setup>(WRN) << "SETUP MAIR" << endl;
   //  CPU::mair_el1(reinterpret_cast<unsigned long long>((0x00ul) << 0 * CPU::ATTR_OFFSET | // first attribute_index
   //      (0x04ul) << 1 * CPU::ATTR_OFFSET | // second attribute index
   //      (0x0cul) << 2 * CPU::ATTR_OFFSET | // third attribute index
   //      (0x44ul) << 3 * CPU::ATTR_OFFSET | // forth attribute index
   //      ((unsigned long long)0xfful) << 4 * CPU::ATTR_OFFSET)); // fifth attribute index


#define PAGESIZE    4096

// granularity
#define PT_PAGE     0b11        // 4k granule
#define PT_BLOCK    0b01        // 2M granule
// accessibility
#define PT_KERNEL   (0<<6)      // privileged, supervisor EL1 access only
#define PT_USER     (1<<6)      // unprivileged, EL0 access allowed
#define PT_RW       (0<<7)      // read-write
#define PT_RO       (1<<7)      // read-only
#define PT_AF       (1<<10)     // accessed flag
#define PT_NX       (1UL<<54)   // no execute
// shareability
#define PT_OSH      (2<<8)      // outter shareable
#define PT_ISH      (3<<8)      // inner shareable
// defined in MAIR register
#define PT_MEM      (0<<2)      // normal memory
#define PT_DEV      (1<<2)      // device MMIO
#define PT_NC       (2<<2)      // non-cachable

#define TTBR_CNP    1

/**
 * Set up page translation tables and enable virtual memory
 */
    // unsigned long data_page = (unsigned long)&_data/PAGESIZE;
    unsigned long r, b, *paging=(unsigned long*)FLAT_PAGE_TABLE;

    /* create MMU translation tables at _end */

    // TTBR0, identity L1
    paging[0]=(unsigned long)((unsigned char*)FLAT_PAGE_TABLE+2*PAGESIZE) |    // physical address
        PT_PAGE |     // it has the "Present" flag, which must be set, and we have area in it mapped by pages
        PT_AF |       // accessed flag. Without this we're going to have a Data Abort exception
        PT_USER |     // non-privileged
        PT_ISH |      // inner shareable
        PT_MEM;       // normal memory

    paging[1]=(unsigned long)((unsigned char*)FLAT_PAGE_TABLE+3*PAGESIZE) |    // physical address
        PT_PAGE |     // it has the "Present" flag, which must be set, and we have area in it mapped by pages
        PT_AF |       // accessed flag. Without this we're going to have a Data Abort exception
        PT_USER |     // non-privileged
        PT_ISH |      // inner shareable
        PT_MEM;       // normal memory

    // identity L2, first 18M of memory, 9 * 2M block - I guess it could be 8 for 16 MB, but lets make it for sure
    for (r=0;r<=8;r++)
        paging[2*512+r]=(unsigned long)((unsigned char*)FLAT_PAGE_TABLE+4*PAGESIZE+r*PAGESIZE) | // physical address
            PT_PAGE |     // we have area in it mapped by pages
            PT_AF |       // accessed flag
            PT_USER |     // non-privileged
            PT_ISH |      // inner shareable
            PT_MEM;       // normal memory

    // // identity L2, first 2M block
    // paging[2*512+496]=(unsigned long)((unsigned char*)FLAT_PAGE_TABLE+6*PAGESIZE) | // physical address
    //     PT_PAGE |     // we have area in it mapped by pages
    //     PT_AF |       // accessed flag
    //     PT_USER |     // non-privileged
    //     PT_ISH |      // inner shareable
    //     PT_MEM;       // normal memory

    // identity L2 2M blocks
    // skip 0th, as we're about to map it by L3
    for(r=9;r<512;r++)
        paging[2*512+r]=(unsigned long)((r<<21)) |  // physical address
        PT_BLOCK |    // map 2M block
        PT_AF |       // accessed flag
        // PT_NX |       // no execute
        PT_USER |     // non-privileged
        (PT_OSH|PT_DEV); // different attributes for device memory - PT_ISH|PT_MEM - for devices

    for(r=0;r<512;r++)
        paging[3*512+r]=(unsigned long)(((r+512)<<21)) |  // physical address
        PT_BLOCK |    // map 2M block
        PT_AF |       // accessed flag
        // PT_NX |       // no execute
        PT_USER |     // non-privileged
        (PT_OSH|PT_DEV); // different attributes for device memory
    // db<Init>(WRN) << "VERIFY THIS ADDR: " << Traits<Machine>::APP_DATA/PAGESIZE << endl;

    // identity L3
    unsigned long out_loop;
    for (out_loop=0;out_loop<9;out_loop++)
        for(r=0;r<512;r++)
            paging[(out_loop+4)*512+r]=(unsigned long)(r*PAGESIZE+out_loop*PAGESIZE) |   // physical address
            PT_PAGE |     // map 4k
            PT_AF |       // accessed flag
            ((r < 0x80) ? PT_KERNEL : PT_USER) |     // non-privileged
            PT_ISH |      // inner shareable
            ((r < 0x80) ? PT_RW : PT_RO); // different for code and data

    // TTBR1, kernel L1
    paging[3]=(unsigned long)((unsigned char*)FLAT_PAGE_TABLE+13*PAGESIZE) | // physical address
        PT_PAGE |     // we have area in it mapped by pages
        PT_AF |       // accessed flag
        PT_USER |   // privileged
        PT_ISH |      // inner shareable
        PT_MEM;       // normal memory

    // kernel L2
    for(r=504;r<512;r++)
    paging[13*512+r]=(unsigned long)((unsigned char*)FLAT_PAGE_TABLE+14*PAGESIZE+(r-504)*PAGESIZE) |   // physical address
        PT_PAGE |     // we have area in it mapped by pages
        PT_AF |       // accessed flag
        PT_USER |   // privileged
        PT_ISH |      // inner shareable
        PT_MEM;       // normal memory

    // kernel L3
    for(out_loop=0;out_loop<8;out_loop++)    
        for(r=0;r<512;r++)
            paging[14*512+r+out_loop*512]=(unsigned long)(Memory_Map::PPS_BASE+r*PAGESIZE+out_loop*512*PAGESIZE) |   // physical address
                PT_PAGE |     // map 4k
                PT_AF |       // accessed flag
                // PT_NX |       // no execute
                PT_USER |   // privileged
                PT_OSH |      // outter shareable
                PT_DEV;       // device memory

    /* okay, now we have to set system registers to enable MMU */

    // check for 4k granule and at least 36 bits physical address bus */
    asm volatile ("mrs %0, id_aa64mmfr0_el1" : "=r" (r));
    b=r&0xF;
    if(r&(0xF<<28)/*4k*/ || b<1/*36 bits*/) {
        db<Init>(WRN) << "ERROR: 4k granule or 36 bit address space not supported" << endl;
        return;
    }
    ASM("stupid_bp_1: nop");
    // first, set Memory Attributes array, indexed by PT_MEM, PT_DEV, PT_NC in our example
    r=  (0xFF << 0) |    // AttrIdx=0: normal, IWBWA, OWBWA, NTR
        (0x04 << 8) |    // AttrIdx=1: device, nGnRE (must be OSH too)
        (0x44 <<16);     // AttrIdx=2: non cacheable
    asm volatile ("msr mair_el1, %0" : : "r" (r));

    // next, specify mapping characteristics in translate control register
    r=  (0b00LL << 37) | // TBI=0, no tagging
        (b << 32) |      // IPS=autodetected
        (0b10LL << 30) | // TG1=4k
        (0b11LL << 28) | // SH1=3 inner
        (0b01LL << 26) | // ORGN1=1 write back
        (0b01LL << 24) | // IRGN1=1 write back
        (0b0LL  << 23) | // EPD1 enable higher half
        (25LL   << 16) | // T1SZ=25, 3 levels (512G)
        (0b00LL << 14) | // TG0=4k
        (0b11LL << 12) | // SH0=3 inner
        (0b01LL << 10) | // ORGN0=1 write back
        (0b01LL << 8) |  // IRGN0=1 write back
        (0b0LL  << 7) |  // EPD0 enable lower half
        (25LL   << 0);   // T0SZ=25, 3 levels (512G)
    asm volatile ("msr tcr_el1, %0; isb" : : "r" (r));

    ASM("stupid_bp_2: nop");
    // tell the MMU where our translation tables are. TTBR_CNP bit not documented, but required
    // lower half, user space
    asm volatile ("msr ttbr0_el1, %0" : : "r" ((unsigned long)FLAT_PAGE_TABLE + TTBR_CNP));
    // upper half, kernel space
    asm volatile ("msr ttbr1_el1, %0" : : "r" ((unsigned long)FLAT_PAGE_TABLE + TTBR_CNP + PAGESIZE));

    ASM("stupid_bp_3: nop");
    // finally, toggle some bits in system control register to enable page translation
    asm volatile ("dsb ish; isb; mrs %0, sctlr_el1" : "=r" (r));
    r|=0xC00800;     // set mandatory reserved bits
    r&=~((1<<25) |   // clear EE, little endian translation tables
         (1<<24) |   // clear E0E
         (1<<19) |   // clear WXN
         (1<<12) |   // clear I, no instruction cache
         (1<<4) |    // clear SA0
         (1<<3) |    // clear SA
         (1<<2) |    // clear C, no cache at all
         (1<<1));    // clear A, no aligment check
    r|=  (1<<0);     // set M, enable MMU 1<< 2 | 1 << 12 | 
    asm volatile ("msr sctlr_el1, %0" : : "r" (r));

#endif
}


void Setup::say_hi()
{
    db<Setup>(TRC) << "Setup::say_hi()" << endl;
    db<Setup>(INF) << "System_Info=" << *si << endl;

    kout << endl;
    kout << "\n*** This is EPOS!\n" << endl;
    kout << "Setting up this machine as follows: " << endl;
    kout << "  Mode:         " << ((Traits<Build>::SMOD == Traits<Build>::LIBRARY) ? "library" : (Traits<Build>::SMOD == Traits<Build>::BUILTIN) ? "built-in" : "kernel") << endl;
    kout << "  Processor:    " << Traits<Machine>::CPUS << " x Cortex A53 (ARMv" << ((Traits<CPU>::WORD_SIZE == 32) ? "7" : "8") << "-A) at " << Traits<CPU>::CLOCK / 1000000 << " MHz (BUS clock = " << Traits<CPU>::CLOCK / 1000000 << " MHz)" << endl;
    kout << "  Machine:      Xilinx FZ3" << endl;
    kout << "  Memory:       " << (si->bm.mem_top - si->bm.mem_base) / 1024 << " KB [" << reinterpret_cast<void *>(si->bm.mem_base) << ":" << reinterpret_cast<void *>(si->bm.mem_top) << "]" << endl;
    kout << "  User memory:  " << (si->pmm.usr_mem_top - si->pmm.usr_mem_base) / 1024 << " KB [" << reinterpret_cast<void *>(si->pmm.usr_mem_base) << ":" << reinterpret_cast<void *>(si->pmm.usr_mem_top) << "]" << endl;
    kout << "  I/O space:    " << (si->bm.mio_top - si->bm.mio_base) / 1024 << " KB [" << reinterpret_cast<void *>(si->bm.mio_base) << ":" << reinterpret_cast<void *>(si->bm.mio_top) << "]" << endl;
    kout << "  Node Id:      ";
    if(si->bm.node_id != -1)
        kout << si->bm.node_id << " (" << Traits<Build>::NODES << ")" << endl;
    else
        kout << "will get from the network!" << endl;
    kout << "  Position:     ";
    if(si->bm.space_x != -1)
        kout << "(" << si->bm.space_x << "," << si->bm.space_y << "," << si->bm.space_z << ")" << endl;
    else
        kout << "will get from the network!" << endl;
    if(si->lm.has_stp)
        kout << "  Setup:        " << si->lm.stp_code_size + si->lm.stp_data_size << " bytes" << endl;
    if(si->lm.has_ini)
        kout << "  Init:         " << si->lm.ini_code_size + si->lm.ini_data_size << " bytes" << endl;
    if(si->lm.has_sys)
        kout << "  OS code:      " << si->lm.sys_code_size << " bytes" << "\tdata: " << si->lm.sys_data_size << " bytes" << "   stack: " << si->lm.sys_stack_size << " bytes" << endl;
    if(si->lm.has_app)
        kout << "  APP code:     " << si->lm.app_code_size << " bytes" << "\tdata: " << si->lm.app_data_size << " bytes" << endl;
    if(si->lm.has_ext)
        kout << "  Extras:       " << si->lm.app_extra_size << " bytes" << endl;

    kout << endl;
}


void Setup::enable_paging()
{
    // db<Setup>(WRN) << "Setup::enable_paging()" << endl;
    if(Traits<Setup>::hysterically_debugged) {
        db<Setup>(INF) << "Setup::pc=" << CPU::pc() << endl;
        db<Setup>(INF) << "Setup::sp=" << CPU::sp() << endl;
    }

#ifdef __armv7__

    // MNG_DOMAIN for no page permission verification, CLI_DOMAIN otherwise
    CPU::dacr(CPU::MNG_DOMAIN);

    CPU::dsb();
    CPU::isb();

    // Clear TTBCR for the system to use ttbr0 instead of 1
    CPU::ttbcr(0);
    // Set ttbr0 with base address
    CPU::ttbr0(FLAT_PAGE_TABLE);

    // Enable MMU through SCTLR and ACTLR
    CPU::actlr(CPU::actlr() | CPU::SMP); // Set SMP bit
    CPU::sctlr((CPU::sctlr() | CPU::DCACHE | CPU::ICACHE | CPU::MMU_ENABLE) & ~(CPU::AFE)); //| CPU::DCACHE | CPU::ICACHE | CPU::MMU_ENABLE) & ~(CPU::AFE));

    CPU::dsb();
    CPU::isb();

    // MMU now enabled - Virtual address system now active
    // Branch Prediction Enable
    CPU::sctlr(CPU::sctlr() | CPU::BRANCH_PRED);

    // Flush TLB to ensure we've got the right memory organization
    MMU::flush_tlb();

    // Adjust pointers that will still be used to their logical addresses
    Display::init(); // adjust the pointers in Display by calling init

    if(Traits<Setup>::hysterically_debugged) {
        db<Setup>(INF) << "pc=" << CPU::pc() << endl;
        db<Setup>(INF) << "sp=" << CPU::sp() << endl;
    }

#else 

    Reg b, r;


    asm volatile ("mrs %0, id_aa64mmfr0_el1" : "=r" (r));
    b=r&0xF;
    if(r&(0xF<<28)/*4k*/ || b<1/*36 bits*/) {
        db<Init>(WRN) << "ERROR: 4k granule or 36 bit address space not supported" << endl;
        return;
    }
    // ASM("stupid_bp_1: nop");
    // first, set Memory Attributes array, indexed by PT_MEM, PT_DEV, PT_NC in our example
    r=  (0xFF << 0) |    // AttrIdx=0: normal, IWBWA, OWBWA, NTR
        (0x04 << 8) |    // AttrIdx=1: device, nGnRE (must be OSH too)
        (0x44 <<16);     // AttrIdx=2: non cacheable
    asm volatile ("msr mair_el1, %0" : : "r" (r));

    // next, specify mapping characteristics in translate control register
    r=  (0b00LL << 37) | // TBI=0, no tagging
        (b << 32) |      // IPS=autodetected
        (0b10LL << 30) | // TG1=4k
        (0b11LL << 28) | // SH1=3 inner
        (0b01LL << 26) | // ORGN1=1 write back
        (0b01LL << 24) | // IRGN1=1 write back
        (0b0LL  << 23) | // EPD1 enable higher half
        (25LL   << 16) | // T1SZ=25, 3 levels (512G)
        (0b00LL << 14) | // TG0=4k
        (0b11LL << 12) | // SH0=3 inner
        (0b01LL << 10) | // ORGN0=1 write back
        (0b01LL << 8) |  // IRGN0=1 write back
        (0b0LL  << 7) |  // EPD0 enable lower half
        (25LL   << 0);   // T0SZ=25, 3 levels (512G)
    asm volatile ("msr tcr_el1, %0; isb" : : "r" (r));

    // ASM("stupid_bp_2: nop");
    // tell the MMU where our translation tables are. TTBR_CNP bit not documented, but required
    // lower half, user space
    asm volatile ("msr ttbr0_el1, %0" : : "r" ((unsigned long)FLAT_PAGE_TABLE + TTBR_CNP));
    // upper half, kernel space
    asm volatile ("msr ttbr1_el1, %0" : : "r" ((unsigned long)FLAT_PAGE_TABLE + TTBR_CNP + PAGESIZE));

    // ASM("stupid_bp_3: nop");
    // finally, toggle some bits in system control register to enable page translation
    asm volatile ("dsb ish; isb; mrs %0, sctlr_el1" : "=r" (r));
    r|=0xC00800;     // set mandatory reserved bits
    r&=~((1<<25) |   // clear EE, little endian translation tables
         (1<<24) |   // clear E0E
         (1<<19) |   // clear WXN
         (1<<12) |   // clear I, no instruction cache
         (1<<4) |    // clear SA0
         (1<<3) |    // clear SA
         (1<<2) |    // clear C, no cache at all
         (1<<1));    // clear A, no aligment check
    r|=  (1<<0);     // set M, enable MMU 1<< 2 | 1 << 12 | 
    asm volatile ("msr sctlr_el1, %0" : : "r" (r));

    // Configure paging with two levels and 16KB pages via TTBRC
    // CPU::ttbcr(CPU::TTBR1_DISABLE | CPU::TTBR0_WALK_INNER_SHAREABLE | CPU::TTBR0_WALK_OUTER_WB_WA | CPU::TTBR0_WALK_INNER_WB_WA | CPU::TTBR0_TG0_16KB | CPU::TTBR0_SIZE_64GB);
    // CPU::isb();

    // // Tell the MMU where our translation tables are
    // db<Setup>(WRN) << "SYS_PD=" << reinterpret_cast<void *>(FLAT_PAGE_TABLE) << endl;
    // CPU::ttbr0(FLAT_PAGE_TABLE);

    // CPU::dsb();
    // CPU::isb();

    // CPU::sctlr(CPU::sctlr() | 0xC00800 | CPU::MMU_ENABLE | CPU::DCACHE | CPU::ICACHE);
    // // CPU::isb();

    Display::init();

#endif
}


void Setup::call_next()
{
    db<Setup>(WRN) << "SETUP ends here!" << endl;

    // Call the next stage
    static_cast<void (*)()>(_start)();

    // SETUP is now part of the free memory and this point should never be reached, but, just in case ... :-)
    db<Setup>(WRN) << "OS failed to init!" << endl;
}

__END_SYS

using namespace EPOS::S;

#ifdef __armv7__

void _entry()
{
    // Interrupt Vector Table
    // We use and indirection table for the ldr instructions because the offset can be to far from the PC to be encoded
    ASM("               ldr pc, .reset                                          \t\n\
                        ldr pc, .ui                                             \t\n\
                        ldr pc, .si                                             \t\n\
                        ldr pc, .pa                                             \t\n\
                        ldr pc, .da                                             \t\n\
                        ldr pc, .reserved                                       \t\n\
                        ldr pc, .irq                                            \t\n\
                        ldr pc, .fiq                                            \t\n\
                                                                                \t\n\
                        .balign 32                                              \t\n\
        .reset:         .word _reset                                            \t\n\
        .ui:            .word 0x0                                               \t\n\
        .si:            .word 0x0                                               \t\n\
        .pa:            .word 0x0                                               \t\n\
        .da:            .word 0x0                                               \t\n\
        .reserved:      .word 0x0                                               \t\n\
        .irq:           .word 0x0                                               \t\n\
        .fiq:           .word 0x0                                               \t");
}

void _reset()
{
    // QEMU get us here in SVC mode with interrupt disabled, but the real FZ3 starts in hypervisor mode, so we must switch to SVC mode
    if(!Traits<Machine>::emulated) {
        CPU::Reg cpsr = CPU::psr();
        cpsr &= ~CPU::FLAG_M;           // clear mode bits
        cpsr |= CPU::MODE_SVC;          // set supervisor flag
        CPU::psr(cpsr);                 // enter supervisor mode
        CPU::Reg address = CPU::ra();
        CPU::elr_hyp(address);
        CPU::tmp_to_cpsr();
    }

    // Configure a stack for SVC mode, which will be used until the first Thread is created
    CPU::mode(CPU::MODE_SVC); // enter SVC mode (with interrupts disabled)
    CPU::sp(Memory_Map::BOOT_STACK + Traits<Machine>::STACK_SIZE * (CPU::id() + 1) - sizeof(long));
    CPU::int_disable(); // interrupts will be re-enabled at init_end

    BCM_Mailbox * mbox = reinterpret_cast<BCM_Mailbox *>(Memory_Map::MBOX_CTRL_BASE);
    if(CPU::id() == 0) {
        for(unsigned int i = 1; i < Traits<Build>::CPUS; i++)
            mbox->start(i, Traits<Machine>::RESET);

        // After a reset, we copy the vector table to 0x0000 to get a cleaner memory map (it is originally at 0x8000)
        // An alternative would be to set vbar address via mrc p15, 0, r1, c12, c0, 0
        CPU::r0(reinterpret_cast<CPU::Reg>(&_entry)); // load r0 with the source pointer
        CPU::r1(Memory_Map::VECTOR_TABLE); // load r1 with the destination pointer

        // Copy the first 32 bytes
        CPU::ldmia(); // load multiple registers from the memory pointed by r0 and auto-increment it accordingly
        CPU::stmia(); // store multiple registers to the memory pointed by r1 and auto-increment it accordingly

        // Repeat to copy the subsequent 32 bytes
        CPU::ldmia();
        CPU::stmia();

        // Clear the BSS (SETUP was linked to CRT0, but entry point didn't go through BSS clear)
        Machine::clear_bss();
    } else {
        mbox->eoi(0);
        mbox->enable();
    }

    // Set VBAR to point to the relocated the vector table
    CPU::vbar(Memory_Map::VECTOR_TABLE);

    CPU::flush_caches();
    CPU::flush_branch_predictors();
    CPU::flush_tlb();
    CPU::actlr(CPU::actlr() | CPU::DCACHE_PREFETCH); // enable D-side prefetch

    CPU::dsb();
    CPU::isb();

    _setup();
}

#else

void _entry()
{
    // Manual D13.2.102 explains the cold start of an ARMv8, with RVBAR_EL3 defined by the SoC's manufaturer
    // Broadcom uses 0 and the GPU initialized that memory region with code that end up jumping to 0x80000 (i.e. _entry)

    if (!Traits<System>::multicore) {
        while (CPU::id() != 0);
    }

    // Set SP to avoid early faults
    CPU::sp(Memory_Map::BOOT_STACK + Traits<Machine>::STACK_SIZE * (CPU::id() + 1)); // set stack
    

    ASM("b _reset");
}

void _vector_table()
{
    // Manual D1.10.2, page D1-1430 shows four configurations in terms of the exception level an exception can came from:
    // current level with SP_EL0, current level with SP_ELx, lower level AArch64, and lower level AArch32.
    // Additionally, there are four exception types: synchronous exception, IRQ, FIQ, and SError.
    // Therefore, each exception level maps the four exception type handlers aligned by 128 bytes (enough room to write simple handlers).

    // Our strategy is to forward all interrupts to _int_entry via .ic_entry. Since x30 (LR) contains the return address and will be adjusted by the "blr" instruction,
    // we save x29 to reach _int_entry. The value of .ic_entry will be set by IC::init(). To match ARMv7, we also disable interrupts (which are reenabled at dispatch).
    ASM("// Current EL with SP0                                                 \t\n\
                        .balign 128                                             \t\n\
        .sync_curr_sp0: str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .irq_curr_sp0:  str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .fiq_curr_sp0:  str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .error_curr_sp0:str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
        // Current EL with SPx                                                  \t\n\
                        .balign 128                                             \t\n\
        .sync_curr_spx: str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .irq_curr_spx:  str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .fiq_curr_spx:  str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .error_curr_spx:str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
        // Lower EL using AArch64                                               \t\n\
                        .balign 128                                             \t\n\
        .sync_lower64:  str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .irq_lower64:   str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .fiq_lower64:   str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .error_lower64: str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        // Lower EL using AArch32                               \t\n\
                        .balign 128                                             \t\n\
        .sync_lower32:  str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .irq_lower32:   str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .fiq_lower32:   str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .error_lower32: str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .ic_entry:      .dword 0x0             // set by IC::init()             \t");
}

void _reset()
{

    CPU::int_disable(); // interrupts will be re-enabled at init_end
    if(CPU::id() == 0) {
        // 64 bits Multicore CPU start SPIN MMIO
        if (!Traits<Machine>::emulated && Traits<System>::multicore) {
            enum {
                CORE0_SPIN_SET          = 0xd8, // https://github.com/raspberrypi/tools/blob/master/armstubs/armstub8.S#L156
                CORE_SPIN_OFFSET        = 0x08
            };

            CPU::Reg64 * spin = reinterpret_cast<CPU::Reg64 *>(CORE0_SPIN_SET);
            for(unsigned int i = 1; i < Traits<Build>::CPUS; i++)
                spin[i] = Traits<Machine>::RESET;
        }

        // if (Traits<Machine>::emulated) {
        // Relocated the vector table, which has 4 entries for each of the 4 scenarios, all 128 bytes aligned, plus an 8 bytes pointer, totaling 2056 bytes
        CPU::Reg * src = reinterpret_cast<CPU::Reg *>(&_vector_table);
        CPU::Reg * dst = reinterpret_cast<CPU::Reg *>(Memory_Map::VECTOR_TABLE);
        for(int i = 0; i < (2056 / 8); i++)
            dst[i] = src[i];
        // }
    }

    // Set EL1 VBAR to the relocated vector table
    // if (Traits<Machine>::emulated)
    CPU::vbar_el1(static_cast<CPU::Phy_Addr>(Memory_Map::VECTOR_TABLE));
    CPU::vbar_el2(static_cast<CPU::Phy_Addr>(Memory_Map::VECTOR_TABLE));
    // else
        // CPU::vbar_el1(static_cast<CPU::Phy_Addr>(&_vector_table));

    // if (CPU::id() == 0) {

    // Activate aarch64
        CPU::hcr(CPU::EL1_AARCH64_EN | CPU::SWIO_HARDWIRED);
        // We start at EL2, but must set EL1 SP for a smooth transition, including further exception/interrupt handling
        CPU::spsr_el2(CPU::FLAG_D | CPU::FLAG_A | CPU::FLAG_I | CPU::FLAG_F | CPU::FLAG_EL1 | CPU::FLAG_SP_ELn);
        // CPU::Reg el1_addr = CPU::pc();
        // el1_addr += 16; // previous instruction, this instruction, and the next one;
        CPU::elr_el2(reinterpret_cast<CPU::Reg>(&_el1_entry));

        CPU::eret();
        ASM("           \t\n\
            nop         \t\n\
            .global _el1_entry \t\n\
            _el1_entry:  \t\n\
                nop     \t\n\
            ");
        
        CPU::sp(Memory_Map::BOOT_STACK + Traits<Machine>::STACK_SIZE * (CPU::id() + 1)); // set stack

    // }
    if(CPU::id() == 0) {
        // Clear the BSS (SETUP was linked to CRT0, but entry point didn't go through BSS clear)
        Machine::clear_bss();
    }

    /*
    ASM("mov    x0, #0x0800  \n\t\
        movk    x0, #0x30d0, lsl #16 \n\t\
        orr    x0, x0, #(0x1 << 2)            // The C bit on (data cache).  \n\t\
        orr    x0, x0, #(0x1 << 12)           // The I bit on (instruction cache) \n\t\
        msr sctlr_el1, x0 ");
    */

    _setup();
} 

#endif

void _setup()
{
    Setup setup;
}

/*
    // TEST NEW UART
    ASM("\t\n\
        exec_main:\t\n\
            //b main\t\n\
                                \t\n\
            mrs x0, MPIDR_EL1\t\n\
            and x0, x0, #0xf\t\n\
            cbnz x0, wait\t\n\
            b my_main_here\t\n\
\t\n\
        wait:\t\n\
            b .\t\n\
\t\n\
        my_main_here:\t\n\
            //stp x29, x30, [sp, #-32]!\t\n\
            //mov x29, sp\t\n\
            //str x19, [sp, #16]\t\n\
            // load uart address to x0, 0xff000000\t\n\
            mov x0, #0xff000000\t\n\
            //lsl x0, x19, #20\t\n\
            bl my_xil_uart_init\t\n\
            mov x0, #0xff000000\t\n\
            //lsl x0, x19, #20\t\n\
            mov x0, #0xff000000\t\n\
            bl my_xil_uart_disable\t\n\
            bl my_xil_uart_enable\t\n\
            //mov x0, #0xff000000\t\n\
            //bl main\t\n\
            //mov w0, #0x48 // set w0\t\n\
            //bl my_put_char\t\n\
            //bl main\t\n\
            \t\n\
            nop\t\n\
            nop\t\n\
            \t\n\
        clean_uart_buffer:\t\n\
            nop\t\n\
            nop\t\n\
            mov w0, #0x48 // set w0\t\n\
            bl my_put_char\t\n\
            nop\t\n\
            nop\t\n\
            mov w0, #0x65\t\n\
            bl my_put_char\t\n\
            nop\t\n\
            nop\t\n\
            mov w0, #0x6c\t\n\
            bl my_put_char\t\n\
            nop\t\n\
            nop\t\n\
            mov w0, #0x6c\t\n\
            bl my_put_char\t\n\
            nop\t\n\
            nop\t\n\
            mov w0, #0x6f\t\n\
            bl my_put_char\t\n\
            nop\t\n\
            nop\t\n\
            mov w0, #0xa\t\n\
            bl my_put_char\t\n\
            nop\t\n\
            nop\t\n\
\t\n\
            mov x19, #3\t\n\
        loop_wait_uart_ready:\t\n\
            nop\t\n\
            mov w0, #0x48 // set w0\t\n\
            bl my_put_char\t\n\
            //bl main\t\n\
            mov w0, #0x65\t\n\
            bl my_put_char\t\n\
            //bl main\t\n\
            mov w0, #0x6c\t\n\
            bl my_put_char\t\n\
            //bl main\t\n\
            mov w0, #0x6c\t\n\
            bl my_put_char\t\n\
            //bl main\t\n\
            mov w0, #0x6f\t\n\
            bl my_put_char\t\n\
            //bl main\t\n\
            //mov w0, #0x33\t\n\
            //bl my_put_char\t\n\
            //bl main\t\n\
            mov w0, #0xa\t\n\
            bl my_put_char\t\n\
            nop\t\n\
            nop\t\n\
            add x19, x19, #-1\t\n\
            cbnz x19, loop_wait_uart_ready\t\n\
\t\n\
            b end_exec1\t\n\
\t\n\
        my_put_char:\t\n\
            mov w1, w0 // put char on w1\t\n\
            mov x0, #0xff000000\t\n\
            //lsl x0, x19, #20\t\n\
            nop\t\n\
            sxtb w1, w1\t\n\
        my_xil_loop_putc:\t\n\
            ldr w2, [x0, #44]\t\n\
            tbnz w2, #4, my_xil_loop_putc\t\n\
            str w1, [x0, #48]\t\n\
            nop\t\n\
            nop\t\n\
            ret  \t\n\
\t\n\
        my_xil_uart_init:\t\n\
            mov x1, x0 //copy uart address to x1\t\n\
            mov w2, #0x28\t\n\
            mov w0, #0x8f\t\n\
            mov w4, #0x5\t\n\
            mov w6, #0x3\t\n\
            mov w5, #0x14\t\n\
            str w2, [x1]\t\n\
            mov w3, #0x1\t\n\
            str w0, [x1, #24]\t\n\
            mov w2, #0xffffffff\t\n\
            str w4, [x1, #52]\t\n\
            mov w0, w3\t\n\
            str w6, [x1]\t\n\
            mov w4, #0x103\t\n\
            str w5, [x1]\t\n\
            str w3, [x1, #32]\t\n\
            str wzr, [x1, #28]\t\n\
            str w2, [x1, #20]\t\n\
            str w3, [x1, #8]\t\n\
            ldr w2, [x1]\t\n\
            orr w2, w2, w4\t\n\
            str w2, [x1]\t\n\
            ret\t\n\
\t\n\
        my_xil_uart_disable:\t\n\
            ldr w2, [x0]\t\n\
            mov w2, #0x128\t\n\
            str w2, [x0]\t\n\
            ret\t\n\
\t\n\
        my_xil_uart_enable:\t\n\
            ldr w2, [x0]\t\n\
            mov w1, #0x117\t\n\
            str w1, [x0]\t\n\
            ret\t\n\
\t\n\
        my_xil_uart_baud_rate:\t\n\
            mov x1, x0\t\n\
            mov w6, #0x28\t\n\
            mov w5, #0x8f\t\n\
            mov w4, #0x5\t\n\
            mov w3, #0x3\t\n\
            mov w2, #0x14\t\n\
            str w6, [x1]\t\n\
            mov w0, #0x1\t\n\
            str w5, [x1, #24]\t\n\
            str w4, [x1, #52]\t\n\
            str w3, [x1]\t\n\
            str w2, [x1]\t\n\
            ret\t\n\
\t\n\
        end_exec1:\t\n\
            nop\t\n\
            nop\t\n\
            nop\t\n\
            nop\t\n\
            nop\t\n\
            nop\t\n\
    ");

*/