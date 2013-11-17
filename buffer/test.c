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
   char c, *str;

   chckBufferEndianType notnative = CHCK_BUFFER_ENDIAN_BIG;
   if (chckBufferIsBigEndian()) notnative = CHCK_BUFFER_ENDIAN_LITTLE;

   /* TEST: little endian buffer */
   {
      static const char test[] = "\x12this integer is 5:\x5\x0\x0\0\1\0\1";
      chckBuffer *buf = chckBufferNewFromPointer(test, sizeof(test)-1, CHCK_BUFFER_ENDIAN_LITTLE);
      assert(chckBufferReadString(buf, 1, &str) == 1);
      assert(strcmp(str, "this integer is 5:") == 0); free(str);
      assert(chckBufferReadInt32(buf, &i) == 1);
      assert(i == 5);
      assert(chckBufferReadInt16(buf, &s) == 1);
      assert(s == 1);
      assert(chckBufferReadInt8(buf, &c) == 1);
      assert(c == 1);
      assert(chckBufferGetOffset(buf) - chckBufferGetSize(buf) == 0);
      chckBufferFree(buf);
   }

   /* TEST: big endian buffer */
   {
      static const char test[] = "\x12this integer is 5:\x0\x0\0\x5\0\1\1";
      chckBuffer *buf = chckBufferNewFromPointer(test, sizeof(test)-1, CHCK_BUFFER_ENDIAN_BIG);
      assert(chckBufferReadString(buf, 1, &str) == 1);
      assert(strcmp(str, "this integer is 5:") == 0); free(str);
      assert(chckBufferReadInt32(buf, &i) == 1);
      assert(i == 5);
      assert(chckBufferReadInt16(buf, &s) == 1);
      assert(s == 1);
      assert(chckBufferReadInt8(buf, &c) == 1);
      assert(c == 1);
      assert(chckBufferGetOffset(buf) - chckBufferGetSize(buf) == 0);
      chckBufferFree(buf);
   }

   /* TEST: native buffer write && read && resize */
   {
      i = 8; s = 6;
      chckBuffer *buf = chckBufferNew(5, CHCK_BUFFER_ENDIAN_NATIVE);
      assert(chckBufferIsNativeEndian(buf) == 1);

      assert(chckBufferWriteString(buf, 4, "test") == 1);
      assert(chckBufferResize(buf, 5+sizeof(int32_t)+sizeof(int16_t)) == 1);
      assert(chckBufferGetSize(buf) == 5+sizeof(int32_t)+sizeof(int16_t));
      assert(chckBufferGetOffset(buf) == 5);
      assert(chckBufferGetOffsetPointer(buf) != chckBufferGetPointer(buf));

      assert(chckBufferWriteUInt32(buf, i) == 1);
      assert(chckBufferWriteUInt16(buf, s) == 1);
      assert(chckBufferGetOffset(buf) - chckBufferGetSize(buf) == 0);

      assert(chckBufferSeek(buf, 0, SEEK_SET) == chckBufferGetOffset(buf));
      assert(chckBufferGetOffset(buf) == 0);

      assert(chckBufferReadString(buf, 1, &str) == 1);
      assert(strcmp(str, "test") == 0); free(str);
      assert(chckBufferReadInt32(buf, &i) == 1);
      assert(i == 8);
      assert(chckBufferReadInt16(buf, &s) == 1);
      assert(s == 6);
      assert(chckBufferGetOffset(buf) - chckBufferGetSize(buf) == 0);
      chckBufferFree(buf);
   }

   /* TEST: non-native buffer write && read && resize */
   {
      i = 8; s = 6;
      chckBuffer *buf = chckBufferNew(5, notnative);
      assert(chckBufferIsNativeEndian(buf) == 0);

      assert(chckBufferWriteString(buf, 4, "test") == 1);
      assert(chckBufferResize(buf, 5+sizeof(int32_t)+sizeof(int16_t)) == 1);
      assert(chckBufferGetSize(buf) == 5+sizeof(int32_t)+sizeof(int16_t));
      assert(chckBufferGetOffset(buf) == 5);
      assert(chckBufferGetOffsetPointer(buf) != chckBufferGetPointer(buf));

      assert(chckBufferWriteUInt32(buf, i) == 1);
      assert(chckBufferWriteUInt16(buf, s) == 1);
      assert(chckBufferGetOffset(buf) - chckBufferGetSize(buf) == 0);

      assert(chckBufferSeek(buf, 0, SEEK_SET) == chckBufferGetOffset(buf));
      assert(chckBufferGetOffset(buf) == 0);

      assert(chckBufferReadString(buf, 1, &str) == 1);
      assert(strcmp(str, "test") == 0); free(str);
      assert(chckBufferReadInt32(buf, &i) == 1);
      assert(i == 8);
      assert(chckBufferReadInt16(buf, &s) == 1);
      assert(s == 6);
      assert(chckBufferGetOffset(buf) - chckBufferGetSize(buf) == 0);
      chckBufferFree(buf);
   }

   /* TEST: zlib compression && decompression */
   {
      static const char uncompressed[] = ".....................";
      static const char compressed[] = { 0x78, 0x9c, 0xd3, 0xd3, 0xc3, 0x2, 0x18, 0x0, 0x2d, 0x5e, 0x3, 0xc7 };
      chckBuffer *buf = chckBufferNewFromPointer(uncompressed, sizeof(uncompressed), CHCK_BUFFER_ENDIAN_NATIVE);
#if HAS_ZLIB
      assert(chckBufferCompressZlib(buf) == 1);
      assert(chckBufferGetSize(buf) == sizeof(compressed));
      assert(memcmp(chckBufferGetPointer(buf), compressed, sizeof(compressed)) == 0);

      assert(chckBufferDecompressZlib(buf) == 1);
      assert(chckBufferGetSize(buf) == sizeof(uncompressed));
      assert(memcmp(chckBufferGetPointer(buf), uncompressed, sizeof(uncompressed)) == 0);
#else
      assert(chckBufferCompressZlib(buf) == 0);
      assert(chckBufferDecompressZlib(buf) == 0);
#endif
      chckBufferFree(buf);
   }

   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
