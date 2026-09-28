#ifndef RIJNDAEL_ENC_LIBC_H
#define RIJNDAEL_ENC_LIBC_H

namespace RijndaelEnc {
int rijndael_enc_toupper(int c);

enum RijndaelOrigin { RIJNDAEL_ENC_SEEK_SET, RIJNDAEL_ENC_SEEK_CUR, RIJNDAEL_ENC_SEEK_END };
struct rijndael_enc_FILE {
    unsigned char *data;
    unsigned int size;
    unsigned cur_pos;
};

unsigned int rijndael_enc_fread(void *ptr, unsigned int size, unsigned int count, struct rijndael_enc_FILE *stream);
unsigned int rijndael_enc_fwrite(const void *ptr,
                                 unsigned int size,
                                 unsigned int count,
                                 struct rijndael_enc_FILE *stream);
int rijndael_enc_fseek(struct rijndael_enc_FILE *stream, int offset, RijndaelOrigin origin);
int rijndael_enc_fgetpos(struct rijndael_enc_FILE *stream, unsigned *position);
int rijndael_enc_feof(struct rijndael_enc_FILE *stream);
}  // namespace RijndaelEnc
#endif  // RIJNDAEL_ENC_LIBC_H
