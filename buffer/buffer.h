#ifndef __chck_buffer__
#define __chck_buffer__

#ifndef size_t
#  include <stddef.h> /* for size_t */
#endif

typedef enum chckBufferEndianType {
   CHCK_BUFFER_ENDIAN_LITTLE = 0,
   CHCK_BUFFER_ENDIAN_BIG    = 1,
   CHCK_BUFFER_ENDIAN_NATIVE = 2,
} chckBufferEndianType;

typedef struct chckBuffer {
   size_t size;
   void *buffer, *curpos;
   char endianess;
   char freeBuffer;
} chckBuffer;

chckBuffer* chckBufferNewFromPointer(const void *ptr, size_t size, chckBufferEndianType endianess);
chckBuffer* chckBufferNew(size_t size, chckBufferEndianType endianess);
void chckBufferFree(chckBuffer *buf);
int chckBufferIsNativeEndian(chckBuffer *buf);
size_t chckBufferRead(void *dst, size_t size, size_t memb, chckBuffer *buf);
int chckBufferReadUInt8(chckBuffer *buf, unsigned char *i);
int chckBufferReadInt8(chckBuffer *buf, char *i);
int chckBufferReadUInt16(chckBuffer *buf, unsigned short *i);
int chckBufferReadInt16(chckBuffer *buf, short *i);
int chckBufferReadUInt32(chckBuffer *buf, unsigned int *i);
int chckBufferReadInt32(chckBuffer *buf, int *i);
int chckBufferReadString(chckBuffer *buf, size_t bytes, char **str);

#endif /* __chck_buffer__ */

/* vim: set ts=8 sw=3 tw=0 :*/
