#include "unicode.h"
#include <assert.h>

uint8_t
chck_utf32_encode(uint32_t dec, char out[4])
{
   assert(out);

   if (!dec)
      return 0;

   const uint8_t bits[4] = { 0x00, 0xC0, 0xE0, 0xF0 };
   const uint8_t mb = (dec < 0x80 ? 0 : (dec < 0x800 ? 1 : (dec < 0x10000 ? 2 : 3)));

   out[0] = (dec >> (mb * 6)) | bits[mb];
   for (int32_t i = mb * 6 - 6, c = 1; i >= 0 && c < mb + 1; i -= 6, ++c)
      out[c] = ((dec >> i) & 0x3F) | 0x80;

   return mb + 1;
}

uint8_t
chck_utf16_encode(uint16_t dec, char out[4], uint16_t *in_out_hi, enum chck_utf16_error *out_error)
{
   assert(out && in_out_hi);

   if (out_error)
      *out_error = CHCK_UTF16_OK;

   uint32_t u32dec = dec;
   const uint16_t hi = *in_out_hi;

#define IS_HIGH_SURROGATE(dec) (((dec) & 0xFC00) == 0xD800)
#define IS_LOW_SURROGATE(dec) (((dec) & 0xFC00) == 0xDC00)
#define DECODE_SURROGATE_PAIR(hi, lo) ((((hi) & 0x3FF) << 10) + ((lo) & 0x3FF) + 0x10000)

   if (hi) {
      if (IS_LOW_SURROGATE(dec)) {
         u32dec = DECODE_SURROGATE_PAIR(hi, dec);
         *in_out_hi = 0;
      } else {
         if (out_error)
            *out_error = CHCK_UTF16_UNEXPECTED_HIGH;
         *in_out_hi = 0;
         return 0;
      }
   } else {
      if (IS_HIGH_SURROGATE(dec)) {
         *in_out_hi = dec;
         return 0;
      } else if (IS_LOW_SURROGATE(dec)) {
         if (out_error)
            *out_error = CHCK_UTF16_UNEXPECTED_LOW;
         return 0;
      }
   }

#undef DECODE_SURROGATE_PAIR
#undef IS_LOW_SURROGATE
#undef IS_HIGH_SURROGATE

   return chck_utf32_encode(u32dec, out);
}

// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

static const uint8_t utf8d[] = {
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
   1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
   8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
   0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
   0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
   0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
   1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
   1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
   1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
   1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
};

uint32_t
chck_utf8_decode(uint32_t *state, uint32_t *codep, uint8_t byte)
{
   assert(state && codep);
   const uint32_t type = utf8d[byte];
   *codep = (*state != CHCK_UTF8_ACCEPT ? (byte & 0x3fu) | (*codep << 6) : (0xff >> type) & byte);
   *state = utf8d[256 + *state * 16 + type];
   return *state;
}

uint8_t
chck_utf8_mblen(const char u8[4])
{
   assert(u8);

   struct {
      uint8_t lo, hi, mb;
   } map[] = {
      { 0xC0, 0x80, 0 },
      { 0x80, 0, 1 },
      { 0xE0, 0xC0, 2 },
      { 0xF0, 0xE0, 3 },
      { 0xF8, 0xF0, 4 },
   };

   for (uint8_t i = 0; i < 5; ++i)
      if (!((u8[0] & map[i].lo) ^ map[i].hi))
         return map[i].mb;

   return 0;
}

bool
chck_utf8_validate(const char u8[4])
{
   assert(u8);

   const uint8_t mb = chck_utf8_mblen(u8);
   uint32_t state = CHCK_UTF8_ACCEPT, cp;
   for (uint8_t i = 0; i < mb; ++i) {
      if (chck_utf8_decode(&state, &cp, u8[i]) == CHCK_UTF8_REJECT)
         return false;
   }

   return (mb > 0);
}

uint32_t
chck_utf8_codepoint(const char u8[4])
{
   assert(u8);

   const uint8_t mb = chck_utf8_mblen(u8);
   uint32_t state = CHCK_UTF8_ACCEPT, cp = u8[0];
   for (uint8_t i = 0; i < mb; ++i) {
      if (chck_utf8_decode(&state, &cp, u8[i]) == CHCK_UTF8_REJECT)
         return CHCK_REPLACEMENT_CHAR;
   }

   return (mb > 0 ? cp : CHCK_REPLACEMENT_CHAR);
}

bool
chck_utf8_strlen(const char *u8, size_t len, size_t *out_len)
{
   assert(u8 && out_len);
   *out_len = 0;

   uint32_t state = CHCK_UTF8_ACCEPT, cp;
   for (size_t i = 0; i < len; ++i) {
      if (chck_utf8_decode(&state, &cp, u8[i]) == CHCK_UTF8_ACCEPT)
         ++*out_len;
   }

   return (state == CHCK_UTF8_ACCEPT);
}
