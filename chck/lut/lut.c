#include "lut.h"
#include "overflow/overflow.h"
#include <stdlib.h> /* for calloc, free, etc.. */
#include <string.h> /* for memcpy/memset */
#include <assert.h> /* for assert */

static inline char*
ccopy(const char *str)
{
   assert(str);
   size_t size = strlen(str);
   char *cpy = chck_calloc_add_of(size, 1);
   return (cpy ? memcpy(cpy, str, size) : NULL);
}

static inline bool
lut_create_table(struct chck_lut *lut)
{
   assert(lut);

   if (!(lut->table = chck_malloc_mul_of(lut->count, lut->member)))
      return false;

   memset(lut->table, lut->set, lut->count * lut->member);
   return true;
}

static inline void*
lut_get_index(struct chck_lut *lut, uint32_t index)
{
   assert(lut && index < lut->count);

   if (!lut->table && !lut_create_table(lut))
      return NULL;

   if (index >= lut->count)
      return NULL;

   return lut->table + index * lut->member;
}

static inline bool
lut_set_index(struct chck_lut *lut, uint32_t index, const void *data)
{
   assert(lut && index < lut->count);

   if (!lut->table && !lut_create_table(lut))
      return false;

   if (index >= lut->count)
      return NULL;

   if (data) {
      memcpy(lut->table + index * lut->member, data, lut->member);
   } else {
      memset(lut->table + index * lut->member, lut->set, lut->member);
   }

   return true;
}

bool
chck_lut(struct chck_lut *lut, int set, size_t count, size_t member)
{
   assert(lut && count > 0 && member > 0);

   if (!count || !member)
      return false;

   memset(lut, 0, sizeof(struct chck_lut));
   lut->set = set;
   lut->count = count;
   lut->member = member;
   lut->hashuint = chck_default_uint_hash;
   lut->hashstr = chck_default_str_hash;
   return true;
}

void
chck_lut_uint_algorithm(struct chck_lut *lut, uint32_t (*hashuint)(uint32_t uint))
{
   assert(lut && hashuint);
   lut->hashuint = hashuint;
}

void
chck_lut_str_algorithm(struct chck_lut *lut, uint32_t (*hashstr)(const char *str, size_t len))
{
   assert(lut && hashstr);
   lut->hashstr = hashstr;
}

void
chck_lut_flush(struct chck_lut *lut)
{
   assert(lut);

   if (lut->table)
      free(lut->table);

   lut->table = NULL;
}

void
chck_lut_release(struct chck_lut *lut)
{
   if (!lut)
      return;

   chck_lut_flush(lut);
   memset(lut, 0, sizeof(struct chck_lut));
}

bool
chck_lut_set(struct chck_lut *lut, uint32_t lookup, const void *data)
{
   assert(lut && lut->hashuint);
   return lut_set_index(lut, lut->hashuint(lookup) % lut->count, data);
}

void*
chck_lut_get(struct chck_lut *lut, uint32_t lookup)
{
   assert(lut && lut->hashuint);
   return lut_get_index(lut, lut->hashuint(lookup) % lut->count);
}

bool
chck_lut_str_set(struct chck_lut *lut, const char *str, size_t len, const void *data)
{
   assert(lut && lut->hashstr);
   return lut_set_index(lut, lut->hashstr(str, len) % lut->count, data);
}

void*
chck_lut_str_get(struct chck_lut *lut, const char *str, size_t len)
{
   assert(lut && lut->hashstr);
   return lut_get_index(lut, lut->hashstr(str, len) % lut->count);
}

void*
chck_lut_iter(struct chck_lut *lut, size_t *iter)
{
   assert(lut);

   if (*iter >= lut->count)
      return NULL;

   return lut->table + (*iter)++ * lut->member;
}

// metadata for resolving collisions
struct header {
   bool placed;
   char *str_key;
   uint32_t uint_key;
};

static bool
header(struct header *hdr, const char *str_key, uint32_t uint_key)
{
   void *str_copy = NULL;
   if (str_key && !(str_copy = ccopy(str_key))) {
      memset(hdr, 0, sizeof(struct header));
      return false;
   }

   hdr->placed = true;
   hdr->uint_key = uint_key;
   hdr->str_key = str_copy;
   return true;
}

static void
header_release(struct header *hdr)
{
   assert(hdr);

   if (hdr->str_key) {
      free(hdr->str_key);
      hdr->str_key = NULL;
   }

   hdr->placed = false;
}

static struct chck_hash_table*
next_table(struct chck_hash_table *table)
{
   assert(table);

   // create new table
   if (!(table->next = malloc(sizeof(struct chck_hash_table))))
      return false;

   if (!chck_hash_table(table->next, table->lut.set, table->lut.count, table->lut.member))
      goto fail;

   chck_hash_table_uint_algorithm(table->next, table->lut.hashuint);
   chck_hash_table_str_algorithm(table->next, table->lut.hashstr);
   return table->next;

fail:
   free(table->next);
   return (table->next = NULL);
}

static bool
hash_table_set(struct chck_hash_table *table, struct chck_hash_table *l, struct header *h, uint32_t index, const char *str_key, uint32_t uint_key, const void *data)
{
   // wanted to remove something that does not exist in hash table
   if (!table && !data)
      return true;

   // collision, we need new table
   if (!table && !(table = next_table(l)))
      return false;

   // release data of current header, if any in this slot
   if (h)
      header_release(h);

   // removal
   if (!data) {
      assert(lut_set_index(&table->meta, index, NULL));
      return lut_set_index(&table->lut, index, NULL);
   }

   struct header hdr;
   if (!header(&hdr, str_key, uint_key) || !lut_set_index(&table->meta, index, &hdr)) {
      header_release(&hdr);
      return false;
   }

   return lut_set_index(&table->lut, index, data);
}

static bool
hash_table_set_uint(struct chck_hash_table *table, uint32_t index, uint32_t key, const void *data)
{
   assert(table);

   struct header *h;
   struct chck_hash_table *t = table, *l;
   do {
      l = t;
      if (!(h = lut_get_index(&t->meta, index)) || !h->placed || (!h->str_key && h->uint_key == key))
         break;
      h = NULL; // Clear, in case we have collision. So we don't remove this header. (func: hash_table_set)
   } while ((t = t->next));

   return hash_table_set(t, l, h, index, NULL, key, data);
}

static bool
hash_table_set_str(struct chck_hash_table *table, uint32_t index, const char *key, const void *data)
{
   assert(table && key);

   struct header *h;
   struct chck_hash_table *t = table, *l;
   do {
      l = t;
      if (!(h = lut_get_index(&t->meta, index)) || !h->placed || (h->str_key && !strcmp(h->str_key, key)))
         break;
      h = NULL; // Clear, in case we have collision. So we don't remove this header. (func: hash_table_set)
   } while ((t = t->next));

   return hash_table_set(t, l, h, index, key, -1, data);
}

bool
chck_hash_table(struct chck_hash_table *table, int set, size_t count, size_t member)
{
   memset(table, 0, sizeof(struct chck_hash_table));

   if (!chck_lut(&table->lut, set, count, member))
      return false;

   if (!chck_lut(&table->meta, 0, count, sizeof(struct header)))
      goto fail;

   return true;

fail:
   chck_lut_release(&table->lut);
   return false;
}

void
chck_hash_table_uint_algorithm(struct chck_hash_table *table, uint32_t (*hashuint)(uint32_t uint))
{
   assert(table && hashuint);

   for (struct chck_hash_table *t = table; t; t = t->next) {
      chck_lut_uint_algorithm(&t->lut, hashuint);
      chck_lut_uint_algorithm(&t->meta, hashuint);
   }
}

void
chck_hash_table_str_algorithm(struct chck_hash_table *table, uint32_t (*hashstr)(const char *str, size_t len))
{
   assert(table && hashstr);

   for (struct chck_hash_table *t = table; t; t = t->next) {
      chck_lut_str_algorithm(&t->lut, hashstr);
      chck_lut_str_algorithm(&t->meta, hashstr);
   }
}

void
chck_hash_table_flush(struct chck_hash_table *table)
{
   assert(table);

   struct chck_hash_table *n;
   for (struct chck_hash_table *t = table; t; t = n) {
      n = t->next;

      // release all metadata headers (in case of string keys)
      struct header *hdr;
      chck_lut_for_each(&t->meta, hdr)
         header_release(hdr);

      chck_lut_flush(&t->lut);
      chck_lut_flush(&t->meta);

      if (t != table)
         free(t);
   }

   table->next = NULL;
}

void
chck_hash_table_release(struct chck_hash_table *table)
{
   if (!table)
      return;

   chck_hash_table_flush(table);
   memset(table, 0, sizeof(struct chck_hash_table));
}

uint32_t
chck_hash_table_collisions(struct chck_hash_table *table)
{
   assert(table);

   uint32_t collisions = 0;
   for (struct chck_hash_table *t = table->next; t; t = t->next) {
      struct header *hdr;
      chck_lut_for_each(&t->meta, hdr) {
         if (hdr->placed)
            ++collisions;
      }
   }

   return collisions;
}

bool
chck_hash_table_set(struct chck_hash_table *table, uint32_t key, const void *data)
{
   assert(table);
   return hash_table_set_uint(table, table->lut.hashuint(key) % table->lut.count, key, data);
}

void*
chck_hash_table_get(struct chck_hash_table *table, uint32_t key)
{
   assert(table);

   if (!table->lut.table)
      return NULL;

   void *data;
   struct header *h;
   struct chck_hash_table *t = table;
   do {
      data = chck_lut_get(&t->lut, key);
      h = chck_lut_get(&t->meta, key);
      t = t->next;

      if (h && !h->str_key && h->uint_key == key)
         return data;

      // check if this item is a intersection, if so cycle from another set of luts
   } while (t);

   return NULL;
}

bool
chck_hash_table_str_set(struct chck_hash_table *table, const char *str, size_t len, const void *data)
{
   assert(table && str);
   return hash_table_set_str(table, table->lut.hashstr(str, len) % table->lut.count, str, data);
}

void*
chck_hash_table_str_get(struct chck_hash_table *table, const char *str, size_t len)
{
   assert(table && str);

   if (!table->lut.table)
      return NULL;

   void *data;
   struct header *h;
   struct chck_hash_table *t = table;
   do {
      data = chck_lut_str_get(&t->lut, str, len);
      h = chck_lut_str_get(&t->meta, str, len);
      t = t->next;

      if (h && h->str_key && !strcmp(h->str_key, str))
         return data;

      // check if this item is a intersection, if so cycle from another set of luts
   } while (t);

   return NULL;
}

void*
chck_hash_table_iter(struct chck_hash_table_iterator *iterator)
{
   assert(iterator && iterator->table);

   if (iterator->iter >= iterator->table->lut.count) {
      if (!iterator->table->next)
         return NULL;

      // switch to another set of luts, since we have collisions
      iterator->table = iterator->table->next;
      iterator->iter = 0;
   }

   return chck_lut_iter(&iterator->table->lut, &iterator->iter);
}
