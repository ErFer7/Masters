// EPOS Xilinx Ultrascale+ ZU3EG - FZ3 - (ARM Cortex-A53) Run-Time System Information

#ifndef __fz3_info_h
#define __fz3_info_h

#include <system/info.h>

__BEGIN_SYS

struct System_Info: public System_Info_Common
{
    Boot_Map bm;
    Physical_Memory_Map pmm;
    Kernel_Load_Map lm;
};

__END_SYS

#endif
