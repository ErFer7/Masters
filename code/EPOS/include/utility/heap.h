// EPOS Heap_Base Utility Declarations

#ifndef __heap_h
#define __heap_h

#include <utility/debug.h>
#include <utility/list.h>
#include <utility/spin.h>

__BEGIN_UTIL

// Heap_Base
class Heap_Base: private Grouping_List<char>
{
protected:
    static const bool typed = Traits<System>::multiheap;

public:
    using Grouping_List<char>::empty;
    using Grouping_List<char>::size;
    using Grouping_List<char>::grouped_size;

    Heap_Base() {
        db<Init, Heaps>(TRC) << "Heap_Base() => " << this << endl;
    }

    Heap_Base(void * addr, unsigned long bytes) {
        db<Init, Heaps>(TRC) << "Heap_Base(addr=" << addr << ",bytes=" << bytes << ") => " << this << endl;

        free(addr, bytes);
    }

    void * alloc(unsigned long bytes) {
        db<Heaps>(TRC) << "Heap_Base::alloc(this=" << this << ",bytes=" << bytes;

        if(!bytes)
            return 0;

        if(!Traits<CPU>::unaligned_memory_access)
            while((bytes % sizeof(void *)))
                ++bytes;

        if(typed)
            bytes += sizeof(void *);  // add room for heap pointer
        bytes += sizeof(long);        // add room for size
        if(bytes < sizeof(Element))
            bytes = sizeof(Element);

        Element * e = search_decrementing(bytes);
        if(!e) {
            out_of_memory(bytes);
            return 0;
        }

        long * addr = reinterpret_cast<long *>(e->object() + e->size());

        if(typed)
            *addr++ = reinterpret_cast<long>(this);
        *addr++ = bytes;

        db<Heaps>(TRC) << ") => " << reinterpret_cast<void *>(addr) << endl;

        return addr;
    }

    void free(void * ptr, unsigned long bytes) {
        db<Heaps>(TRC) << "Heap_Base::free(this=" << this << ",ptr=" << ptr << ",bytes=" << bytes << ")" << endl;

        if(ptr && (bytes >= sizeof(Element))) {
            Element * e = new (ptr) Element(reinterpret_cast<char *>(ptr), bytes);
            Element * m1, * m2;
            insert_merging(e, &m1, &m2);
        }
    }

    static void typed_free(void * ptr) {
        long * addr = reinterpret_cast<long *>(ptr);
        unsigned long bytes = *--addr;
        Heap_Base * heap = reinterpret_cast<Heap_Base *>(*--addr);
        heap->free(addr, bytes);
    }

    static void untyped_free(Heap_Base * heap, void * ptr) {
        long * addr = reinterpret_cast<long *>(ptr);
        unsigned long bytes = *--addr;
        heap->free(addr, bytes);
    }

private:
    void out_of_memory(unsigned long bytes);
};


// Atomic heap
class Atomic_Heap: public Heap_Base
{
public:
    Atomic_Heap() {}
    Atomic_Heap(void * addr, unsigned int bytes): Heap_Base(addr, bytes) {}

    bool empty() {
        enter();
        bool tmp = Heap_Base::empty();
        leave();
        return tmp;
    }

    unsigned long size() {
        enter();
        unsigned long tmp = Heap_Base::size();
        leave();
        return tmp;
    }

    void * alloc(unsigned long bytes) {
        enter();
        void * tmp = Heap_Base::alloc(bytes);
        leave();
        return tmp;
    }

    void free(void * ptr, unsigned long bytes) {
        enter();
        Heap_Base::free(ptr, bytes);
        leave();
    }

private:
    void enter() { _lock.acquire(); }
    void leave() { _lock.release(); }

private:
    Simple_Spin _lock;
};

typedef IF<Traits<System>::multicore, Atomic_Heap, Heap_Base>::Result Heap;
typedef Heap Application_Heap;

__END_UTIL

#endif
