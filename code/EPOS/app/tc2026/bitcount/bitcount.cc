/*

  This program is part of the TACLeBench benchmark suite.
  Version V 1.x

  Name: bitcount.c

  Author: Bob Stout & Auke Reitsma

  Function: test program for bit counting functions

  Source: www.snippest.com

  Changes: no major functional changes

  License: May be used, modified, and re-distributed freely.

*/

#include "bitcount.h"

namespace BitCount {

#define FUNCS 8

/*
   First declaration of the functions
*/

int BitCount::bitcount_bit_shifter(long int x) {
    int n;
    unsigned int i;

    for (i = n = 0; x && (i < (sizeof(long) * 8)); ++i, x >>= 1) n += (int)(x & 1L);
    return n;
}

BitCount::BitCount() {
    bitcount_iterations = 10;

    bitcount_init3();
    bitcount_init4();
}

unsigned long BitCount::bitcount_random(void) {
    long x, hi, lo, t;

    /*
       Compute x[n + 1] = (7^5 * x[n]) mod (2^31 - 1).
       From "Random number generators: good ones are hard to find",
       Park and Miller, Communications of the ACM, vol. 31, no. 10,
       October 1988, p. 1195.
    */
    x = bitcount_randseed;
    hi = x / 127773;
    lo = x % 127773;
    t = 16807 * lo - 2836 * hi;
    if (t <= 0) t += 0x7fffffff;
    bitcount_randseed = t;
    return (t);
}

int BitCount::run() {
    reset();

    unsigned int i, j;
    for (i = 0; i < FUNCS; i++) {
        for (j = 0, bitcount_seed = bitcount_random(); j < bitcount_iterations; j++, bitcount_seed += 13) {
            // The original calls were done by function pointers
            switch (i) {
                case 0:
                    bitcount_res = bitcount_bit_count(bitcount_seed);
                    break;
                case 1:
                    bitcount_res = bitcount_bitcount(bitcount_seed);
                    break;
                case 2: {
                    bitcount_res = bitcount_ntbl_bitcnt(bitcount_seed);
                    break;
                }
                case 3: {
                    bitcount_res = bitcount_btbl_bitcnt(bitcount_seed);
                    break;
                }
                case 4:
                    bitcount_res = bitcount_ntbl_bitcount(bitcount_seed);
                    break;
                case 5:
                    bitcount_res = bitcount_BW_btbl_bitcount(bitcount_seed);
                    break;
                case 6:
                    bitcount_res = bitcount_AR_btbl_bitcount(bitcount_seed);
                    break;
                case 7:
                    bitcount_res = bitcount_bit_shifter(bitcount_seed);
                    break;
                default:
                    break;
            }
            bitcount_n += bitcount_res;
        }
    }

    return bitcount_n + (-1095) != 0;
}
}  // namespace BitCount
