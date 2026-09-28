// EPOS RISC-V 64 System Call Function Implementation

#include <architecture/rv64/rv64_cpu.h>
#include <machine/ic.h>

__BEGIN_SYS

void CPU::syscall(void * message)
{
    CPU::a1(reinterpret_cast<CPU::Reg>(message));
#ifdef __visionfive2__
    CPU::a7(0);  // NOTE: This was added to make it safe for the machine_interrupt_handler
#endif
    CPU::ecall();
}

__END_SYS
