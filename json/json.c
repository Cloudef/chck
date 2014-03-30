#include "json.h"
#include <stdio.h>  /* for printf, etc.. */
#include <stdarg.h> /* for va_list, etc.. */
#include <stdlib.h> /* for calloc, free, etc.. */
#include <assert.h> /* for assert */
#include <string.h> /* for memcpy/memset */
#include <ctype.h>  /* for isspace */

enum { RETURN_OK = 1, RETURN_FAIL = 0 };

/* \brief awesome json struct */
typedef struct _chckJson {
   union {
      char *string;
      struct _chckJson *value;
      char boolean;
   };
   struct _chckJson *next;
   chckJsonType type;
} _chckJson;

typedef struct _chckJsonDecoder {
   const char *currentChar;
   const char *lineStart;
   unsigned int currentLine; /* 2^32 - 1 lines */
   chckJsonErrorCallback callback;
   struct _chckJson *json;
   void *userdata;
   int allowComments;
} _chckJsonDecoder;

static void chckJsonDecoderDecodeValue(chckJsonDecoder *decoder, int isProperty);

static void* chckRealloc(void *ptr, size_t osize, size_t size)
{
   void *newPtr;

   if (!(newPtr = realloc(ptr, size))) {
      if (!(newPtr = malloc(size)))
         goto fail;

      memcpy(newPtr, ptr, osize);
      free(ptr);
   }

   return newPtr;

fail:
   return NULL;
}

static void chckJsonDecoderThrow(chckJsonDecoder *decoder, chckJsonError code, const char *fmt, ...)
{
   if (!decoder->callback)
      return;

   va_list args;
   char message[2048];
   memset(message, 0, sizeof(message));
   va_start(args, fmt);
   vsnprintf(message, sizeof(message) - 1, fmt, args);
   va_end(args);

   size_t i;
   char line[128];
   memset(line, 0, sizeof(line));
   for (i = 0; i < sizeof(line) && decoder->lineStart[i] && decoder->lineStart[i] != '\n'; ++i)
      line[i] = decoder->lineStart[i];

   decoder->callback(decoder, decoder->currentLine, decoder->currentChar - decoder->lineStart + 1, line, code, message);
}

static char chckJsonDecoderSkip(chckJsonDecoder *decoder, size_t numChars)
{
   decoder->currentChar += numChars;
   return *decoder->currentChar;
}

static char chckJsonDecoderAdvance(chckJsonDecoder *decoder, int skipWhitespace)
{
   if (!*decoder->currentChar)
      return *decoder->currentChar;

   do {
      decoder->currentChar++;
      if (*decoder->currentChar == '\n') {
         decoder->currentLine++;
         decoder->lineStart = decoder->currentChar + 1;
      }
   } while (*decoder->currentChar == '\n' ||
           (skipWhitespace && isspace(*decoder->currentChar)));

   return *decoder->currentChar;
}

static int chckJsonDecoderDecodeHex(chckJsonDecoder *decoder)
{
   int result = 0, i = 0;

   for (i = 0; i < 4; ++i) {
      char hex = 0;
      char chr = chckJsonDecoderAdvance(decoder, 0);
      if (chr >= '0' && chr <= '9') {
         hex += chr - '0';
      } else if (chr >= 'A' && chr <= 'F') {
         hex += 10 + chr - 'A';
      } else if (chr >= 'a' && chr <= 'f') {
         hex += 10 + chr - 'a';
      } else {
         chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_UNEXPECTED, "Unexpected chr character '%c'", *decoder->currentChar);
      }
      result |= hex << ((3 - i) * 4);
   }

   return result;
}

static void chckJsonDecoderStringInsert(chckJsonDecoder *decoder, char **inOutString, size_t *inOutLen, size_t *inOutAlloc)
{
   int chr;
   char *string, chrSize = 0;
   size_t len, alloc;
   assert(inOutString && inOutLen && inOutAlloc);

   string = *inOutString;
   alloc = *inOutAlloc;
   len = *inOutLen;
   chr = *decoder->currentChar;

   if (*decoder->currentChar == '\\') {
      chr = chckJsonDecoderAdvance(decoder, 0);
      switch (chr) {
         case '\"': break;
         case '\\': break;
         case '/': break;

         case 'b': chr = '\b'; break;
         case 'f': chr = '\f'; break;
         case 'n': chr = '\n'; break;
         case 'r': chr = '\r'; break;
         case 't': chr = '\t'; break;

         case 'u': /* 16 bit unicode character */
            chr = chckJsonDecoderDecodeHex(decoder);
            if (chr <= 0x7F) chrSize = 0;
            else if (chr <= 0x7FF) chrSize = 1;
            else chrSize = 2;
            break;

         default:
            chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_UNEXPECTED, "Invalid character '%c', everything else expect '\', '\"' and some control characters are allowed inside strings", *decoder->currentChar);
            return;
      }
   }

   if (!string) {
      if ((string = calloc(1, 128))) {
         alloc = 128;
         len = 0;
      } else {
         len = alloc = 0;
         chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_OUT_OF_MEMORY, "Cannot allocate string buffer.");
      }
   }

   if (alloc && len + chrSize >= alloc - 1) {
      void *tmp;
      if ((tmp = chckRealloc(string, alloc, alloc * 2))) {
         string = tmp;
         memset(string + alloc, 0, alloc);
         alloc *= 2;
      } else {
         chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_OUT_OF_MEMORY, "Cannot grow string buffer.");
      }
   }

   if (alloc && len + chrSize < alloc - 1) {
      char *u8 = string + len;
      if (chrSize == 2) {
         u8[0] = (0xE0 | (chr >> 12));
         u8[1] = (0x80 | ((chr >> 6) & 0x3F));
         u8[2] = (0x80 | (chr & 0x3F));
      } else if (chrSize == 1) {
         u8[0] = (0xC0 | (chr >> 6));
         u8[1] = (0x80 | (chr & 0x3F));
      } else {
         u8[0] = (char)chr;
      }
      len += chrSize + 1;
   }

   *inOutString = string;
   *inOutAlloc = alloc;
   *inOutLen = len;
}

static void chckJsonDecoderDecodeObject(chckJsonDecoder *decoder)
{
   if (*decoder->currentChar != '{') {
      chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_UNEXPECTED, "Expected opening '{'");
      return;
   }

   printf("START OBJECT\n");

   while (chckJsonDecoderAdvance(decoder, 1)) {
      if (*decoder->currentChar == '}')
         break;

      chckJsonDecoderDecodeValue(decoder, 0);

      if (*decoder->currentChar == '}')
         break;
   }

   printf("END OBJECT\n");
   chckJsonDecoderAdvance(decoder, 0);
}

static void chckJsonDecoderDecodeArray(chckJsonDecoder *decoder)
{
   if (*decoder->currentChar != '[') {
      chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_UNEXPECTED, "Expected opening '['");
      return;
   }

   printf("START ARRAY\n");

   while (chckJsonDecoderAdvance(decoder, 1)) {
      if (*decoder->currentChar == ']')
         break;

      chckJsonDecoderDecodeValue(decoder, 1);

      if (*decoder->currentChar == ']')
         break;
   }

   printf("END ARRAY\n");
   chckJsonDecoderAdvance(decoder, 0);
}

static void chckJsonDecoderDecodeTrue(chckJsonDecoder *decoder)
{
   if (memcmp(decoder->currentChar, "true", 4)) {
      chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_UNEXPECTED, "Expected 'true'");
      return;
   }

   chckJsonDecoderSkip(decoder, 4);
   printf("TRUE\n");
}

static void chckJsonDecoderDecodeFalse(chckJsonDecoder *decoder)
{
   if (memcmp(decoder->currentChar, "false", 5)) {
      chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_UNEXPECTED, "Expected 'false'");
      return;
   }

   chckJsonDecoderSkip(decoder, 5);
   printf("FALSE\n");
}

static void chckJsonDecoderDecodeNull(chckJsonDecoder *decoder)
{
   if (memcmp(decoder->currentChar, "null", 4)) {
      chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_UNEXPECTED, "Expected 'null'");
      return;
   }

   chckJsonDecoderSkip(decoder, 4);
   printf("NULL\n");
}

static void chckJsonDecoderDecodeNumber(chckJsonDecoder *decoder)
{
   char *string = NULL;
   size_t len, alloc;

   if (!isdigit(*decoder->currentChar) && *decoder->currentChar != '-' && *decoder->currentChar != '+' && *decoder->currentChar != '.') {
      chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_UNEXPECTED, "Expected digit, '-' or '+'");
      return;
   }

   do {
      if (!isdigit(*decoder->currentChar) && *decoder->currentChar != '-' && *decoder->currentChar != '+' &&
          *decoder->currentChar != 'e' && *decoder->currentChar != 'E' && *decoder->currentChar != '.') {
         break;
      }

      chckJsonDecoderStringInsert(decoder, &string, &len, &alloc);
   } while (chckJsonDecoderAdvance(decoder, 0));

   /* go back one char, since number might end on any token */
   decoder->currentChar -= 1;

   printf("NUMBER: %s\n", string);

   if (string)
      free(string);
}

static void chckJsonDecoderDecodeString(chckJsonDecoder *decoder)
{
   char *string = NULL;
   size_t len, alloc;

   if (*decoder->currentChar != '\"') {
      chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_UNEXPECTED, "Expected opening '\"'");
      return;
   }

   while (chckJsonDecoderAdvance(decoder, 0)) {
      if (*decoder->currentChar == '\"')
         break;

      chckJsonDecoderStringInsert(decoder, &string, &len, &alloc);
   }

   printf("STRING: %s\n", string);
   chckJsonDecoderAdvance(decoder, 1);

   if (string)
      free(string);
}

static void chckJsonDecoderDecodeComment(chckJsonDecoder *decoder)
{
   if (!decoder->allowComments) {
      chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_COMMENTS_DISABLED, "Comment decoding is disabled in decoder. Comments are not part of JSON standard.");
      return;
   }

   if (*decoder->currentChar == '/' && *(decoder->currentChar + 1) != '/' && *(decoder->currentChar + 1) != '*') {
      chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_UNEXPECTED, "Expected '/' or '*' after opening '/'");
      return;
   }

   unsigned int commentLine = decoder->currentLine;
   int cStyleComment = (*(decoder->currentChar + 1) == '*');
   while (chckJsonDecoderAdvance(decoder, 1)) {
      if (cStyleComment) {
         if (*decoder->currentChar == '/' && *(decoder->currentChar - 1) == '*')
            break;
      } else {
         if (decoder->currentLine > commentLine)
            break;
      }
   }

   printf("%sCOMMENT\n", (cStyleComment ? "C STYLE " : ""));
}

static void chckJsonDecoderDecodeValue(chckJsonDecoder *decoder, int isProperty)
{
   switch (*decoder->currentChar) {
      case ',':
         return;
      case '\"':
         chckJsonDecoderDecodeString(decoder);
         return;
      case '{':
         chckJsonDecoderDecodeObject(decoder);
         return;
      case '[':
         chckJsonDecoderDecodeArray(decoder);
         return;
      case 't':
         chckJsonDecoderDecodeTrue(decoder);
         return;
      case 'f':
         chckJsonDecoderDecodeFalse(decoder);
         return;
      case 'n':
         chckJsonDecoderDecodeNull(decoder);
         return;
      case '+':
      case '-':
      case '.':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
         chckJsonDecoderDecodeNumber(decoder);
         return;
      case '/':
         chckJsonDecoderDecodeComment(decoder);
         return;
      default:break;
   }

   if (!isProperty) {
      switch (*decoder->currentChar) {
         case ':':
            chckJsonDecoderAdvance(decoder, 1);
            chckJsonDecoderDecodeValue(decoder, 1);
            return;
         default:break;
      }
   }

   chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_UNEXPECTED, "Unexpected token '%c'", *decoder->currentChar);
}

chckJson* chckJsonDecoderDecode(chckJsonDecoder *decoder, const char *data)
{
   assert(decoder && data);

   decoder->lineStart = decoder->currentChar = data;
   decoder->currentLine = 1;

   if (!*decoder->currentChar)
      return NULL;

   do {
      chckJsonDecoderDecodeValue(decoder, 0);
   } while (chckJsonDecoderAdvance(decoder, 1));

   return NULL;
}

chckJsonDecoder* chckJsonDecoderNew(void)
{
   chckJsonDecoder *decoder;

   if (!(decoder = calloc(1, sizeof(chckJsonDecoder))))
      goto fail;

   return decoder;

fail:
   return NULL;
}

void chckJsonDecoderFree(chckJsonDecoder *decoder)
{
   assert(decoder);
   free(decoder);
}

void chckJsonDecoderUserdata(chckJsonDecoder *decoder, void *userdata)
{
   assert(decoder);
   decoder->userdata = userdata;
}

void* chckJsonDecoderGetUserdata(chckJsonDecoder *decoder)
{
   assert(decoder);
   return decoder->userdata;
}

void chckJsonDecoderAllowComments(chckJsonDecoder *decoder, int allowComments)
{
   assert(decoder);
   decoder->allowComments = allowComments;
}

int chckJsonDecoderGetAllowComments(chckJsonDecoder *decoder)
{
   assert(decoder);
   return decoder->allowComments;
}

void chckJsonDecoderErrorCallback(chckJsonDecoder *decoder, chckJsonErrorCallback callback)
{
   assert(decoder);
   decoder->callback = callback;
}

/* vim: set ts=8 sw=3 tw=0 :*/
