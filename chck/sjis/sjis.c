#include "sjis.h"
#include "utf8sjis.h"
#include <chck/overflow/overflow.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#pragma GCC diagnostic ignored "-Woverflow"

static inline bool
resize(uint8_t **buf, size_t *size, size_t nsize)
{
   assert(buf && size);

   if (nsize == *size)
      return true;

   void *tmp;
   if (!(tmp = realloc(*buf, nsize)))
      return false;

   *buf = tmp;
   *size = nsize;
   return true;
}

static inline bool
resize_mul(uint8_t **buf, size_t *size, size_t nsize, size_t mul)
{
   size_t nsz;
   if (unlikely(chck_mul_ofsz(nsize, mul, &nsz)))
      return false;

   return resize(buf, size, nsz);
}

static inline bool
put(uint8_t **buf, size_t *o, size_t *size, const char *bytes)
{
   assert(buf && o && size && bytes && *size > *o);

   size_t len = strlen(bytes);
   if (len >= *size - *o && !resize_mul(buf, size, *size, 2))
      return false;

   memcpy(*buf + *o, bytes, len);
   *o += len;
   return true;
}

char*
chck_sjis_to_utf8(const uint8_t *sjis, size_t size, size_t *out_size, bool terminate)
{
   assert(sjis && size != 0);

   size_t dsize;
   uint8_t *dec;
   if (!(dec = malloc((dsize = size))))
      return NULL;

   size_t d = 0;
   for (size_t i = 0; i < size; ++i) {
      dec[d] = 0x00; /* this is the unexpected case */

      /* modified ASCII */
      if (sjis[i] <= 0x7f) {
         if (sjis[i] == 0x5c) { // YEN
            put(&dec, &d, &dsize, "\xc2\xa5");
         } else if (sjis[i] == 0x7e) { // OVERLINE
            put(&dec, &d, &dsize, "\xe2\x80\xbe");
         } else {
            put(&dec, &d, &dsize, (const char[]){ sjis[i], 0 });
         }
      }

      /* single byte half-width katakana */
      if (sjis[i] >= 0xa1 && sjis[i] <= 0xdf) {
         char hw_katakana[4] = { 0xef, 0xbd, sjis[i], 0x00 };
         if (sjis[i] >= 0xc0) {
            hw_katakana[2] = 0xbe;
            hw_katakana[3] = 0x80 + sjis[i] - 0xc0;
         }
         put(&dec, &d, &dsize, hw_katakana);
      }

      /* multibyte */
      if (sjis[i] >= 0x81 && sjis[i] <= 0xfc) {
         const struct map_sjis_utf8 *data = NULL;
         for (uint16_t mb = 0; mb < map_sjis_utf8_size; ++mb) {
            if (!memcmp(map_sjis_utf8[mb].sjis, sjis + i, 2)) {
               data = &map_sjis_utf8[mb];
               break;
            }
         }

         if (data) {
            put(&dec, &d, &dsize, data->utf8);
         } else {
            put(&dec, &d, &dsize, "\xef\xbf\xbd"); // invalid
         }

         i += 1; // skip byte
      }
   }

   if (d > 0) {
      /* resize buffer to real size */
      terminate = (terminate ? true : false);
      size = d + (dec[d - 1] != 0x00 ? terminate : 0);
      resize(&dec, &dsize, size);
      if (terminate && dec[d - 1] != 0x00) dec[d] = 0x00;
   } else {
      size = 0;
      free(dec);
      dec = NULL;
   }

   if (out_size)
      *out_size = size;

   return (char*)dec;
}

uint8_t*
chck_utf8_to_sjis(const char *input, size_t size, size_t *out_size, bool terminate)
{
   const uint8_t *utf8 = (const uint8_t*)input;
   assert(input && size != 0);

   size_t dsize;
   uint8_t *dec;
   if (!(dec = malloc((dsize = size))))
      return NULL;

   size_t d = 0;
   for (size_t i = 0; i < size; ++i) {
      dec[d] = 0x00; /* this is the unexpected case */

      /* ASCII */
      if (utf8[i] <= 0x7f) {
         if (utf8[i] == 0x5c) { // BACKSLASH
            put(&dec, &d, &dsize, "\x81\x5f");
         } else if (utf8[i] == 0x7e) { // TILDE
            put(&dec, &d, &dsize, "\x81\x60");
         } else {
            put(&dec, &d, &dsize, (const char[]){ utf8[i], 0 });
         }
      }

      /* half-width katakana */
      if (utf8[i + 1] >= 0xbd && utf8[i + 1] <= 0xbe) {
         char hw_katakana[2] = { utf8[i + 2], 0x00 };
         if (utf8[i + 1] >= 0xbe) hw_katakana[0] = 0xc0 + utf8[i + 2] - 0x80;
         put(&dec, &d, &dsize, hw_katakana);
      }

      /* multibyte */
      if ((utf8[i + 1] & 0xc0) == 0x80) {
         uint8_t mblen = 0;
         const struct map_sjis_utf8 *data = NULL;
         while (i + mblen + 1 < size && (utf8[i + mblen + 1] & 0xc0) == 0x80)
            ++mblen;

         for (uint16_t mb = 0; mb < map_sjis_utf8_size; ++mb) {
            if (!memcmp(map_sjis_utf8[mb].utf8, utf8 + i, mblen + 1)) {
               data = &map_sjis_utf8[mb];
               break;
            }
         }

         if (data) {
            put(&dec, &d, &dsize, data->sjis);
         } else {
            put(&dec, &d, &dsize, "\x81\x9f"); // invalid
         }

         i += mblen; // skip bytes
      }
   }

   if (d > 0) {
      /* resize buffer to real size */
      terminate = (terminate ? true : false);
      size = d + (dec[d - 1] != 0x00 ? terminate : 0);
      resize(&dec, &dsize, size);
      if (terminate && dec[d - 1] != 0) dec[d] = 0x00;
   } else {
      size = 0;
      free(dec);
      dec = NULL;
   }

   if (out_size)
      *out_size = size;

   return dec;
}
