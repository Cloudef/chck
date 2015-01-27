#ifndef __sjis_h__
#define __sjis_h__

#include <stddef.h>
#include <stdbool.h>

char* chck_sjis_to_utf8(const unsigned char *sjis, size_t size, size_t *out_size, bool terminate);
unsigned char* chck_utf8_to_sjis(const char *input, size_t size, size_t *out_size, bool terminate);

#endif /* __sjis_h__ */
