#pragma once

#include <architecture/cpu.h>
#include <machine/cache.h>
#include <system/memory_map.h>

__BEGIN_SYS

extern OStream kout;

class Cache : Cache_Common {
    typedef CPU::Reg8 Reg8;
    typedef CPU::Reg16 Reg16;
    typedef CPU::Reg32 Reg32;
    typedef CPU::Reg64 Reg64;

    enum {
        CONFIG = 0x000,
        L2_WAYS = 0x008,
        L2_FLUSH = 0x200,
    };

    static const bool enable_prefetcher = false;

    static const unsigned long L2_CACHE_LINE_SIZE = 64;

   public:
    static void init() {
        unsigned int config = reg32(CONFIG);
        unsigned int banks = masked(config, 0, 7);
        unsigned int ways = masked(config, 8, 15);
        unsigned int lg_sets = masked(config, 16, 23);
        unsigned int lg_block_bytes = masked(config, 24, 31);

        reg32(L2_WAYS) = ways - 1;

        db<Cache>(INF) << "Cache info: Banks: " << banks << ", Ways: " << ways << ", lgSets: " << lg_sets
                       << ", lgBlockBytes: " << lg_block_bytes << endl;

        for (unsigned long i = 0x800; i <= 0x8d0; i += 8) {
            reg32(i) = 0xffff;
        }

        // Enable the prefetcher for all cores
        if constexpr (enable_prefetcher) {
            reg32(0x2032000, 0) |= 1 | 1 << 28;
            reg32(0x2034000, 0) |= 1 | 1 << 28;
            reg32(0x2036000, 0) |= 1 | 1 << 28;
            reg32(0x2038000, 0) |= 1 | 1 << 28;
        }
    }

    static void flush(const void *const ptr, unsigned int size) {
        unsigned long line = reinterpret_cast<unsigned long>(ptr);
        unsigned long end = line + size;
        barrier();
        for (; line < end; line += L2_CACHE_LINE_SIZE) {
            reg64(L2_FLUSH) = line;
            barrier();
        }
    }

    static void barrier() { asm volatile("fence iorw, iorw" ::: "memory"); }

   private:
    static volatile Reg64 &reg64(unsigned int offset) {
        return *reinterpret_cast<volatile Reg64 *>(Memory_Map::L2_CACHE_BASE + offset);
    }

    static volatile Reg32 &reg32(unsigned int offset) {
        return reinterpret_cast<volatile Reg32 *>(Memory_Map::L2_CACHE_BASE)[offset / sizeof(Reg32)];
    }

    static volatile Reg32 &reg32(unsigned long base, unsigned int offset) {
        return reinterpret_cast<volatile Reg32 *>(base)[offset / sizeof(Reg32)];
    }

    static constexpr Reg32 mask(unsigned int mask_low = 0, unsigned int mask_high = 31) {
        return ((1ULL << (mask_high - mask_low + 1)) - 1) << mask_low;
    }

    static inline Reg32 masked(Reg32 value, unsigned int mask_low = 0, unsigned int mask_high = 31) {
        return (value & mask(mask_low, mask_high)) >> mask_low;
    }
};

__END_SYS
