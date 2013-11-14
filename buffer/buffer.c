#include "buffer.h"
#include <stdint.h> /* for standard integers */
#include <assert.h> /* for assert */
#include <string.h> /* for memcpy/memset */

#ifdef __APPLE__
#  include <malloc/malloc.h>
#  else
#  include <malloc.h>
#endif

enum { RETURN_OK = 1, RETURN_FAIL = 0 };

/* \brief is current machine big endian? */
static int chckIsBigEndian(void)
{
   union {
      uint32_t i;
      char c[4];
   } bint = {0x01020304};
   return bint.c[0] == 1;
}

/* \brief flip bits for buffer of given size */
void chckFlipEndian(void *v, size_t size, size_t memb)
{
   size_t s, m;
   unsigned char b[size];
   assert(v);
   for (m = 0; m < memb; ++m) {
      memcpy(b, v+(m*size), size);
      for (s = 0; s < size; ++s) memset(v+(m*size)+s, b[size - s - 1], 1);
   }
}

/* \brief free buffer */
void chckBufferFree(chckBuffer *buf)
{
   assert(buf);
   if (buf->buffer && buf->freeBuffer) free(buf->buffer);
   free(buf);
}

/* \brief create new buffer with pointer and size, buffer won't be copied nor freed! */
chckBuffer* chckBufferNewFromPointer(const void *ptr, size_t size, chckBufferEndianType endianess)
{
   chckBuffer *buf;

   if (!(buf = calloc(1, sizeof(chckBuffer))))
      return NULL;

   if (endianess == CHCK_BUFFER_ENDIAN_NATIVE) {
      buf->endianess = chckIsBigEndian();
   } else {
      buf ->endianess = endianess;
   }

   buf->size = size;
   buf->buffer = buf->curpos = (void*)ptr;
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

/* \brief is current buffer endianess same as our machine? */
int chckBufferIsNativeEndian(chckBuffer *buf) { return chckIsBigEndian() == buf->endianess; }

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
   uint8_t r;
   assert(i);

   if (chckBufferRead(&r, sizeof(r), 1, buf) != 1)
      return RETURN_FAIL;

   *i = r;
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

   if (!chckBufferIsNativeEndian(buf)) chckFlipEndian(&r, sizeof(r), 1);
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

   if (!chckBufferIsNativeEndian(buf)) chckFlipEndian(&r, sizeof(r), 1);
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
      if (chckBufferRead(&u.l8, bytes, 1, buf) != 1)
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

/* vim: set ts=8 sw=3 tw=0 :*/
