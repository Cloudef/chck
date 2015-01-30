#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

int main(void)
{
   /* TEST: ownership move */
   {
      struct chck_buffer buf;
      assert(chck_buffer(&buf, 1, CHCK_ENDIANESS_NATIVE));
      buf.copied = false;
      chck_buffer_release(&buf);
      free(buf.buffer); // should not SIGSEGV
   }

   /* TEST: over-read */
   {
      struct chck_buffer buf;
      static char data[] = "yolo I have only this many bytes";
      assert(chck_buffer_from_pointer(&buf, data, sizeof(data), CHCK_ENDIANESS_NATIVE));
      char bb[sizeof(data)];
      assert(chck_buffer_read(bb, 1, sizeof(data), &buf) == sizeof(data));
      chck_buffer_seek(&buf, 0, SEEK_SET);
      assert(chck_buffer_read(bb, 1, sizeof(data) * 4, &buf) == sizeof(data));
      chck_buffer_seek(&buf, 0, SEEK_SET);
      assert(chck_buffer_read(bb, 8, sizeof(data), &buf) == 4);
      assert(chck_buffer_read(bb, 12, sizeof(data), &buf) == 0);
      assert(chck_buffer_read(bb, 1, sizeof(data), &buf) == 1);
      chck_buffer_release(&buf);
   }

   /* TEST: little endian buffer */
   {
      static const struct {
         char *data;
         size_t size;
         enum chck_endianess endianess;
      } tests[] = {
         {
            .data = "\x1\x12this integer is 5:\x12this integer is 5:\x5\x0\x0\0\1\0\1",
            .size = sizeof("\x0\x12this integer is 5:\x12this integer is 5:\x5\x0\x0\0\1\0\1") - 1,
            .endianess = CHCK_ENDIANESS_LITTLE,
         }, {
            .data = "\x1\x12this integer is 5:\x12this integer is 5:\x0\x0\0\x5\0\1\1",
            .size = sizeof("\x0\x12this integer is 5:\x12this integer is 5:\x0\x0\0\x5\0\1\1") - 1,
            .endianess = CHCK_ENDIANESS_BIG,
         },
      };

      for (int t = 0; t < 2; ++t) {
         int32_t i;
         int16_t s;
         int8_t c;
         char *str;
         struct chck_buffer buf;
         assert(chck_buffer_from_pointer(&buf, tests[t].data, tests[t].size, tests[t].endianess));
         assert(buf.endianess == tests[t].endianess);
         size_t len;
         assert(chck_buffer_read_string(&str, &len, &buf));
         assert(len == strlen("this integer is 5:"));
         assert(!strcmp(str, "this integer is 5:")); free(str);
         assert(chck_buffer_read_string_of_type(&str, NULL, sizeof(uint8_t), &buf));
         assert(strcmp(str, "this integer is 5:") == 0); free(str);
         assert(chck_buffer_read_int(&i, sizeof(i), &buf));
         assert(i == 5);
         assert(chck_buffer_read_int(&s, sizeof(s), &buf));
         assert(s == 1);
         assert(chck_buffer_read_int(&c, sizeof(c), &buf));
         assert(c == 1);
         assert((buf.curpos - buf.buffer) - buf.size == 0);
         chck_buffer_release(&buf);
      }
   }

   /* TEST: buffer write && read && resize */
   {
      int64_t l = 32;
      int32_t i = 8;
      int16_t s = 6;
      enum chck_endianess tests[] = { CHCK_ENDIANESS_NATIVE, !chck_endianess() };
      for (int t = 0; t < 2; ++t) {
         char *str;
         struct chck_buffer buf;
         assert(chck_buffer(&buf, 5+6, tests[t]));
         assert((tests[t] == CHCK_ENDIANESS_NATIVE) == chck_buffer_native_endianess(&buf));

         assert(chck_buffer_write_string("test", 4, &buf) == 1);
         assert(chck_buffer_write_string_of_type("test", 4, sizeof(uint8_t), &buf));
         assert(chck_buffer_resize(&buf, 5+6+sizeof(uint64_t)+sizeof(int32_t)+sizeof(int16_t)));
         assert(buf.size == 5+6+sizeof(uint64_t)+sizeof(int32_t)+sizeof(int16_t));
         assert(buf.curpos - buf.buffer == 6+5);
         assert(buf.buffer != buf.curpos);

         assert(chck_buffer_write_int(&l, sizeof(l), &buf));
         assert(chck_buffer_write_int(&i, sizeof(i), &buf));
         assert(chck_buffer_write_int(&s, sizeof(s), &buf));
         assert((buf.curpos - buf.buffer) - buf.size == 0);

         assert(chck_buffer_seek(&buf, 0, SEEK_SET) == buf.curpos - buf.buffer);
         assert(buf.curpos - buf.buffer == 0);

         assert(chck_buffer_read_string(&str, NULL, &buf));
         assert(strcmp(str, "test") == 0); free(str);
         assert(chck_buffer_read_string_of_type(&str, NULL, sizeof(uint8_t), &buf));
         assert(strcmp(str, "test") == 0); free(str);
         assert(chck_buffer_read_int(&l, sizeof(l), &buf));
         assert(l == 32);
         assert(chck_buffer_read_int(&i, sizeof(i), &buf));
         assert(i == 8);
         assert(chck_buffer_read_int(&s, sizeof(s), &buf));
         assert(s == 6);
         assert((buf.curpos - buf.buffer) - buf.size == 0);
         chck_buffer_release(&buf);
      }
   }

   /* TEST: zlib compression && decompression */
   {
      static char uncompressed[] = ".....................";
      static char compressed[] = { 0x78, 0x9c, 0xd3, 0xd3, 0xc3, 0x2, 0x18, 0x0, 0x2d, 0x5e, 0x3, 0xc7 };
      struct chck_buffer buf;
      chck_buffer_from_pointer(&buf, uncompressed, sizeof(uncompressed), CHCK_ENDIANESS_NATIVE);
#if HAS_ZLIB
      assert(chck_buffer_compress_zlib(&buf));
      assert(buf.size == sizeof(compressed));
      assert(memcmp(buf.buffer, compressed, sizeof(compressed)) == 0);

      assert(chck_buffer_decompress_zlib(&buf));
      assert(buf.size == sizeof(uncompressed));
      assert(memcmp(buf.buffer, uncompressed, sizeof(uncompressed)) == 0);
#else
      assert(!chck_buffer_compress_zlib(&buf));
      assert(!chck_buffer_decompress_zlib(&buf));
#endif
      chck_buffer_release(&buf);
   }

   return EXIT_SUCCESS;
}
