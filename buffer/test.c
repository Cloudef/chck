#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

int main(void)
{
   int i;
   short s;
   char c;
   char *str;

   /* TEST: little endian buffer */
   {
      static const char test[] = "\x12this integer is 5:\x5\x0\x0\0\1\0\1";
      chckBuffer *buf = chckBufferNewFromPointer(test, sizeof(test)-1, CHCK_BUFFER_ENDIAN_LITTLE);
      chckBufferReadString(buf, 1, &str);
      assert(!strcmp(str, "this integer is 5:"));
      chckBufferReadInt32(buf, &i);
      assert(i == 5);
      chckBufferReadInt16(buf, &s);
      assert(s == 1);
      chckBufferReadInt8(buf, &c);
      assert(c == 1);
      assert(buf->size - (buf->curpos - buf->buffer) == 0);
      chckBufferFree(buf);
   }

   /* TEST: big endian buffer */
   {
      static const char test[] = "\x12this integer is 5:\x0\x0\0\x5\0\1\1";
      chckBuffer *buf = chckBufferNewFromPointer(test, sizeof(test)-1, CHCK_BUFFER_ENDIAN_BIG);
      chckBufferReadString(buf, 1, &str);
      assert(!strcmp(str, "this integer is 5:"));
      chckBufferReadInt32(buf, &i);
      assert(i == 5);
      chckBufferReadInt16(buf, &s);
      assert(s == 1);
      chckBufferReadInt8(buf, &c);
      assert(c == 1);
      assert(buf->size - (buf->curpos - buf->buffer) == 0);
      chckBufferFree(buf);
   }

   /* TEST: native buffer */
   {
      i = 8; s = 6;
      chckBuffer *buf = chckBufferNew(5+sizeof(int32_t)+sizeof(int16_t), CHCK_BUFFER_ENDIAN_NATIVE);
      memcpy(buf->buffer, "\x4test", 5);
      memcpy(buf->buffer+5, &i, sizeof(int32_t));
      memcpy(buf->buffer+5+sizeof(int32_t), &s, sizeof(int16_t));
      chckBufferReadString(buf, 1, &str);
      assert(!strcmp(str, "test"));
      chckBufferReadInt32(buf, &i);
      assert(i == 8);
      chckBufferReadInt16(buf, &s);
      assert(s == 6);
      assert(buf->size - (buf->curpos - buf->buffer) == 0);
      chckBufferFree(buf);
   }

   return EXIT_SUCCESS;
}
