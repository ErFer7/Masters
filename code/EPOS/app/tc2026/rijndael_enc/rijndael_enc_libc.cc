#include "rijndael_enc_libc.h"

namespace RijndaelEnc {
int rijndael_enc_toupper(int c) {
    if ((c >= 'a') && (c <= 'z')) return c - 'a' + 'A';
    return c;
}

unsigned int rijndael_enc_fread(void *ptr, unsigned int size, unsigned int count, struct rijndael_enc_FILE *stream) {
    unsigned i = stream->cur_pos, i2 = 0;
    unsigned int number_of_chars_to_read =
        stream->size - stream->cur_pos >= size * count ? size * count : stream->size - stream->cur_pos;
    while (i < stream->cur_pos + number_of_chars_to_read) ((unsigned char *)ptr)[i2++] = stream->data[i++];
    stream->cur_pos += number_of_chars_to_read;
    return number_of_chars_to_read;
}

unsigned int rijndael_enc_fwrite(const void *ptr,
                                 unsigned int size,
                                 unsigned int count,
                                 struct rijndael_enc_FILE *stream) {
    unsigned i = stream->cur_pos, i2 = 0;
    unsigned int number_of_chars_to_write =
        stream->size - stream->cur_pos >= size * count ? size * count : stream->size - stream->cur_pos;
    while (i < stream->cur_pos + number_of_chars_to_write) stream->data[i++] = ((unsigned char *)ptr)[i2++];
    stream->cur_pos += number_of_chars_to_write;
    return number_of_chars_to_write;
}

int rijndael_enc_fseek(struct rijndael_enc_FILE *stream, int offset, RijndaelOrigin origin) {
    if (origin == RIJNDAEL_ENC_SEEK_SET) {
        stream->cur_pos = offset;
        return 0;
    } else if (origin == RIJNDAEL_ENC_SEEK_CUR) {
        stream->cur_pos += offset;
        return 0;
    } else if (origin == RIJNDAEL_ENC_SEEK_END) {
        stream->cur_pos = stream->size + offset;
        return 0;
    }
    return -1;
}

int rijndael_enc_fgetpos(struct rijndael_enc_FILE *stream, unsigned *position) {
    *position = stream->cur_pos;
    return 0;
}

int rijndael_enc_feof(struct rijndael_enc_FILE *stream) { return stream->cur_pos == stream->size ? 1 : 0; }
}  // namespace RijndaelEnc
