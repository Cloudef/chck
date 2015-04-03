#include "unicode.h"
#include <stdlib.h>
#include <string.h>

#undef NDEBUG
#include <assert.h>

int main(void)
{
   // test: UTF32 love hotel
   {
      char out[4];
      assert(chck_utf32_encode('a', out) == 1);
      assert(!strncmp(out, "a", 1));
      assert(chck_utf32_encode(0x1f3e9, out) == 4);
      assert(!strncmp(out, "🏩", 4));
      assert(chck_utf32_encode(0, out) == 0);
   }

   // test: UTF16 love hotel
   {
      char out[4];
      uint16_t hi = 0;
      enum chck_utf16_error error;
      assert(chck_utf16_encode('a', out, &hi, &error) == 1);
      assert(error == CHCK_UTF16_OK);
      assert(hi == 0);
      assert(!strncmp(out, "a", 1));

      assert(chck_utf16_encode(0, out, &hi, &error) == 0);
      assert(error == CHCK_UTF16_OK);
      assert(hi == 0);

      assert(chck_utf16_encode(0xDFE9, out, &hi, &error) == 0);
      assert(error == CHCK_UTF16_UNEXPECTED_LOW);
      assert(hi == 0);

      assert(chck_utf16_encode(0xD83C, out, &hi, &error) == 0);
      assert(error == CHCK_UTF16_OK);
      assert(hi != 0);
      assert(chck_utf16_encode(0xD83C, out, &hi, &error) == 0);
      assert(error == CHCK_UTF16_UNEXPECTED_HIGH);
      assert(hi == 0);

      assert(chck_utf16_encode(0xD83C, out, &hi, &error) == 0);
      assert(error == CHCK_UTF16_OK);
      assert(hi != 0);
      assert(chck_utf16_encode(0xDFE9, out, &hi, &error) == 4);
      assert(error == CHCK_UTF16_OK);
      assert(!strncmp(out, "🏩", 4));
   }

   // test: mblen
   {
      assert(chck_utf8_mblen("🏩") == 4);
      assert(chck_utf8_mblen("a") == 1);
      assert(chck_utf8_mblen("\xb1") == 0);
   }

   // test: validate
   {
      assert(chck_utf8_validate("🏩"));
      assert(chck_utf8_validate("a"));
      assert(chck_utf8_validate("\xc3\xb1"));
      assert(!chck_utf8_validate("\xc3\x28"));
      assert(!chck_utf8_validate("\xa0\xa1"));
      assert(chck_utf8_validate("\xe2\x82\xa1"));
      assert(!chck_utf8_validate("\xe2\x28\xa1"));
      assert(!chck_utf8_validate("\xe2\x82\x28"));
      assert(!chck_utf8_validate("\xf0\x28\x8c\xbc"));
      assert(!chck_utf8_validate("\xf0\x90\x28\xbc"));
      assert(chck_utf8_validate("\xf0\x90\x8c\xbc"));
      assert(!chck_utf8_validate("\xf0\x28\x8c\x28"));
   }

   // test: codepoint
   {
      assert(chck_utf8_codepoint("🏩") == 0x1f3e9);
      assert(chck_utf8_codepoint("\xc3\x28") == CHCK_REPLACEMENT_CHAR);
      assert(chck_utf8_codepoint("a") == 'a');
   }
}
