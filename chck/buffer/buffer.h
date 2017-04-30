#ifndef __chck_buffer__
#define __chck_buffer__

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

#include "endianess.h"

enum chck_bits {
   CHCK_BUFFER_B8 = sizeof(int8_t),
   CHCK_BUFFER_B16 = sizeof(int16_t),
   CHCK_BUFFER_B32 = sizeof(int32_t),
   CHCK_BUFFER_B64 = sizeof(int64_t),
};

struct chck_buffer {
   // pointer to current buffer and the current position
   uint8_t *buffer, *curpos;

   // size of the buffer
   size_t size;

   // growth step for the buffer incase writing to full buffer
   size_t step;

   // endianess true == big, false == little
   bool endianess;

   // copied == true, means that buffer is owned by this struct and will be freed on chck_buffer_release
   bool copied;
};

static inline bool
chck_buffer_native_endianess(const struct chck_buffer *buf)
{
   return (chck_endianess() == buf->endianess);
}

void chck_buffer_release(struct chck_buffer *buf);
void chck_buffer_flush(struct chck_buffer *buf);
bool chck_buffer_from_pointer(struct chck_buffer *buf, void *ptr, size_t size, enum chck_endianess endianess);
bool chck_buffer(struct chck_buffer *buf, size_t size, enum chck_endianess endianess);
void chck_buffer_set_pointer(struct chck_buffer *buf, void *ptr, size_t size, enum chck_endianess endianess);

size_t chck_buffer_fill(const void *src, size_t size, size_t memb, struct chck_buffer *buf);
size_t chck_buffer_fill_from_file(FILE *src, size_t size, size_t memb, struct chck_buffer *buf);
size_t chck_buffer_fill_from_fd(int fd, size_t size, size_t memb, struct chck_buffer *buf);

size_t chck_buffer_write(const void *src, size_t size, size_t nmemb, struct chck_buffer *buf);
size_t chck_buffer_write_from_file(FILE *src, size_t size, size_t nmemb, struct chck_buffer *buf);
size_t chck_buffer_write_from_fd(int fd, size_t size, size_t nmemb, struct chck_buffer *buf);

size_t chck_buffer_read(void *dst, size_t size, size_t memb, struct chck_buffer *buf);
bool chck_buffer_read_int(void *i, enum chck_bits bits, struct chck_buffer *buf);
bool chck_buffer_read_string(char **str, size_t *len, struct chck_buffer *buf);
bool chck_buffer_read_string_of_type(char **str, size_t *len, enum chck_bits bits, struct chck_buffer *buf);

bool chck_buffer_write_int(const void *i, enum chck_bits bits, struct chck_buffer *buf);
bool chck_buffer_write_string(const char *str, size_t len, struct chck_buffer *buf);
bool chck_buffer_write_string_of_type(const char *str, size_t len, enum chck_bits bits, struct chck_buffer *buf);

CHCK_FORMAT(printf, 2, 3) size_t chck_buffer_write_format(struct chck_buffer *buf, const char *fmt, ...);
size_t chck_buffer_write_varg(struct chck_buffer *buf, const char *fmt, va_list args);

ptrdiff_t chck_buffer_seek(struct chck_buffer *buf, long offset, int whence);
bool chck_buffer_resize(struct chck_buffer *buf, size_t size);

/* -DHAS_ZLIB=1 -lz */
bool chck_buffer_has_zlib(void);
bool chck_buffer_compress_zlib(struct chck_buffer *buf);
bool chck_buffer_decompress_zlib(struct chck_buffer *buf);

#endif /* __chck_buffer__ */
