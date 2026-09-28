// EPOS No_MMU Mediator Initialization

#include <architecture/mmu.h>
#include <system/memory_map.h>

extern "C" char _edata;
extern "C" char __bss_start;
extern "C" char _end;

__BEGIN_SYS

void No_MMU::init()
{
    db<Init, MMU>(TRC) << "MMU::init()" << endl;

    db<Init, MMU>(INF) << "MMU::init::dat.e=" << reinterpret_cast<void *>(&_edata) << ",bss.b=" << reinterpret_cast<void *>(&__bss_start) << ",bss.e=" << reinterpret_cast<void *>(&_end) << endl;

    // For machines that do not feature a real MMU, frame size = 1 byte
    // Allocations (using Grouping_List<Frame>::search_decrementing() start from the end
    // For multiheap on non-multitasking configurations (see Init_Application), the application's heap is at "_end" and we must skip it while initializing the MMU's free frame list.
    char * free_base = &_end;
    if(!Traits<System>::multitask) // heap in data segment
        free_base += Traits<Application>::STACK_SIZE;
    free(free_base, pages(Memory_Map::FREE_TOP + 1 - reinterpret_cast<unsigned long>(free_base)));
}

__END_SYS

