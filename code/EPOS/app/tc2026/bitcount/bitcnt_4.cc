/*

  This program is part of the TACLeBench benchmark suite.
  Version V 1.x

  Name: bitcnt_4.c

  Author: Bob Stout

  Function: Recursive bit counting functions using table lookup

  Source: http://www.snippets.org/

  Changes: no major functional changes

  License: May be used, modified, and re-distributed freely.

*/

#include "bitcount.h" /* from Snippets */

namespace BitCount {
/*
**  Count bits in each nybble
**
**  Note: Only the first 16 table entries are used, the rest could be
**        omitted.
*/

void BitCount::bitcount_init4(void) {
    int volatile i = 0;
    char bitcount_bits_tmp[256] = {
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2,
        3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3,
        3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5,
        6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4,
        3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4,
        5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6,
        6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

    for (i = 0; i < 256; i++) bitcount_bits[i] = bitcount_bits_tmp[i];
}

int BitCount::bitcount_ntbl_bitcnt(unsigned long x) {
    int cnt = bitcount_bits[(int)(x & 0x0000000FL)];

    if (0L != (x >>= 4)) cnt += bitcount_ntbl_bitcnt(x);

    return cnt;
}

/*
**  Count bits in each byte
*/

int BitCount::bitcount_btbl_bitcnt(unsigned long x) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    int idx = sizeof(x) - 1;
#else
    int idx = 0;
#endif
    int cnt = bitcount_bits[((char *)&x)[idx] & 0xFF];

    if (0L != (x >>= 8)) cnt += bitcount_btbl_bitcnt(x);

    return cnt;
}
}  // namespace BitCount
