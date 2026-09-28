#pragma once

#include "aes.h"
#include "rijndael_enc_libc.h"

namespace RijndaelEnc {
class RijndaelEnc {
   private:
    static const unsigned int DATA_SIZE = 31369;

   public:
    RijndaelEnc();

    ~RijndaelEnc() { delete[] rijndael_enc_data; }

    int run();

   private:
    void rijndael_enc_fillrand(unsigned char *buf, int len);
    void rijndael_enc_encfile(struct rijndael_enc_FILE *fin, struct aes *ctx);

   private:
    // These were static before
    unsigned int a[2], mt = 1, count = 4;
    char r[4];

    unsigned char rijndael_enc_key[32];
    int rijndael_enc_key_len;
    rijndael_enc_FILE rijndael_enc_fin;
    int rijndael_enc_checksum = 0;
    unsigned char *rijndael_enc_data;
    static const unsigned char rijndael_enc_input_data[];
};
}  // namespace RijndaelEnc
