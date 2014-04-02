#ifndef __chck_json__
#define __chck_json__

#ifndef size_t
#  include <stddef.h> /* for size_t */
#endif

typedef enum chckJsonError {
   CHCK_JSON_ERROR_UNEXPECTED = 1,
   CHCK_JSON_ERROR_COMMENTS_DISABLED,
   CHCK_JSON_ERROR_OUT_OF_MEMORY,
} chckJsonError;

typedef enum chckJsonType {
   CHCK_JSON_TYPE_OBJECT, // chckJsonGetChild
   CHCK_JSON_TYPE_STRING, // chckJsonGetString
   CHCK_JSON_TYPE_NUMBER, // chckJsonGetString (bigint etc...), chckJsonGetLong, chckJsonGetDouble
   CHCK_JSON_TYPE_ARRAY, // chckJsonGetChild
   CHCK_JSON_TYPE_BOOL, // chckJsonGetLong (1 == true, 0 == false)
   CHCK_JSON_TYPE_NULL // no-op
} chckJsonType;

typedef struct _chckJson chckJson;
typedef struct _chckJsonDecoder chckJsonDecoder;

typedef void (*chckJsonErrorCallback)(chckJsonDecoder *decoder, unsigned int line, unsigned int position, const char *linePtr, chckJsonError code, const char *message);

chckJsonDecoder* chckJsonDecoderNew(void);
void chckJsonDecoderFree(chckJsonDecoder *decoder);
void chckJsonDecoderUserdata(chckJsonDecoder *decoder, void *userdata);
void* chckJsonDecoderGetUserdata(chckJsonDecoder *decoder);
void chckJsonDecoderAllowComments(chckJsonDecoder *decoder, int allowComments);
int chckJsonDecoderGetAllowComments(chckJsonDecoder *decoder);
void chckJsonDecoderErrorCallback(chckJsonDecoder *decoder, chckJsonErrorCallback callback);
chckJson* chckJsonDecoderDecode(chckJsonDecoder *decoder, const char *data);

chckJson* chckJsonNew(chckJsonType type);
chckJson* chckJsonNewString(const char *str);
chckJson* chckJsonNewStringf(const char *fmt, ...);
chckJson* chckJsonNewNumberLong(long jlong);
chckJson* chckJsonNewNumberDouble(double jdouble);
chckJson* chckJsonNewNumberf(const char *fmt, ...);
chckJson* chckJsonNewBool(unsigned char boolean);
chckJson* chckJsonCopy(const chckJson *json);
void chckJsonFree(chckJson *json);
void chckJsonFreeChilds(chckJson *json);
void chckJsonFreeAll(chckJson *json);
chckJsonType chckJsonGetType(chckJson *json);
void chckJsonNext(chckJson *json, chckJson *next);
chckJson* chckJsonGetNext(chckJson *json);
chckJson* chckJsonGetNextAt(chckJson *json, unsigned int idx);
void chckJsonChild(chckJson *json, chckJson *child);
chckJson* chckJsonGetChild(chckJson *json, unsigned int *numChilds);
chckJson* chckJsonChildPop(chckJson *json, unsigned int idx);
chckJson* chckJsonGetChildAt(chckJson *json, unsigned int idx);
void chckJsonChildPush(chckJson *json, unsigned int idx, chckJson *child);
void chckJsonChildAppend(chckJson *json, chckJson *child);
void chckJsonProperty(chckJson *json, const char* name, chckJson *value);
chckJson* chckJsonGetProperty(chckJson *json, const char* name);
void chckJsonString(chckJson *json, const char *str);
const char* chckJsonGetString(chckJson *json);
void chckJsonStringf(chckJson *json, const char *fmt, ...);
void chckJsonLong(chckJson *json, long jlong);
long chckJsonGetLong(chckJson *json);
void chckJsonDouble(chckJson *json, double jdouble);
double chckJsonGetDouble(chckJson *json);
char* chckJsonEncode(chckJson *json, size_t *outSize);

#endif /* __chck_json__ */

/* vim: set ts=8 sw=3 tw=0 :*/
