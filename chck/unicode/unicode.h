#ifndef __chck_unicode_h__
#define __chck_unicode_h__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <chck/macros.h>

#define CHCK_REPLACEMENT_CHAR 0xFFFD

#define CHCK_UTF8_ACCEPT 0
#define CHCK_UTF8_REJECT 1

enum chck_utf16_error
{
   CHCK_UTF16_OK,
   CHCK_UTF16_UNEXPECTED_HIGH,
   CHCK_UTF16_UNEXPECTED_LOW,
};

CHCK_NONULL uint8_t chck_utf32_encode(uint32_t dec, char out[4]);
CHCK_NONULLV(2,3) uint8_t chck_utf16_encode(uint16_t dec, char out[4], uint16_t *in_out_hi, enum chck_utf16_error *out_error);
CHCK_NONULL uint32_t chck_utf8_decode(uint32_t *state /* accept, reject */, uint32_t *codep, uint8_t byte);
CHCK_NONULL uint8_t chck_utf8_mblen(const char u8[4]);
CHCK_NONULL bool chck_utf8_validate(const char u8[4]);
CHCK_NONULL uint32_t chck_utf8_codepoint(const char u8[4]);
CHCK_NONULL bool chck_utf8_strlen(const char *u8, size_t len, size_t *out_len);

#endif /* __chck_unicode_h__ */
