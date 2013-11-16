#ifndef __chck_buffer__
#define __chck_buffer__

#ifndef size_t
#  include <stddef.h> /* for size_t */
#endif

#ifndef FILE
#  include <stdio.h> /* for FILE */
#endif

typedef enum chckBufferEndianType {
   CHCK_BUFFER_ENDIAN_LITTLE = 0,
   CHCK_BUFFER_ENDIAN_BIG    = 1,
   CHCK_BUFFER_ENDIAN_NATIVE = 2,
} chckBufferEndianType;

typedef struct _chckBuffer chckBuffer;

int chckBufferIsBigEndian(void);
void chckBufferSwap(void *v, size_t size, size_t memb);

void chckBufferFree(chckBuffer *buf);
chckBuffer* chckBufferNewFromPointer(const void *ptr, size_t size, chckBufferEndianType endianess);
chckBuffer* chckBufferNew(size_t size, chckBufferEndianType endianess);
void chckBufferSetPointer(chckBuffer *buf, const void *ptr, size_t size, chckBufferEndianType endianess);
void* chckBufferGetPointer(chckBuffer *buf);
void* chckBufferGetOffsetPointer(chckBuffer *buf);
int chckBufferResize(chckBuffer *buf, size_t size);

void chckBufferSeek(chckBuffer *buf, long offset, int whence);
size_t chckBufferGetOffset(chckBuffer *buf);
size_t chckBufferGetSize(chckBuffer *buf);

int chckBufferIsNativeEndian(chckBuffer *buf);

size_t chckBufferFill(const void *src, size_t size, size_t memb, chckBuffer *buf);
size_t chckBufferFillFromFile(FILE *src, size_t, size_t memb, chckBuffer *buf);

size_t chckBufferRead(void *dst, size_t size, size_t memb, chckBuffer *buf);
int chckBufferReadUInt8(chckBuffer *buf, unsigned char *i);
int chckBufferReadInt8(chckBuffer *buf, char *i);
int chckBufferReadUInt16(chckBuffer *buf, unsigned short *i);
int chckBufferReadInt16(chckBuffer *buf, short *i);
int chckBufferReadUInt32(chckBuffer *buf, unsigned int *i);
int chckBufferReadInt32(chckBuffer *buf, int *i);
int chckBufferReadString(chckBuffer *buf, size_t bytes, char **str);

size_t chckBufferWrite(const void *src, size_t size, size_t nmemb, chckBuffer *buf);
size_t chckBufferWriteFromFile(FILE *src, size_t size, size_t nmemb, chckBuffer *buf);
int chckBufferWriteUInt8(chckBuffer *buf, unsigned char i);
int chckBufferWriteInt8(chckBuffer *buf, char i);
int chckBufferWriteUInt16(chckBuffer *buf, unsigned short i);
int chckBufferWriteInt16(chckBuffer *buf, short i);
int chckBufferWriteUInt32(chckBuffer *buf, unsigned int i);
int chckBufferWriteInt32(chckBuffer *buf, int i);
int chckBufferWriteString(chckBuffer *buf, size_t len, const char *str);

#endif /* __chck_buffer__ */

/* vim: set ts=8 sw=3 tw=0 :*/
