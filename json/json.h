#ifndef __chck_json__
#define __chck_json__

typedef enum chckJsonError {
   CHCK_JSON_ERROR_UNEXPECTED = 1,
   CHCK_JSON_ERROR_COMMENTS_DISABLED,
   CHCK_JSON_ERROR_OUT_OF_MEMORY,
} chckJsonError;

typedef enum chckJsonType {
   CHCK_JSON_TYPE_OBJECT,
   CHCK_JSON_TYPE_STRING,
   CHCK_JSON_TYPE_NUMBER,
   CHCK_JSON_TYPE_ARRAY,
   CHCK_JSON_TYPE_BOOL,
   CHCK_JSON_TYPE_NULL
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

#endif /* __chck_json__ */

/* vim: set ts=8 sw=3 tw=0 :*/
