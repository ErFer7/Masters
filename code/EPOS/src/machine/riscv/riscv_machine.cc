// EPOS RISC-V Mediator Implementation

#include <machine/machine.h>
#include <machine/display.h>
#include <machine/pmic.h>

__BEGIN_SYS

void Machine::reboot()
{
    if(Traits<System>::reboot) {
        db<Machine>(TRC) << "Machine::reboot()" << endl;

#ifdef __visionfive2__
    if (CPU::id() == CPU::BSP) {
        CPU::int_disable();
        PMIC::shutdown(true);
    }
#endif

#ifdef __sifive_e__
        CPU::Reg * reset = reinterpret_cast<CPU::Reg *>(Memory_Map::AON_BASE);
        reset[0] = 0x5555;
#endif

        while(true) {
            CPU::int_disable();
            CPU::halt();
        }
    } else {
        poweroff();
    }
}

void Machine::poweroff()
{
    db<Machine>(TRC) << "Machine::poweroff()" << endl;

#ifdef __visionfive2__
    if (CPU::id() == CPU::BSP) {
        CPU::int_disable();
        PMIC::shutdown();
    }
#endif

#ifdef __sifive_e__
        CPU::Reg * reset = reinterpret_cast<CPU::Reg *>(Memory_Map::AON_BASE);
        reset[0] = 0x5555;
#endif

        while(true) {
            CPU::int_disable();
            CPU::halt();
        }
}

__END_SYS
