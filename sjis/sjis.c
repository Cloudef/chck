#include "sjis.h"
#include "utf8sjis.h"
#include <stdlib.h> /* for calloc, free, etc... */
#include <string.h> /* for memcpy/strlen */
#include <assert.h> /* for assert */

enum { RETURN_OK = 1, RETURN_FAIL = 0 };

/* \brief resize buffer */
static int chckResizeBuf(unsigned char **buf, size_t *size, size_t nsize)
{
   void *tmp;

   if (nsize < *size || !(tmp = realloc(*buf, nsize))) {
      if (!(tmp = malloc(nsize))) return RETURN_FAIL;
      memcpy(tmp, *buf, (nsize > *size ? *size : nsize));
      free(*buf);
   }

   *buf = tmp;
   *size = nsize;
   return RETURN_OK;
}

/* \brief put bytes to buffer */
static int chckPutBuf(unsigned char **buf, size_t *o, size_t *size, const char *bytes)
{
   size_t len = strlen(bytes);
   if (len > *size - *o && chckResizeBuf(buf, size, *size * 2) != RETURN_OK)
      return RETURN_FAIL;

   memcpy(*buf + *o, bytes, len);
   *o += len;
   return RETURN_OK;
}

/* \brief convert SJIS buffer to UTF8 buffer */
char* chckSJISToUTF8(const unsigned char *sjis, size_t size, size_t *outSize, int terminate)
{
   unsigned char *dec;
   size_t i, d, dsize = size;
   unsigned short mb;
   assert(sjis && size != 0);

   if (!(dec = malloc(dsize)))
      goto fail;

   for (i = 0, d = 0; i < size; ++i) {
      dec[d] = 0x00; /* this is the unexpected case */

      /* modified ASCII */
      if (sjis[i] <= 0x7f) {
         if (sjis[i] == 0x5c) { // YEN
            chckPutBuf(&dec, &d, &dsize, "\xc2\xa5");
         } else if (sjis[i] == 0x7e) { // OVERLINE
            chckPutBuf(&dec, &d, &dsize, "\xe2\x80\xbe");
         } else {
            chckPutBuf(&dec, &d, &dsize, (const char[]){ sjis[i], 0 });
         }
      }

      /* single byte half-width katakana */
      if (sjis[i] >= 0xa1 && sjis[i] <= 0xdf) {
         char halfWidthKatakana[4] = { 0xef, 0xbd, sjis[i], 0x00 };
         if (sjis[i] >= 0xc0) {
            halfWidthKatakana[2] = 0xbe;
            halfWidthKatakana[3] = 0x80 + sjis[i] - 0xc0;
         }
         chckPutBuf(&dec, &d, &dsize, halfWidthKatakana);
      }

      /* multibyte */
      if (sjis[i] >= 0x81 && sjis[i] <= 0xfc) {
         const chckMapSJISToUTF8 *data = NULL;
         for (mb = 0; mb < mbSJISToUTF8MapLength; ++mb) {
            if (!memcmp(mbSJISToUTF8Map[mb].sjis, sjis + i, 2)) {
               data = &mbSJISToUTF8Map[mb];
               break;
            }
         }

         if (data) {
            chckPutBuf(&dec, &d, &dsize, data->utf8);
         } else {
            chckPutBuf(&dec, &d, &dsize, "\xef\xbf\xbd"); // invalid
         }

         i += 1; // skip byte
      }
   }

   /* resize buffer to real size */
   terminate = (terminate ? 1 : 0);
   size = d + (dec[d-1] != 0x00 ? terminate : 0);
   chckResizeBuf(&dec, &dsize, size);
   if (terminate && dec[d-1] != 0x00) dec[d] = 0x00;
   if (outSize) *outSize = size;
   return (char*)dec;

fail:
   if (dec) free(dec);
   return NULL;
}

/* \brief convert UTF8 buffer to SJIS string */
unsigned char* chckUTF8ToSJIS(const char *input, size_t size, size_t *outSize, int terminate)
{
   unsigned char *dec;
   const unsigned char *utf8 = (const unsigned char*)input;
   size_t i, d, dsize = size;
   unsigned short mb;
   assert(input && size != 0);

   if (!(dec = malloc(dsize)))
      goto fail;

   for (i = 0, d = 0; i < size; ++i) {
      dec[d] = 0x00; /* this is the unexpected case */

      /* ASCII */
      if (utf8[i] <= 0x7f) {
         if (utf8[i] == 0x5c) { // BACKSLASH
            chckPutBuf(&dec, &d, &dsize, "\x81\x5f");
         } else if (utf8[i] == 0x7e) { // TILDE
            chckPutBuf(&dec, &d, &dsize, "\x81\x60");
         } else {
            chckPutBuf(&dec, &d, &dsize, (const char[]){ utf8[i], 0 });
         }
      }

      /* half-width katakana */
      if (utf8[i + 1] >= 0xbd && utf8[i + 1] <= 0xbe) {
         char halfWidthKatakana[2] = { utf8[i + 2], 0x00 };
         if (utf8[i + 1] >= 0xbe) halfWidthKatakana[0] = 0xc0 + utf8[i + 2] - 0x80;
         chckPutBuf(&dec, &d, &dsize, halfWidthKatakana);
      }

      /* multibyte */
      if ((utf8[i + 1] & 0xc0) == 0x80) {
         char mblen = 0;
         const chckMapSJISToUTF8 *data = NULL;
         while ((utf8[i + mblen + 1] & 0xc0) == 0x80) ++mblen;

         for (mb = 0; mb < mbSJISToUTF8MapLength; ++mb) {
            if (!memcmp(mbSJISToUTF8Map[mb].utf8, utf8 + i, mblen + 1)) {
               data = &mbSJISToUTF8Map[mb];
               break;
            }
         }

         if (data) {
            chckPutBuf(&dec, &d, &dsize, data->sjis);
         } else {
            chckPutBuf(&dec, &d, &dsize, "\x81\x9f"); // invalid
         }

         i += mblen; // skip bytes
      }
   }

   /* resize buffer to real size */
   terminate = (terminate ? 1 : 0);
   size = d + (dec[d-1] != 0x00 ? terminate : 0);
   chckResizeBuf(&dec, &dsize, size);
   if (terminate && dec[d-1] != 0) dec[d] = 0x00;
   if (outSize) *outSize = size;
   return dec;

fail:
   if (dec) free(dec);
   return NULL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
