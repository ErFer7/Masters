// EPOS Xilinx Ultrascale+ ZU3EG - FZ3 - (Cortex-A53) Mediator Declarations

#ifndef __fz3_machine_h
#define __fz3_machine_h

#include <machine/machine.h>
#include <system/memory_map.h>
#include <system.h>
#include <machine/cortex/fz3/fz3_ioctrl.h>

__BEGIN_SYS

class FZ3: private Machine_Common
{
    friend Machine; // for pre_init() and init()

protected:
    typedef CPU::Reg32 Reg32;
    typedef CPU::Log_Addr Log_Addr;

public:
    FZ3() {}

    using Machine_Common::delay;
    using Machine_Common::clear_bss;

    static void reboot();
    static void poweroff() { reboot(); }

    static const UUID & uuid() { return System::info()->bm.uuid; }

private:
    static void pre_init() {}
    static void init() {}
};

typedef FZ3 Machine_Model;

__END_SYS

#endif
