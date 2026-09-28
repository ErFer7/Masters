// EPOS ARMv8 MMU Mediator Implementation

#include <architecture/armv8/armv8_mmu.h>

__BEGIN_SYS

#ifdef __MMU_H

ARMv8_MMU::List ARMv8_MMU::_free[colorful * COLORS + 1];
ARMv8_MMU::Page_Directory * ARMv8_MMU::_master;

#endif

__END_SYS
