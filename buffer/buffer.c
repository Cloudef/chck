#include "buffer.h"
#include <stdint.h> /* for standard integers */
#include <assert.h> /* for assert */
#include <string.h> /* for memcpy/memset */

#ifdef __APPLE__
#  include <malloc/malloc.h>
#  else
#  include <malloc.h>
#endif

#if defined(__GNUC__) || defined(__CYGWIN__)
#  include <byteswap.h>
#  define bswap16 __bswap_16
#  define bswap32 __bswap_32
#  define HAS_BYTESWAP 1
#elif defined(_MSC_VER)
#  define bswap16 _byteswap_ushort
#  define bswap32 _byteswap_ulong
#  define HAS_BYTESWAP 1
#else
#  define HAS_BYTESWAP 0
#endif

#if HAS_ZLIB
#  include <zlib.h>
#endif

enum { RETURN_OK = 1, RETURN_FAIL = 0 };

/* \brief awesome buffer struct */
typedef struct _chckBuffer {
   size_t size;
   void *buffer, *curpos;
   char endianess, freeBuffer;
} chckBuffer;

/* \brief is current machine big endian? */
int chckBufferIsBigEndian(void)
{
   union {
      uint32_t i;
      char c[4];
   } bint = {0x01020304};
   return bint.c[0] == 1;
}

/* \brief flip bytes for buffer of given size */
void chckBufferSwap(void *v, size_t size, size_t memb)
{
   size_t m;
   assert(v && size > 1);

   for (m = 0; m < memb; ++m) {
#if HAS_BYTESWAP
      if (size == sizeof(uint32_t)) *((uint32_t*)v+(m*size)) = bswap32(*((uint32_t*)v+(m*size)));
      if (size == sizeof(uint16_t)) *((uint16_t*)v+(m*size)) = bswap16(*((uint16_t*)v+(m*size)));
#else
      size_t s;
      unsigned char b[size];
      memcpy(b, v+(m*size), size);
      for (s = 0; s < size; ++s) memset(v+(m*size)+s, b[size - s - 1], 1);
#endif
   }
}

/* \brief free buffer */
void chckBufferFree(chckBuffer *buf)
{
   assert(buf);
   if (buf->buffer && buf->freeBuffer) free(buf->buffer);
   free(buf);
}

/* \brief create new buffer with pointer and size, pointer won't be copied nor freed! */
chckBuffer* chckBufferNewFromPointer(const void *ptr, size_t size, chckBufferEndianType endianess)
{
   chckBuffer *buf;

   if (!(buf = calloc(1, sizeof(chckBuffer))))
      return NULL;

   chckBufferSetPointer(buf, ptr, size, endianess);
   return buf;
}

/* \brief create new buffer, buffer with the size will be allocated */
chckBuffer* chckBufferNew(size_t size, chckBufferEndianType endianess)
{
   void *data;
   chckBuffer *buf;
   assert(size > 0);

   if (!(data = malloc(size)))
      goto fail;

   if (!(buf = chckBufferNewFromPointer(data, size, endianess)))
      goto fail;

   buf->freeBuffer = 1;
   return buf;

fail:
   if (data) free(data);
   return NULL;
}

/* \brief set new pointer to the buffer, pointer won't be freed or copied! */
void chckBufferSetPointer(chckBuffer *buf, const void *ptr, size_t size, chckBufferEndianType endianess)
{
   assert(buf);

   if (buf->buffer && buf->freeBuffer) free(buf->buffer);

   if (endianess == CHCK_BUFFER_ENDIAN_NATIVE) {
      buf->endianess = chckBufferIsBigEndian();
   } else {
      buf->endianess = endianess;
   }

   buf->size = size;
   buf->buffer = buf->curpos = (void*)ptr;
}

/* \brief get buffer pointer for low-level access */
void* chckBufferGetPointer(chckBuffer *buf) { return buf->buffer; }

/* \brief get offset pointer for low-level access */
void* chckBufferGetOffsetPointer(chckBuffer *buf) { return buf->curpos; }

/* \brief resize buffer. allocates new buffer, if the buffer wasn't created by chckBuffer */
int chckBufferResize(chckBuffer *buf, size_t size)
{
   void *tmp = NULL;
   assert(size != 0 && size != buf->size);

   if (buf->freeBuffer && size > buf->size)
      tmp = realloc(buf->buffer, size);

   if (!tmp) {
      if (!(tmp = malloc(size)))
         return RETURN_FAIL;

      /* memcpy much bytes as we can from old buffer's curpos */
      if (size < buf->size) {
         if (buf->buffer + (buf->size - size) < buf->curpos) {
            memcpy(tmp, buf->buffer + (buf->size - size), size);
         } else {
            memcpy(tmp, buf->curpos, size);
         }
      } else {
         memcpy(tmp, buf->buffer, size);
      }

      if (buf->freeBuffer) free(buf->buffer);
   }

   /* set new buffer position */
   if (buf->curpos - buf->buffer > (long)size) {
      buf->curpos = tmp + (buf->size - size);
   } else {
      buf->curpos = tmp + (buf->curpos - buf->buffer);
   }

   buf->size = size;
   buf->buffer = tmp;
   buf->freeBuffer = 1;
   return RETURN_OK;
}

/* \brief seek buffer */
size_t chckBufferSeek(chckBuffer *buf, long offset, int whence)
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

   return chckBufferGetOffset(buf);
}

/* \brief get current position in buffer */
size_t chckBufferGetOffset(chckBuffer *buf) { return buf->curpos - buf->buffer; }

/* \brief get current buffer size */
size_t chckBufferGetSize(chckBuffer *buf) { return buf->size; }

/* \brief is current buffer endianess same as our machine? */
int chckBufferIsNativeEndian(chckBuffer *buf) { return chckBufferIsBigEndian() == buf->endianess; }

/* \brief fill bytes to buffer (do not go forward) */
size_t chckBufferFill(const void *src, size_t size, size_t memb, chckBuffer *buf)
{
   assert(src && buf);

   if (size * memb > buf->size - (buf->curpos - buf->buffer))
      return 0;

   memcpy(buf->curpos, src, size * memb);
   return memb;
}

/* \brief fill bytes to buffer from file (do not go forward) */
size_t chckBufferFillFromFile(FILE *src, size_t size, size_t memb, chckBuffer *buf)
{
   assert(src && buf);

   if (size * memb > buf->size - (buf->curpos - buf->buffer))
      return 0;

   fread(buf->curpos, size, memb, src);
   return memb;
}

/* \brief read bytes from buffer */
size_t chckBufferRead(void *dst, size_t size, size_t memb, chckBuffer *buf)
{
   assert(dst && buf);

   if (size * memb > buf->size - (buf->curpos - buf->buffer))
      return 0;

   memcpy(dst, buf->curpos, size * memb);
   buf->curpos += size * memb;
   return memb;
}

/* \brief read 8 bit unsigned integer from buffer */
int chckBufferReadUInt8(chckBuffer *buf, unsigned char *i)
{
   assert(i);
   if (chckBufferRead(i, 1, 1, buf) != 1)
      return RETURN_FAIL;
   return RETURN_OK;
}

/* \brief read 8 bit signed integer from buffer */
int chckBufferReadInt8(chckBuffer *buf, char *i) { return chckBufferReadUInt8(buf, (unsigned char*)i); }

/* \brief read 16 bit unsigned integer from buffer */
int chckBufferReadUInt16(chckBuffer *buf, unsigned short *i)
{
   uint16_t r;
   assert(i);

   if (chckBufferRead(&r, sizeof(r), 1, buf) != 1)
      return RETURN_FAIL;

   if (!chckBufferIsNativeEndian(buf)) chckBufferSwap(&r, sizeof(r), 1);
   *i = r;
   return RETURN_OK;
}

/* \brief read 16 bit signed integer from buffer */
int chckBufferReadInt16(chckBuffer *buf, short *i) { return chckBufferReadUInt16(buf, (unsigned short*)i); }

/* \brief read 32 bit unsigned integer from buffer */
int chckBufferReadUInt32(chckBuffer *buf, unsigned int *i)
{
   uint32_t r;
   assert(i);

   if (chckBufferRead(&r, sizeof(r), 1, buf) != 1)
      return RETURN_FAIL;

   if (!chckBufferIsNativeEndian(buf)) chckBufferSwap(&r, sizeof(r), 1);
   *i = r;
   return RETURN_OK;
}

/* \brief read 32 bit signed integer from buffer */
int chckBufferReadInt32(chckBuffer *buf, int *i) { return chckBufferReadUInt32(buf, (unsigned int*)i); }

/* \brief read string from buffer */
int chckBufferReadString(chckBuffer *buf, size_t bytes, char **str)
{
   union {
      uint8_t l8;
      uint16_t l16;
      uint32_t l32;
   } u;
   size_t len;
   assert(buf && str && bytes > 0 && bytes <= sizeof(uint32_t));
   *str = NULL;

   if (bytes == sizeof(uint32_t)) {
      if (chckBufferReadUInt32(buf, &u.l32) != RETURN_OK)
         return RETURN_FAIL;
      len = u.l32;
   } else if (bytes == sizeof(uint16_t)) {
      if (chckBufferReadUInt16(buf, &u.l16) != RETURN_OK)
         return RETURN_FAIL;
      len = u.l16;
   } else {
      if (chckBufferReadUInt8(buf, &u.l8) != RETURN_OK)
         return RETURN_FAIL;
      len = u.l8;
   }

   if (len) {
      if (!(*str = calloc(1, len+1)))
         return RETURN_FAIL;
      chckBufferRead(*str, 1, len, buf);
   }
   return RETURN_OK;
}

/* \brief write bytes to buffer */
size_t chckBufferWrite(const void *src, size_t size, size_t memb, chckBuffer *buf)
{
   memb = chckBufferFill(src, size, memb, buf);
   buf->curpos += size * memb;
   return memb;
}

/* \brief write bytes to buffer from file */
size_t chckBufferWriteFromFile(FILE *src, size_t size, size_t memb, chckBuffer *buf)
{
   memb = chckBufferFillFromFile(src, size, memb, buf);
   buf->curpos += size * memb;
   return memb;
}

/* \brief write 8 bit unsigned integer to buffer */
int chckBufferWriteUInt8(chckBuffer *buf, unsigned char i)
{
   assert(i);
   if (chckBufferWrite(&i, 1, 1, buf) != 1)
      return RETURN_FAIL;
   return RETURN_OK;
}

/* \brief write 8 bit signed integer to buffer */
int chckBufferWriteInt8(chckBuffer *buf, char i) { return chckBufferWriteUInt8(buf, (unsigned char)i); }

/* \brief write 16 bit unsigned integer to buffer */
int chckBufferWriteUInt16(chckBuffer *buf, unsigned short i)
{
   uint16_t r = i;
   assert(i);

   if (!chckBufferIsNativeEndian(buf)) chckBufferSwap(&r, sizeof(r), 1);

   if (chckBufferWrite(&r, sizeof(r), 1, buf) != 1)
      return RETURN_FAIL;

   return RETURN_OK;
}

/* \brief write 16 bit signed integer to buffer */
int chckBufferWriteInt16(chckBuffer *buf, short i) { return chckBufferWriteUInt16(buf, (unsigned short)i); }

/* \brief write 32 bit unsigned integer to buffer */
int chckBufferWriteUInt32(chckBuffer *buf, unsigned int i)
{
   uint32_t r = i;
   assert(i);

   if (!chckBufferIsNativeEndian(buf)) chckBufferSwap(&r, sizeof(r), 1);

   if (chckBufferWrite(&r, sizeof(r), 1, buf) != 1)
      return RETURN_FAIL;

   return RETURN_OK;
}

/* \brief write 32 bit signed integer to buffer */
int chckBufferWriteInt32(chckBuffer *buf, int i) { return chckBufferWriteUInt32(buf, (unsigned int)i); }

/* \brief write string to buffer */
int chckBufferWriteString(chckBuffer *buf, size_t len, const char *str)
{
   assert(buf);

   if (len > 0xffff) {
      if (chckBufferWriteUInt32(buf, len) != RETURN_OK)
         return RETURN_FAIL;
   } else if (len > 0xff) {
      if (chckBufferWriteUInt16(buf, len) != RETURN_OK)
         return RETURN_FAIL;
   } else {
      if (chckBufferWriteUInt8(buf, len) != RETURN_OK)
         return RETURN_FAIL;
   }

   if (len) chckBufferWrite(str, 1, len, buf);
   return RETURN_OK;
}

/* \brief compress buffer using zlib */
int chckBufferCompressZlib(chckBuffer *buf)
{
#if HAS_ZLIB
   int ret;
   void *compressed = NULL, *tmp;
   size_t destSize, bufSize;

   destSize = bufSize = compressBound(buf->size);
   if (!(compressed = malloc(destSize)))
      goto fail;

   while ((ret = compress(compressed, &destSize, buf->buffer, buf->size)) == Z_BUF_ERROR) {
      if (!(tmp = realloc(compressed, bufSize * 2))) {
         if (!(tmp = malloc(bufSize * 2))) goto fail;
         free(compressed);
      }
      compressed = tmp;
      destSize = bufSize *= 2;
   }
   if (ret != Z_OK) goto fail;

   if (buf->buffer && buf->freeBuffer) free(buf->buffer);
   buf->size = bufSize;
   buf->buffer = buf->curpos = compressed;
   buf->freeBuffer = 1;

   if (bufSize != destSize) chckBufferResize(buf, destSize);
   return RETURN_OK;

fail:
   if (compressed) free(compressed);
   return RETURN_FAIL;
#else
   (void)buf;
   assert(0 && "not compiled with zlib support");
   return RETURN_FAIL;
#endif
}

/* \brief decompress buffer using zlib */
int chckBufferDecompressZlib(chckBuffer *buf)
{
#if HAS_ZLIB
   int ret;
   void *decompressed = NULL, *tmp;
   size_t destSize, bufSize;

   destSize = bufSize = buf->size * 2;
   if (!(decompressed = malloc(destSize)))
      goto fail;

   while ((ret = uncompress(decompressed, &destSize, buf->buffer, buf->size)) == Z_BUF_ERROR) {
      if (!(tmp = realloc(decompressed, bufSize * 2))) {
         if (!(tmp = malloc(bufSize * 2))) goto fail;
         free(decompressed);
      }
      decompressed = tmp;
      destSize = bufSize *= 2;
   }
   if (ret != Z_OK) goto fail;

   if (buf->buffer && buf->freeBuffer) free(buf->buffer);
   buf->size = bufSize;
   buf->buffer = buf->curpos = decompressed;
   buf->freeBuffer = 1;

   if (bufSize != destSize) chckBufferResize(buf, destSize);
   return RETURN_OK;

fail:
   if (decompressed) free(decompressed);
   return RETURN_FAIL;
#else
   (void)buf;
   assert(0 && "not compiled with zlib support");
   return RETURN_FAIL;
#endif
}

/* vim: set ts=8 sw=3 tw=0 :*/
