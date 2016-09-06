#ifndef __chck_lut__
#define __chck_lut__

#include <chck/macros.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

struct chck_lut {
   uint8_t *table;

   // count and member size (lut size == count * member)
   size_t count, member;

   // the value lut was initialized with (memset(table, set, count * member))
   int set;

   // pointers to hash functions
   uint32_t (*hashuint)(uint32_t uint);
   uint32_t (*hashstr)(const char *str, size_t len);
};

struct chck_hash_table {
   struct chck_lut lut;
   struct chck_lut meta;

   // if there was collision, next table is created
   struct chck_hash_table *next;
};

struct chck_hash_table_iterator {
   struct chck_hash_table *table;
   size_t iter;
   const char *str_key;
   uint32_t uint_key;
};

// simply return the input, this is good for incrementing numbers
// or, when you know that the input will be somewhere around range of your hash table's size
CHCK_CONST static inline uint32_t
chck_incremental_uint_hash(uint32_t uint)
{
   return uint;
}

// default simple hash from <http://stackoverflow.com/a/12996028>
CHCK_CONST static inline uint32_t
chck_default_uint_hash(uint32_t uint)
{
   uint = ((uint >> 16) ^ uint) * 0x45d9f3b;
   uint = ((uint >> 16) ^ uint) * 0x45d9f3b;
   return ((uint >> 16) ^ uint);
}

// default simple string hash
CHCK_CONST static inline uint32_t
chck_default_str_hash(const char *str, size_t len)
{
   (void)len;
   int32_t c;
   uint32_t hash = 5381;
   while ((c = *str++)) hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
   return hash;
}

/**
 * LUTs are manual lookup tables for your data.
 * Iterating LUT may not be effecient operation depending on the size of the lut.
 *
 * LUTs won't handle hash collisions at all, and stores the data in fixed size pool, thus references are copied.
 * This means, when collision happen, new data is copied over the intersecting data.
 * Thus you should not store anything allocated in luts (unless you can free the memory otherwise).
 */

#define chck_lut_for_each_call(lut, function, ...) \
{ void *_P; for (size_t _I = 0; (_P = chck_lut_iter(lut, &_I));) function(_P, ##__VA_ARGS__); }

#define chck_lut_for_each(lut, pos) \
   for (size_t _I = 0; (pos = chck_lut_iter(lut, &_I));)

bool chck_lut(struct chck_lut *lut, int set, size_t count, size_t member);
void chck_lut_uint_algorithm(struct chck_lut *lut, uint32_t (*hashuint)(uint32_t uint));
void chck_lut_str_algorithm(struct chck_lut *lut, uint32_t (*hashstr)(const char *str, size_t len));
void chck_lut_release(struct chck_lut *lut);
void chck_lut_flush(struct chck_lut *lut);
bool chck_lut_set(struct chck_lut *lut, uint32_t lookup, const void *data);
void* chck_lut_get(struct chck_lut *lut, uint32_t lookup);
bool chck_lut_str_set(struct chck_lut *lut, const char *str, size_t len, const void *data);
void* chck_lut_str_get(struct chck_lut *lut, const char *str, size_t len);
void* chck_lut_iter(struct chck_lut *lut, size_t *iter);

/**
 * Hash tables are wrappers around LUTs that does not have collisions.
 * Iterating Hash table may not be a effecient operation depending on the size of the hash table.
 *
 * Hash table uses internally LUTs.
 * When collision occurs it will push a new layer of luts for intersected items.
 * Thus the effeciency of the hash table decreases the more collisions/redirects there is.
 */

#define chck_hash_table_for_each_call(table, function, ...) \
{ struct chck_hash_table_iterator _I = { table, 0, NULL, 0 }; void *_P; while ((_P = chck_hash_table_iter(&_I))) function(_P, ##__VA_ARGS__); }

#define chck_hash_table_for_each(table, pos) \
   for (struct chck_hash_table_iterator _I = { table, 0, NULL, 0 }; (pos = chck_hash_table_iter(&_I));)

bool chck_hash_table(struct chck_hash_table *table, int set, size_t count, size_t member);
void chck_hash_table_uint_algorithm(struct chck_hash_table *table, uint32_t (*hashuint)(uint32_t uint));
void chck_hash_table_str_algorithm(struct chck_hash_table *table, uint32_t (*hashstr)(const char *str, size_t len));
void chck_hash_table_release(struct chck_hash_table *table);
void chck_hash_table_flush(struct chck_hash_table *table);
uint32_t chck_hash_table_collisions(struct chck_hash_table *table);
bool chck_hash_table_set(struct chck_hash_table *table, uint32_t key, const void *data);
void* chck_hash_table_get(struct chck_hash_table *table, uint32_t key);
bool chck_hash_table_str_set(struct chck_hash_table *table, const char *str, size_t len, const void *data);
void* chck_hash_table_str_get(struct chck_hash_table *table, const char *str, size_t len);
void* chck_hash_table_iter(struct chck_hash_table_iterator *iter);

#endif /* __chck_lut__ */
