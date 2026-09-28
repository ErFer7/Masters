#pragma once

namespace Bandwidth {
class Bandwidth {
   public:
    static const unsigned int CACHE_LINE_SIZE = 64;
    static const unsigned int L1_CACHE_SIZE = 32 * 1024;
    static const unsigned int L2_CACHE_SIZE = 2 * 1024 * 1024;
    static const unsigned int STRIDE = (CACHE_LINE_SIZE / sizeof(unsigned long long));

   public:
    Bandwidth(unsigned long long size) : _size(size) {
        unsigned long long element_count = _size / sizeof(unsigned long long);

        _buffer = new unsigned long long[element_count];

        for (unsigned long long i = 0; i < element_count; i += STRIDE) {
            _buffer[i] = i;
        }
    }

    ~Bandwidth() { delete[] _buffer; }

    inline unsigned long long run() {
        unsigned long long element_count = _size / sizeof(unsigned long long);
        volatile unsigned long long acc = 0;

        for (unsigned long long i = 0; i < element_count; i += STRIDE) {
            _buffer[i]++;
            acc += _buffer[i];
        }

        return acc;
    }

   private:
    unsigned long long *_buffer;
    unsigned long long _size;
};
}  // namespace Bandwidth
