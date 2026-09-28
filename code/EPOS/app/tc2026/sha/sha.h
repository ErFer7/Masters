/*

  This program is part of the TACLeBench benchmark suite.
  Version V 1.x

  Name: sha.h

  Author: Peter C. Gutmann's (heavily modified by Uwe Hollerbach)

  NIST Secure Hash Algorithm

  Source: Peter C. Gutmann's implementation as found in Applied Cryptography by Bruce Schneier

  Changes: no major functional changes

  License: May be used, modified, and re-distributed freely.
*/

#ifndef SHA_H
#define SHA_H

/* Useful defines & typedefs */

namespace Sha {

typedef unsigned char BYTE;
typedef unsigned long LONG;

/* Type to use for unaligned operations. */
#define SHA_BLOCKSIZE 64
#define SHA_DIGESTSIZE 20
#define LITTLE_ENDIAN
#define NULL ((void *)0)

struct SHA_MY_FILE {
    const unsigned char *data;
    size_t size;
    unsigned cur_pos;
};

struct SHA_INFO {
    LONG digest[5];          /* message digest */
    LONG count_lo, count_hi; /* 64-bit bit count */
    LONG data[16];           /* SHA data buffer */
};

class Sha {
   public:
    Sha();

    ~Sha() = default;

    int run();

   private:
    inline void reset() {
        sha_info.digest[0] = 0x67452301L;
        sha_info.digest[1] = 0xefcdab89L;
        sha_info.digest[2] = 0x98badcfeL;
        sha_info.digest[3] = 0x10325476L;
        sha_info.digest[4] = 0xc3d2e1f0L;
        sha_info.count_lo = 0L;
        sha_info.count_hi = 0L;
        for (int i = 0; i < 16; i++) sha_info.data[i] = 0;
    }

    void sha_transform(struct SHA_INFO *);
    void sha_byte_reverse(LONG *buffer, int count);
    void sha_init(void);
    size_t sha_fread(void *, size_t, size_t, struct SHA_MY_FILE *);
    void sha_update(struct SHA_INFO *, BYTE *, int);
    void sha_final(struct SHA_INFO *);
    void sha_stream(struct SHA_INFO *, struct SHA_MY_FILE *);

   private:
    SHA_INFO sha_info;
    static const unsigned char sha_data[32743];
};
}  // namespace Sha

#endif  // SHA_H
