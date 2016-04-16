#include "buffer.h"
#include <chck/overflow/overflow.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#if HAS_ZLIB
#  include <zlib.h>
#elif !defined(HAS_ZLIB)
#  define HAS_ZLIB 0
#endif

static inline bool
valid_bits(enum chck_bits bits)
{
   return (bits == CHCK_BUFFER_B8  ||
           bits == CHCK_BUFFER_B16 ||
           bits == CHCK_BUFFER_B32 ||
           bits == CHCK_BUFFER_B64);
}

void
chck_buffer_flush(struct chck_buffer *buf)
{
   assert(buf);

   if (buf->copied)
      free(buf->buffer);

   buf->copied = false;
   buf->curpos = buf->buffer = NULL;
}

void
chck_buffer_release(struct chck_buffer *buf)
{
   if (!buf)
      return;

   chck_buffer_flush(buf);
   *buf = (struct chck_buffer){0};
}

bool
chck_buffer_from_pointer(struct chck_buffer *buf, void *ptr, size_t size, enum chck_endianess endianess)
{
   assert(buf);
   *buf = (struct chck_buffer){ .step = 32 };
   chck_buffer_set_pointer(buf, ptr, size, endianess);
   return true;
}

bool
chck_buffer(struct chck_buffer *buf, size_t size, enum chck_endianess endianess)
{
   assert(buf);

   void *data = NULL;
   if (size > 0 && !(data = malloc(size)))
      return false;

   if (unlikely(!chck_buffer_from_pointer(buf, data, size, endianess)))
      goto fail;

   buf->copied = true;
   return true;

fail:
   free(data);
   return false;
}

void
chck_buffer_set_pointer(struct chck_buffer *buf, void *ptr, size_t size, enum chck_endianess endianess)
{
   assert(buf);

   if (buf->copied) {
      free(buf->buffer);
      buf->buffer = NULL;
   }

   if (endianess == CHCK_ENDIANESS_NATIVE) {
      buf->endianess = chck_endianess();
   } else {
      buf->endianess = endianess;
   }

   buf->size = size;
   buf->buffer = buf->curpos = ptr;
   buf->copied = false;
}

bool
chck_buffer_resize(struct chck_buffer *buf, size_t size)
{
   assert(buf);

   if (unlikely(size == buf->size))
      return true;

   if (unlikely(size == 0)) {
      chck_buffer_flush(buf);
      return true;
   }

   uint8_t *tmp;
   if (!(tmp = realloc((buf->copied ? buf->buffer : NULL), size)))
      return false;

   /* set new buffer position */
   if (buf->curpos - buf->buffer > (ptrdiff_t)size) {
      buf->curpos = tmp + (buf->size - size);
   } else {
      buf->curpos = tmp + (buf->curpos - buf->buffer);
   }

   buf->size = size;
   buf->buffer = tmp;
   buf->copied = true;
   return true;
}

ptrdiff_t
chck_buffer_seek(struct chck_buffer *buf, long offset, int whence)
{
   assert(buf);
   assert(whence == SEEK_SET || whence == SEEK_END || whence == SEEK_CUR);

   switch (whence) {
      case SEEK_SET:
         if (buf->buffer + offset > buf->buffer + buf->size) {
            buf->curpos = buf->buffer + buf->size;
         } else if (offset >= 0) {
            buf->curpos = buf->buffer + offset;
         }
      break;
      case SEEK_CUR:
         if (buf->curpos + offset > buf->buffer + buf->size) {
            buf->curpos = buf->curpos + buf->size;
         } else if (buf->curpos + offset < buf->buffer) {
            buf->curpos = buf->buffer;
         } else {
            buf->curpos = buf->curpos + offset;
         }
      break;
      case SEEK_END:
         buf->curpos = buf->buffer + buf->size;
      break;
      default:break;
   }

   return buf->curpos - buf->buffer;
}

static bool
bounds_check(struct chck_buffer *buf, size_t size, size_t memb)
{
   size_t nsz;
   if (unlikely(chck_mul_ofsz(size, memb, &nsz)))
      return false;

   const size_t sz = buf->size - (buf->curpos - buf->buffer);
   if (nsz > sz) {
      /* buf->size + size * memb + buf->step */
      if (unlikely(chck_add_ofsz(buf->size, nsz, &nsz)) || unlikely(chck_add_ofsz(buf->step, nsz, &nsz)))
         return false;

      if (!chck_buffer_resize(buf, nsz))
         return false;
   }

   return true;
}

size_t
chck_buffer_fill(const void *src, size_t size, size_t memb, struct chck_buffer *buf)
{
   assert(src && buf);

   if (!bounds_check(buf, size, memb) || !buf->curpos || !src)
      return 0;

   memcpy(buf->curpos, src, size * memb);
   return memb;
}

size_t
chck_buffer_fill_from_file(FILE *src, size_t size, size_t memb, struct chck_buffer *buf)
{
   assert(src && buf);

   if (!bounds_check(buf, size, memb) || !buf->curpos || !src)
      return 0;

   return fread(buf->curpos, size, memb, src);
}

size_t
chck_buffer_fill_from_fd(int fd, size_t size, size_t memb, struct chck_buffer *buf)
{
   assert(fd && buf);

   if (!bounds_check(buf, size, memb) || !buf->curpos)
      return 0;

   const ssize_t ret = read(fd, buf->curpos, size * memb);
   if (unlikely(ret == -1 || ret == 0))
      return 0;

   return (size_t)ret / size;
}

size_t
chck_buffer_read(void *dst, size_t size, size_t memb, struct chck_buffer *buf)
{
   assert(dst && buf);

   size_t nsz;
   if (unlikely(chck_mul_ofsz(size, memb, &nsz)))
      return 0;

   if (unlikely(nsz > buf->size - (buf->curpos - buf->buffer))) {
      assert(size != 0); // should never happen
      // read as much as we can
      memb = (buf->size - (buf->curpos - buf->buffer)) / size;
   }

   memcpy(dst, buf->curpos, size * memb);
   buf->curpos += size * memb;
   return memb;
}

bool
chck_buffer_read_int(void *i, enum chck_bits bits, struct chck_buffer *buf)
{
   assert(i && buf);

   if (!valid_bits(bits))
      return false;

   if (unlikely(chck_buffer_read(i, bits, 1, buf) != 1))
      return false;

   if (!chck_buffer_native_endianess(buf))
      chck_bswap_single(i, bits);

   return true;
}

bool
chck_buffer_read_string_of_type(char **str, size_t *out_len, enum chck_bits bits, struct chck_buffer *buf)
{
   assert(buf && str);
   *str = NULL;

   if (out_len)
      *out_len = 0;

   size_t len = 0;
   if (unlikely(!chck_buffer_read_int(&len, bits, buf)))
      return false;

   if (out_len)
      *out_len = len;

   if (len <= 0)
      return true;

   if (!(*str = chck_calloc_add_of(len, 1)))
      return false;

   if (unlikely(chck_buffer_read(*str, 1, len, buf) != len)) {
      free(*str);
      return false;
   }

   return true;
}

bool
chck_buffer_read_string(char **str, size_t *len, struct chck_buffer *buf)
{
   assert(str && buf);
   *str = NULL;

   if (len)
      *len = 0;

   uint8_t bits;
   if (unlikely(!chck_buffer_read_int(&bits, sizeof(uint8_t), buf)))
      return false;

   return likely(chck_buffer_read_string_of_type(str, len, bits, buf));
}

size_t
chck_buffer_write(const void *src, size_t size, size_t memb, struct chck_buffer *buf)
{
   memb = chck_buffer_fill(src, size, memb, buf);
   buf->curpos += size * memb;
   return memb;
}

size_t
chck_buffer_write_from_file(FILE *src, size_t size, size_t memb, struct chck_buffer *buf)
{
   memb = chck_buffer_fill_from_file(src, size, memb, buf);
   buf->curpos += size * memb;
   return memb;
}

size_t
chck_buffer_write_from_fd(int fd, size_t size, size_t memb, struct chck_buffer *buf)
{
   memb = chck_buffer_fill_from_fd(fd, size, memb, buf);
   buf->curpos += size * memb;
   return memb;
}

bool
chck_buffer_write_int(const void *i, enum chck_bits bits, struct chck_buffer *buf)
{
   assert(buf);

   if (!valid_bits(bits))
      return false;

   bool ret;
   if (!chck_buffer_native_endianess(buf)) {
      uint8_t b[sizeof(uint64_t)];
      memcpy(b, i, bits);
      chck_bswap_single(b, bits);
      ret = (chck_buffer_write(b, bits, 1, buf) == 1);
   } else {
      ret = (chck_buffer_write(i, bits, 1, buf) == 1);
   }

   return ret;
}

bool
chck_buffer_write_string_of_type(const char *str, size_t len, enum chck_bits bits, struct chck_buffer *buf)
{
   assert(buf);

   if (unlikely(!chck_buffer_write_int(&len, bits, buf)))
      return false;

   if (len <= 0)
      return true;

   return likely(chck_buffer_write(str, 1, len, buf) == len);
}

bool
chck_buffer_write_string(const char *str, size_t len, struct chck_buffer *buf)
{
   assert(buf);
   const enum chck_bits bits = (len > 0xffff ? sizeof(uint32_t) : (len > 0xff ? sizeof(uint16_t) : sizeof(uint8_t)));

   if (unlikely(!chck_buffer_write_int(&bits, sizeof(uint8_t), buf)))
      return false;

   return likely(chck_buffer_write_string_of_type(str, len, bits, buf));
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"

size_t
chck_buffer_write_format(struct chck_buffer *buf, const char *fmt, ...)
{
   va_list argp;
   va_start(argp, fmt);
   const size_t wrote = chck_buffer_write_varg(buf, fmt, argp);
   va_end(argp);
   return wrote;
}

size_t
chck_buffer_write_varg(struct chck_buffer *buf, const char *fmt, va_list args)
{
   va_list cpy;
   va_copy(cpy, args);

   char *str = NULL;
   const size_t len = vsnprintf(NULL, 0, fmt, args);
   if (len > 0 && !(str = chck_malloc_add_of(len, 1))) {
      va_end(cpy);
      return false;
   }

   vsnprintf(str, len + 1, fmt, cpy);
   va_end(cpy);

   const size_t wrote = chck_buffer_write(str, 1, len, buf);
   free(str);
   return wrote;
}

#pragma GCC diagnostic pop

CHCK_CONST bool
chck_buffer_has_zlib(void)
{
   return HAS_ZLIB;
}

#pragma GCC diagnostic ignored "-Wsuggest-attribute=const"

bool
chck_buffer_compress_zlib(struct chck_buffer *buf)
{
#if HAS_ZLIB
   uLongf dsize, bsize;
   dsize = bsize = compressBound(buf->size);

   void *compressed;
   if (!(compressed = malloc(dsize)))
      return false;

   int ret;
   while ((ret = compress(compressed, &dsize, buf->buffer, buf->size)) == Z_BUF_ERROR) {
      void *tmp;
      if (!(tmp = chck_realloc_mul_of(compressed, bsize, 2)))
         goto fail;

      compressed = tmp;
      dsize = (bsize *= 2);
   }

   if (unlikely(ret != Z_OK))
      goto fail;

   chck_buffer_set_pointer(buf, compressed, bsize, buf->endianess);
   buf->copied = true;

   if (buf->size > dsize)
      chck_buffer_resize(buf, dsize);

   return true;

fail:
   free(compressed);
   return false;
#else
   (void)buf;
   return false;
#endif
}

bool
chck_buffer_decompress_zlib(struct chck_buffer *buf)
{
#if HAS_ZLIB
   uLongf dsize, bsize;

   {
      size_t sz;
      if (unlikely(chck_mul_ofsz(buf->size, 2, &sz)) || (uLongf)sz < sz)
         return false;

      dsize = bsize = sz;
   }

   if (!dsize)
      return false;

   void *decompressed;
   if (!(decompressed = malloc(dsize)))
      return false;

   int ret;
   while ((ret = uncompress(decompressed, &dsize, buf->buffer, buf->size)) == Z_BUF_ERROR) {
      void *tmp;
      if (!(tmp = chck_realloc_mul_of(decompressed, bsize, 2)))
         goto fail;

      decompressed = tmp;
      dsize = (bsize *= 2);
   }

   if (unlikely(ret != Z_OK))
      goto fail;

   chck_buffer_set_pointer(buf, decompressed, bsize, buf->endianess);
   buf->copied = true;

   if (bsize > dsize)
      chck_buffer_resize(buf, dsize);

   return true;

fail:
   free(decompressed);
   return false;
#else
   (void)buf;
   return false;
#endif
}
