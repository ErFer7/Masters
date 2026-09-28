// EPOS Raspberry Pi3 (ARM Cortex-A53) Mediator Implementation

#include <machine/machine.h>
#include <machine/display.h>
#include <system/memory_map.h>

__BEGIN_SYS

void FZ3::reboot()
{
    while(true);
}

__END_SYS
