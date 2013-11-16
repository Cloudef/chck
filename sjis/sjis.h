#ifndef __sjis_h__
#define __sjis_h__

#ifndef size_t
#  include <stddef.h> /* for size_t */
#endif

char* chckSJISToUTF8(const unsigned char *sjis, size_t size, size_t *outSize, int terminate);
unsigned char* chckUTF8ToSJIS(const char *input, size_t size, size_t *outSize, int terminate);

#endif /* __sjis_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
