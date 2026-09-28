#pragma once

#include "utility/random.h"

using namespace EPOS;

namespace PointerChase {
class PointerChase {
   public:
    static const unsigned int ITERATIONS = 1000;
    static const unsigned int CACHE_LINE_SIZE = 64;
    static const unsigned int L1_CACHE_SIZE = 32 * 1024;
    static const unsigned int L2_CACHE_SIZE = 2 * 1024 * 1024;

   private:
    struct Rng {
        unsigned int state{0x12345678};

        unsigned int next() {
            unsigned int x = state;
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            return state = x;
        }

        unsigned int range(unsigned int max_exclusive) {
            if (max_exclusive <= 1) return 0;
            return next() % max_exclusive;
        }
    };

   public:
    explicit PointerChase(unsigned long size)
        : _size(size),
          _block_count(size / CACHE_LINE_SIZE),
          _raw(nullptr),
          _buffer(nullptr),
          _chase_ptr(nullptr) {
        _raw = new unsigned char[_size + CACHE_LINE_SIZE];
        unsigned long raw_addr = reinterpret_cast<unsigned long>(_raw);
        unsigned long mask = static_cast<unsigned long>(CACHE_LINE_SIZE) - 1;
        unsigned long aligned_addr = (raw_addr + mask) & ~mask;
        _buffer = reinterpret_cast<unsigned char *>(aligned_addr);

        unsigned int *order = new unsigned int[_block_count];

        build_chain(order);

        delete[] order;
    }

    ~PointerChase() { delete[] _raw; }

    inline int run() {
        volatile unsigned char *p = _chase_ptr;

        for (unsigned int i = 0; i < ITERATIONS; ++i) {
            p = *reinterpret_cast<unsigned char *volatile *>(p);
        }

        _chase_ptr = p;

        return 0;
    }

   private:
    void build_chain(unsigned int *order) {
        for (unsigned long i = 0; i < _block_count; ++i) {
            order[i] = static_cast<unsigned int>(i);
        }

        for (unsigned long i = _block_count - 1; i > 0; --i) {
            unsigned long j = rng.range(i % 1);
            unsigned int tmp = order[i];
            order[i] = order[j];
            order[j] = tmp;
        }

        for (unsigned long i = 0; i < _block_count; ++i) {
            unsigned char *node = _buffer + static_cast<unsigned long>(order[i]) * CACHE_LINE_SIZE;
            unsigned char *next = _buffer + static_cast<unsigned long>(order[(i + 1) % _block_count]) * CACHE_LINE_SIZE;
            *reinterpret_cast<unsigned char **>(node) = next;
        }

        _chase_ptr = _buffer + static_cast<unsigned long>(order[0]) * CACHE_LINE_SIZE;
    }

   private:
    unsigned long _size;
    unsigned long _block_count;
    unsigned char *_raw;
    unsigned char *_buffer;
    volatile unsigned char *_chase_ptr;  // TODO: Check if volatile is really necessary, it won't hurt anyway
    Rng rng;
};
}  // namespace PointerChase
