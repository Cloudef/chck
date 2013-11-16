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
      assert(chckBufferGetOffset(buf) - chckBufferGetSize(buf) == 0);
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
      assert(chckBufferGetOffset(buf) - chckBufferGetSize(buf) == 0);
      chckBufferFree(buf);
   }

   /* TEST: native buffer write && read */
   {
      i = 8; s = 6;
      chckBuffer *buf = chckBufferNew(5+sizeof(int32_t)+sizeof(int16_t), CHCK_BUFFER_ENDIAN_NATIVE);
      chckBufferWriteString(buf, 4, "test");
      chckBufferWriteUInt32(buf, i);
      chckBufferWriteUInt16(buf, s);
      assert(chckBufferGetOffset(buf) - chckBufferGetSize(buf) == 0);
      chckBufferSeek(buf, 0, SEEK_SET);

      chckBufferReadString(buf, 1, &str);
      assert(!strcmp(str, "test"));
      chckBufferReadInt32(buf, &i);
      assert(i == 8);
      chckBufferReadInt16(buf, &s);
      assert(s == 6);
      assert(chckBufferGetOffset(buf) - chckBufferGetSize(buf) == 0);
      chckBufferFree(buf);
   }

   /* TEST: non-native buffer write && read */
   {
      i = 8; s = 6;
      chckBufferEndianType endianess = CHCK_BUFFER_ENDIAN_BIG;
      if (chckBufferIsBigEndian()) endianess = CHCK_BUFFER_ENDIAN_LITTLE;
      chckBuffer *buf = chckBufferNew(5+sizeof(int32_t)+sizeof(int16_t), endianess);
      chckBufferWriteString(buf, 4, "test");
      chckBufferWriteUInt32(buf, i);
      chckBufferWriteUInt16(buf, s);
      assert(chckBufferGetOffset(buf) - chckBufferGetSize(buf) == 0);
      chckBufferSeek(buf, 0, SEEK_SET);

      chckBufferReadString(buf, 1, &str);
      assert(!strcmp(str, "test"));
      chckBufferReadInt32(buf, &i);
      assert(i == 8);
      chckBufferReadInt16(buf, &s);
      assert(s == 6);
      assert(chckBufferGetOffset(buf) - chckBufferGetSize(buf) == 0);
      chckBufferFree(buf);
   }

   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
