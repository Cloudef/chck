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
      unsigned char boolean;
   };
   struct _chckJson *child;
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

static chckJson* chckJsonDecoderDecodeValue(chckJsonDecoder *decoder, chckJson *parent, int isProperty);

static int chckResizeBuf(unsigned char **buf, size_t *size, size_t nsize)
{
   void *tmp;
   assert(buf && size);

   if (nsize < *size || !(tmp = realloc(*buf, nsize))) {
      if (!(tmp = malloc(nsize))) return RETURN_FAIL;
      memcpy(tmp, *buf, (nsize > *size ? *size : nsize));
      free(*buf);
   }

   *buf = tmp;
   *size = nsize;
   return RETURN_OK;
}

static int chckPutBuf(unsigned char **buf, size_t *o, size_t *size, const char *bytes)
{
   assert(buf && o && size && bytes);
   size_t len = strlen(bytes);
   assert(len > 0);

   if (len > *size - *o && chckResizeBuf(buf, size, *size * 2) != RETURN_OK)
      return RETURN_FAIL;

   memcpy(*buf + *o, bytes, len);
   *o += len;
   return RETURN_OK;
}

static void chckPutBufEscaped(unsigned char **enc, size_t *e, size_t *esize, const char *s) {
   for (; *s; ++s) {
      if (*s == '\"') chckPutBuf(enc, e, esize, "\\\"");
      else if (*s == '\\') chckPutBuf(enc, e, esize, "\\\\");
      else if (*s == '\b') chckPutBuf(enc, e, esize, "\\b");
      else if (*s == '\f') chckPutBuf(enc, e, esize, "\\f");
      else if (*s == '\n') chckPutBuf(enc, e, esize, "\\n");
      else if (*s == '\r') chckPutBuf(enc, e, esize, "\\r");
      else if (*s == '\t') chckPutBuf(enc, e, esize, "\\t");
      else chckPutBuf(enc, e, esize, (const char[]){*s, 0});
   }
}

static void chckJsonDecoderThrow(chckJsonDecoder *decoder, chckJsonError code, const char *fmt, ...)
{
   size_t i;
   va_list args;
   char message[128], line[128];
   assert(decoder && fmt);

   if (!decoder->callback)
      return;

   memset(message, 0, sizeof(message));
   memset(line, 0, sizeof(line));

   va_start(args, fmt);
   vsnprintf(message, sizeof(message) - 1, fmt, args);
   va_end(args);

   for (i = 0; i < sizeof(line) && decoder->lineStart[i] && decoder->lineStart[i] != '\n'; ++i)
      line[i] = decoder->lineStart[i];

   decoder->callback(decoder, decoder->currentLine, decoder->currentChar - decoder->lineStart + 1, line, code, message);
}

static char chckJsonDecoderSkip(chckJsonDecoder *decoder, size_t numChars)
{
   size_t i;
   assert(decoder && numChars > 0);
   for (i = 0; i < numChars && *decoder->currentChar; ++decoder->currentChar, ++i);
   return *decoder->currentChar;
}

static char chckJsonDecoderAdvance(chckJsonDecoder *decoder, int skipWhitespace)
{
   assert(decoder);

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
   assert(decoder);

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
      if ((string = malloc((alloc = 128)))) {
         len = 0;
      } else {
         len = alloc = 0;
         chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_OUT_OF_MEMORY, "Cannot allocate string buffer");
      }
   }

   if (alloc && len + chrSize >= alloc - 1) {
      if (chckResizeBuf((unsigned char**)&string, &alloc, alloc * 2) != RETURN_OK)
         chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_OUT_OF_MEMORY, "Cannot grow string buffer");
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

static void chckJsonDecoderDecodeObject(chckJsonDecoder *decoder, chckJson *json)
{
   chckJson *parent = json;
   assert(*decoder->currentChar == '{');

   while (chckJsonDecoderAdvance(decoder, 1)) {
      if (*decoder->currentChar == '}')
         break;

      parent = chckJsonDecoderDecodeValue(decoder, parent, (parent == json));

      if (*decoder->currentChar == '}')
         break;
   }

   if (*decoder->currentChar != '}')
      chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_UNEXPECTED, "Expected '}' before end of data");

   chckJsonDecoderAdvance(decoder, 0);
}

static void chckJsonDecoderDecodeArray(chckJsonDecoder *decoder, chckJson *json)
{
   chckJson *parent = json;
   assert(*decoder->currentChar == '[');

   while (chckJsonDecoderAdvance(decoder, 1)) {
      if (*decoder->currentChar == ']')
         break;

      parent = chckJsonDecoderDecodeValue(decoder, parent, (parent == json));

      if (*decoder->currentChar == ']')
         break;
   }

   if (*decoder->currentChar != ']')
      chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_UNEXPECTED, "Expected ']' before end of data");

   chckJsonDecoderAdvance(decoder, 0);
}

static void chckJsonDecoderDecodeBool(chckJsonDecoder *decoder, chckJson *json)
{
   assert(memcmp(decoder->currentChar, "true", 4) == 0 || memcmp(decoder->currentChar, "false", 5) == 0);
   json->boolean = (memcmp(decoder->currentChar, "true", 4) == 0);
   chckJsonDecoderSkip(decoder, (json->boolean ? 4 : 5));
}

static void chckJsonDecoderDecodeNull(chckJsonDecoder *decoder, chckJson *json)
{
   (void)json;
   assert(memcmp(decoder->currentChar, "null", 4) == 0);
   chckJsonDecoderSkip(decoder, 4);
}

static void chckJsonDecoderDecodeNumber(chckJsonDecoder *decoder, chckJson *json)
{
   char *string = NULL;
   size_t len = 0, alloc;
   assert(isdigit(*decoder->currentChar) || *decoder->currentChar == '-' || *decoder->currentChar == '+' || *decoder->currentChar == '.');

   do {
      if (!isdigit(*decoder->currentChar) && *decoder->currentChar != '-' && *decoder->currentChar != '+' &&
          *decoder->currentChar != 'e' && *decoder->currentChar != 'E' && *decoder->currentChar != '.') {
         break;
      }

      chckJsonDecoderStringInsert(decoder, &string, &len, &alloc);
   } while (chckJsonDecoderAdvance(decoder, 0));

   if (len > 0) {
      chckResizeBuf((unsigned char**)&string, &alloc, len + 1);
      json->string = string;
      string[len] = 0;
   } else {
      free(string);
   }
}

static void chckJsonDecoderDecodeString(chckJsonDecoder *decoder, chckJson *json)
{
   char *string = NULL;
   size_t len = 0, alloc;
   assert(json && *decoder->currentChar == '\"');

   while (chckJsonDecoderAdvance(decoder, 0)) {
      if (*decoder->currentChar == '\"')
         break;

      chckJsonDecoderStringInsert(decoder, &string, &len, &alloc);
   }

   if (*decoder->currentChar != '\"')
      chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_UNEXPECTED, "Expected '\"' before end of data");

   if (len > 0) {
      chckResizeBuf((unsigned char**)&string, &alloc, len + 1);
      json->string = string;
      string[len] = 0;
   } else {
      free(string);
   }
}

static void chckJsonDecoderDecodeComment(chckJsonDecoder *decoder, chckJson *json)
{
   (void)json;

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

   if (cStyleComment && *decoder->currentChar != '/')
      chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_UNEXPECTED, "Expected '*/' before end of data");
}

static chckJson* chckJsonDecoderDecodeValue(chckJsonDecoder *decoder, chckJson *parent, int isContainer)
{
   int i, j;
   chckJson *json = NULL;
   int found = 0;

   if (parent && *decoder->currentChar == ':') {
      chckJsonDecoderAdvance(decoder, 1);
      chckJsonDecoderDecodeValue(decoder, parent, 1);
      return parent;
   }

   const struct jsonDecodeEntry {
      int type;
      const char **except;
      void (*function)(chckJsonDecoder *decoder, chckJson *json);
   } *decodeTable = (const struct jsonDecodeEntry[]){
         { -1, (const char*[]){ ",", NULL }, NULL },
         { CHCK_JSON_TYPE_STRING, (const char*[]){ "\"", NULL }, chckJsonDecoderDecodeString },
         { CHCK_JSON_TYPE_OBJECT, (const char*[]){ "{", NULL }, chckJsonDecoderDecodeObject },
         { CHCK_JSON_TYPE_ARRAY, (const char*[]){ "[", NULL }, chckJsonDecoderDecodeArray },
         { CHCK_JSON_TYPE_BOOL, (const char*[]){ "true", "false", NULL }, chckJsonDecoderDecodeBool },
         { CHCK_JSON_TYPE_NULL, (const char*[]){ "null", NULL }, chckJsonDecoderDecodeNull },
         { -1, (const char*[]){ "/", NULL }, chckJsonDecoderDecodeComment },
         { CHCK_JSON_TYPE_NUMBER,
            (const char*[]){ "+", "-", ".", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", NULL },
            chckJsonDecoderDecodeNumber },
         { -1, NULL, NULL }
      };

   for (i = 0; !found && decodeTable[i].except; ++i) {
      for (j = 0; decodeTable[i].except[j]; ++j) {
         if (memcmp(decoder->currentChar, decodeTable[i].except[j], strlen(decodeTable[i].except[j])) == 0) {
            found = 1;
            break;
         }
      }

      if (!found)
         continue;

      if (!decodeTable[i].function)
         return parent;

      if (decodeTable[i].type != -1 && !(json = chckJsonNew(decodeTable[i].type))) {
         chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_OUT_OF_MEMORY, "Cannot allocate json object");
         return parent;
      }

      decodeTable[i].function(decoder, json);
   }

   if (!found) {
      chckJsonDecoderThrow(decoder, CHCK_JSON_ERROR_UNEXPECTED, "Unexpected token '%c'", *decoder->currentChar);
   } else if (parent && json) {
      if (isContainer) {
         chckJsonChild(parent, json);
      } else {
         chckJsonNext(parent, json);
      }
   }

   return (json ? json : parent);
}

chckJson* chckJsonDecoderDecode(chckJsonDecoder *decoder, const char *data)
{
   chckJson *parent = NULL;
   assert(decoder && data);

   decoder->lineStart = decoder->currentChar = data;
   decoder->currentLine = 1;
   decoder->json = NULL;

   if (!*decoder->currentChar)
      return NULL;

   do {
      parent = chckJsonDecoderDecodeValue(decoder, parent, 0);
      if (!decoder->json && parent) decoder->json = parent;
   } while (chckJsonDecoderAdvance(decoder, 1));

   return decoder->json;
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

static void _chckJsonStringf(chckJson *json, const char *fmt, va_list args)
{
   va_list orig;
   char *buf = NULL;
   size_t len;
   assert(json);

   if (json->type != CHCK_JSON_TYPE_STRING &&
       json->type != CHCK_JSON_TYPE_NUMBER) {
      assert(0 && "JSON object type must be either string or number for this setter");
      return;
   }

   va_copy(orig, args);
   len = vsnprintf(NULL, 0, fmt, args) + 1;
   if (len > 0 && (buf = calloc(1, len + 1)))
      vsnprintf(buf, len, fmt, orig);

   if (json->string)
      free(json->string);
   json->string = buf;
}

static void chckJsonSerialize(chckJson *json, unsigned char **enc, size_t *e, size_t *esize)
{
   if (json->type == CHCK_JSON_TYPE_OBJECT) {
      chckPutBuf(enc, e, esize, "{");
   } else if (json->type == CHCK_JSON_TYPE_ARRAY) {
      chckPutBuf(enc, e, esize, "[");
   } else if (json->type == CHCK_JSON_TYPE_NULL) {
      chckPutBuf(enc, e, esize, "null");
   } else if (json->type == CHCK_JSON_TYPE_BOOL) {
      chckPutBuf(enc, e, esize, (json->boolean ? "true" : "false"));
   } else if (json->type == CHCK_JSON_TYPE_STRING) {
      chckPutBuf(enc, e, esize, "\"");
      if (json->string)
         chckPutBufEscaped(enc, e, esize, json->string);
      chckPutBuf(enc, e, esize, "\"");
   } else if (json->type == CHCK_JSON_TYPE_NUMBER) {
      chckPutBufEscaped(enc, e, esize, (json->string ? json->string : "0"));
   }

   if (json->child) {
      if (json->type != CHCK_JSON_TYPE_OBJECT && json->type != CHCK_JSON_TYPE_ARRAY)
         chckPutBuf(enc, e, esize, ":");
      chckJsonSerialize(json->child, enc, e, esize);
   }

   if (json->type == CHCK_JSON_TYPE_OBJECT) {
      chckPutBuf(enc, e, esize, "}");
   } else if (json->type == CHCK_JSON_TYPE_ARRAY) {
      chckPutBuf(enc, e, esize, "]");
   }

   if (json->next) {
      chckPutBuf(enc, e, esize, ",");
      chckJsonSerialize(json->next, enc, e, esize);
   }
}

char* chckJsonEncode(chckJson *json, size_t *outSize)
{
   unsigned char *enc;
   size_t e = 0, esize = 128;
   assert(json);

   if (outSize) *outSize = 0;

   if (!(enc = malloc(esize)))
      return NULL;

   chckJsonSerialize(json, &enc, &e, &esize);

   if (e == 0) {
      free(enc);
      return NULL;
   }

   chckResizeBuf(&enc, &esize, e + 1);
   enc[e] = 0;

   if (outSize) *outSize = esize;
   return (char*)enc;
}

chckJson* chckJsonNew(chckJsonType type)
{
   chckJson *json;

   if (!(json = calloc(1, sizeof(chckJson))))
      return NULL;

   json->type = type;
   return json;
}

chckJson* chckJsonNewString(const char *str)
{
   chckJson *json;

   if (!(json = chckJsonNew(CHCK_JSON_TYPE_STRING)))
      return NULL;

   chckJsonString(json, str);
   return json;
}

chckJson* chckJsonNewStringf(const char *fmt, ...)
{
   chckJson *json;
   va_list args;

   if (!(json = chckJsonNew(CHCK_JSON_TYPE_STRING)))
      return NULL;

   va_start(args, fmt);
   _chckJsonStringf(json, fmt, args);
   va_end(args);
   return json;
}

chckJson* chckJsonNewNumberLong(long jlong)
{
   chckJson *json;

   if (!(json = chckJsonNew(CHCK_JSON_TYPE_NUMBER)))
      return NULL;

   chckJsonLong(json, jlong);
   return json;
}

chckJson* chckJsonNewNumberDouble(double jdouble)
{
   chckJson *json;

   if (!(json = chckJsonNew(CHCK_JSON_TYPE_NUMBER)))
      return NULL;

   chckJsonDouble(json, jdouble);
   return json;
}

chckJson* chckJsonNewNumberf(const char *fmt, ...)
{
   chckJson *json;
   va_list args;

   if (!(json = chckJsonNew(CHCK_JSON_TYPE_NUMBER)))
      return NULL;

   va_start(args, fmt);
   _chckJsonStringf(json, fmt, args);
   va_end(args);
   return json;
}

chckJson* chckJsonNewBool(unsigned char boolean)
{
   chckJson *json;

   if (!(json = chckJsonNew(CHCK_JSON_TYPE_BOOL)))
      return NULL;

   chckJsonLong(json, boolean);
   return json;
}

chckJson* chckJsonCopy(const chckJson *json)
{
   chckJson *copy;

   switch (json->type) {
      case CHCK_JSON_TYPE_BOOL:
         copy = chckJsonNewBool(json->boolean);
         break;

      case CHCK_JSON_TYPE_NULL:
         copy = chckJsonNew(CHCK_JSON_TYPE_NULL);
         break;

      case CHCK_JSON_TYPE_NUMBER:
      case CHCK_JSON_TYPE_STRING:
         copy = chckJsonNew(json->type);
         chckJsonString(copy, json->string);
         break;

      case CHCK_JSON_TYPE_ARRAY:
      case CHCK_JSON_TYPE_OBJECT:
         copy = chckJsonNew(json->type);
         break;

      default:
         assert(0 && "Unknown JSON type");
         break;
   }

   if (json->child)
      copy->child = chckJsonCopy(json->child);

   if (json->next)
      copy->next = chckJsonCopy(json->next);

   return copy;
}

void chckJsonFree(chckJson *json)
{
   assert(json);

   if ((json->type == CHCK_JSON_TYPE_STRING || json->type == CHCK_JSON_TYPE_NUMBER) && json->string)
      free(json->string);

   free(json);
}

void chckJsonFreeAll(chckJson *json)
{
   assert(json);

   if (json->child)
      chckJsonFreeAll(json->child);

   if (json->next)
      chckJsonFreeAll(json->next);

   chckJsonFree(json);
}

chckJsonType chckJsonGetType(chckJson *json)
{
   assert(json);
   return json->type;
}

void chckJsonNext(chckJson *json, chckJson *next)
{
   assert(json);
   json->next = next;
}

chckJson* chckJsonGetNext(chckJson *json)
{
   assert(json);
   return json->next;
}

chckJson* chckJsonGetNextAt(chckJson *json, unsigned int idx)
{
   unsigned int i;
   chckJson *j = NULL;
   assert(json);

   for (i = 0, j = json; j && i < idx; j = j->next, ++i);
   return j;
}

void chckJsonChild(chckJson *json, chckJson *child)
{
   assert(json);
   json->child = child;
}

chckJson* chckJsonGetChild(chckJson *json, unsigned int *numChilds)
{
   unsigned int i;
   chckJson *j;
   assert(json);

   if (numChilds) {
      for (i = 0, j = json->child; j; j = j->next, ++i);
      *numChilds = i;
   }

   return json->child;
}

chckJson* chckJsonChildPop(chckJson *json, unsigned int idx)
{
   unsigned int i;
   chckJson *j, *p = NULL;
   assert(json);

   for (i = 0, j = json->child; j && i < idx; p = j, j = j->next, ++i);
   if (p && j) p->next = j->next;
   else if (!p && j) json->child = j->next;
   return j;
}

chckJson* chckJsonGetChildAt(chckJson* json, unsigned int idx)
{
   return chckJsonGetNextAt(json->child, idx);
}

void chckJsonChildPush(chckJson *json, unsigned int idx, chckJson *child)
{
   unsigned int i;
   chckJson *j, *p;
   assert(json);

   for (i = 0, j = json->child; j && i < idx; p = j, j = j->next, ++i);
   if (p && j) p->next = child;
   else if (!p && j) json->child = child;
   child->next = j;
}

void chckJsonChildAppend(chckJson *json, chckJson *child)
{
   chckJson **j;
   for(j = &json->child; *j; j = &(*j)->next);
   *j = child;
}

void chckJsonProperty(chckJson *json, const char* name, chckJson *value)
{
   chckJson** j;
   for(j = &json->child; *j && strcmp(name, (*j)->string); j = &(*j)->next);
   if(*j) chckJsonFreeAll((*j)->child);
   else *j = chckJsonNewString(name);
   (*j)->child = value;
}

chckJson* chckJsonGetProperty(chckJson *json, const char* name)
{
   chckJson** j;
   for(j = &json->child; *j && strcmp(name, (*j)->string); j = &(*j)->next);
   return *j ? (*j)->child : NULL;
}

void chckJsonString(chckJson *json, const char *str)
{
   assert(json);
   chckJsonStringf(json, "%s", str);
}

void chckJsonStringf(chckJson *json, const char *fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   _chckJsonStringf(json, fmt, args);
   va_end(args);
}

const char* chckJsonGetString(chckJson *json)
{
   assert(json);

   if (json->type != CHCK_JSON_TYPE_STRING &&
       json->type != CHCK_JSON_TYPE_NUMBER) {
      assert(0 && "JSON object type must be either string or number for this getter");
      return NULL;
   }

   return json->string;
}

void chckJsonLong(chckJson *json, long jlong)
{
   assert(json);

   if (json->type == CHCK_JSON_TYPE_BOOL) {
      json->boolean = jlong;
   } else {
      chckJsonStringf(json, "%ld", jlong);
   }
}

long chckJsonGetLong(chckJson *json)
{
   const char *buf;
   assert(json);

   if (json->type == CHCK_JSON_TYPE_BOOL)
      return json->boolean;

   if (!(buf = chckJsonGetString(json)))
      return 0;

   return strtol(buf, NULL, 10);
}

void chckJsonDouble(chckJson *json, double jdouble)
{
   assert(json);

   if (json->type == CHCK_JSON_TYPE_BOOL) {
      json->boolean = (unsigned char)jdouble;
   } else {
      chckJsonStringf(json, "%f", jdouble);
   }
}

double chckJsonGetDouble(chckJson *json)
{
   const char *buf;
   assert(json);

   if (json->type == CHCK_JSON_TYPE_BOOL)
      return (double)json->boolean;

   if (!(buf = chckJsonGetString(json)))
      return 0.0;

   return strtod(buf, NULL);
}

/* vim: set ts=8 sw=3 tw=0 :*/
