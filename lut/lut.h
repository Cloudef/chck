#ifndef __chck_lut__
#define __chck_lut__

#ifndef size_t
#  include <stddef.h> /* for size_t */
#endif

typedef struct _chckLut chckLut;
typedef struct _chckHashTable chckHashTable;

/**
 * LUTs are manual lookup tables for your data.
 * LUTs won't handle hash collisions at all, and stores the data in fixed size pool, thus references are copied.
 */

#define chckLutIterCall(lut, function, ...) \
{ size_t i; void *p; for (i = 0; (p = chckLutIter(lut, &i));) function(p, ##__VA_ARGS__); }

chckLut* chckLutNew(size_t size, int set, size_t member);
void chckLutFree(chckLut *lut);
void chckLutFlush(chckLut *lut);
void* chckLutGet(chckLut *lut, unsigned int lookup);
void* chckLutStrGet(chckLut *lut, const char *str);
void chckLutSet(chckLut *lut, unsigned int lookup, const void *data);
void chckLutStrSet(chckLut *lut, const char *str, const void *data);
void* chckLutIter(chckLut *lut, size_t *iter);

/**
 * HashTables are wrappers around LUTs.
 * Collisions are avoided with the Set/Get functions, however StrSet/StrGet may collide since the key for those is hash.
 * Due to simplicity of implementation, you can not iterate HashTable.
 */

chckHashTable* chckHashTableNew(size_t size);
void chckHashTableFree(chckHashTable *table);
void chckHashTableFlush(chckHashTable *table);
int chckHashTableSet(chckHashTable *table, unsigned int key, const void *data, size_t member);
int chckHashTableStrSet(chckHashTable *table, const char *str, const void *data, size_t member);
void* chckHashTableGet(chckHashTable *table, unsigned int key);
void* chckHashTableStrGet(chckHashTable *table, const char *str);

#endif /* __chck_lut__ */

/* vim: set ts=8 sw=3 tw=0 :*/
