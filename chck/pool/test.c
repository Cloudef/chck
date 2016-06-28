#include "pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#undef NDEBUG
#include <assert.h>

struct item {
   uint32_t a;
   void *b;
};

static void printa(struct item *item)
{
   printf("item::%d\n", item->a);
}

int main(void)
{
   struct item dummy = {0};

   /* TEST: pool */
   {
      struct chck_pool pool = {0};

      {
         size_t iter = 0;
         assert(!chck_pool_iter(&pool, &iter, false));
         assert(!iter);
         assert(!chck_pool_iter(&pool, &iter, true));
         assert(!iter);
      }

      assert(chck_pool(&pool, 32, 3, sizeof(struct item)));
      assert(pool.items.step == 32 * sizeof(struct item));
      assert(pool.items.member == sizeof(struct item));
      assert(pool.items.allocated == 3 * sizeof(struct item));
      assert(pool.items.used == 0);
      assert(pool.items.count == 0);
      assert(pool.items.buffer);

      for (uint32_t i = 0; i < 3; ++i)
         assert(!chck_pool_get(&pool, i));

      assert(chck_pool_add(&pool, (&(struct item){1, NULL}), NULL));
      assert(chck_pool_add(&pool, (&(struct item){2, NULL}), NULL));
      chck_pool_remove(&pool, 0);
      assert(((struct item*)chck_pool_get(&pool, 1))->a == 2);
      chck_pool_flush(&pool);

      size_t a, b, c;
      assert(chck_pool_add(&pool, (&(struct item){1, NULL}), &a));
      assert(chck_pool_add(&pool, (&(struct item){2, NULL}), &b));
      assert(chck_pool_add(&pool, (&(struct item){3, NULL}), &c));
      assert(memcmp(chck_pool_get(&pool, 0), &dummy, sizeof(dummy)));

      assert(a == 0 && b == 1 && c == 2);
      assert(a != b && b != c && a != c);

      {
         size_t iter = 0;
         struct item *current;
         while ((current = chck_pool_iter(&pool, &iter, false)))
            assert(current != NULL);

         assert(pool.items.count == 3);
         assert(pool.items.used == 3 * sizeof(struct item));
         assert(pool.items.allocated == 32 * sizeof(struct item));
         assert(iter == 3);
      }

      chck_pool_remove(&pool, b);

      {
         size_t iter = 0;
         struct item *current;
         while ((current = chck_pool_iter(&pool, &iter, false)))
            assert(current != NULL);

         assert(pool.items.count == 2);
         assert(pool.items.used == 3 * sizeof(struct item));
         assert(pool.items.allocated == 32 * sizeof(struct item));
         assert(iter == 3);
      }

      assert(chck_pool_add(&pool, NULL, NULL));
      assert(!memcmp(chck_pool_get(&pool, 1), &dummy, sizeof(dummy)));

      {
         size_t iter = 0;
         struct item *current;
         while ((current = chck_pool_iter(&pool, &iter, false)))
            assert(current != NULL);

         assert(pool.items.count == 3);
         assert(pool.items.used == 3 * sizeof(struct item));
         assert(pool.items.allocated == 32 * sizeof(struct item));
         assert(iter == 3);
      }

      chck_pool_remove(&pool, c);

      {
         size_t iter = 0;
         struct item *current;
         while ((current = chck_pool_iter(&pool, &iter, true)))
            assert(current != NULL);

         assert(pool.items.count == 2);
         assert(pool.items.used == 2 * sizeof(struct item));
         assert(pool.items.allocated == 32 * sizeof(struct item));
         assert(iter == (size_t)-1);
      }

      chck_pool_add(&pool, NULL, NULL);

      {
         size_t iter = 0;
         struct item *current;
         while ((current = chck_pool_iter(&pool, &iter, false)))
            assert(current != NULL);

         assert(pool.items.count == 3);
         assert(pool.items.used == 3 * sizeof(struct item));
         assert(pool.items.allocated == 32 * sizeof(struct item));
         assert(iter == 3);
      }

      struct item *itemA = chck_pool_get(&pool, a);
      struct item *itemB = chck_pool_get(&pool, b);
      struct item *itemC = chck_pool_get(&pool, c);
      itemA->a = 1;
      itemB->a = 2;
      itemC->a = 3;

      assert(chck_pool_get(&pool, 0) == itemA);
      assert(chck_pool_get(&pool, 1) == itemB);
      assert(chck_pool_get(&pool, 2) == itemC);
      assert(chck_pool_get_last(&pool) == itemC);
      assert(&((struct item*)chck_pool_to_c_array(&pool, NULL))[0] == itemA);
      assert(&((struct item*)chck_pool_to_c_array(&pool, NULL))[1] == itemB);
      assert(&((struct item*)chck_pool_to_c_array(&pool, NULL))[2] == itemC);
      chck_pool_for_each_call(&pool, printa);

      chck_pool_flush(&pool);

      chck_pool_add(&pool, (&(struct item){1, NULL}), &a);
      chck_pool_add(&pool, (&(struct item){2, NULL}), &b);
      chck_pool_add(&pool, (&(struct item){3, NULL}), &c);
      chck_pool_add(&pool, (&(struct item){4, NULL}), &a);
      chck_pool_add(&pool, (&(struct item){5, NULL}), &b);
      chck_pool_add(&pool, (&(struct item){6, NULL}), &c);
      chck_pool_remove(&pool, a);
      chck_pool_remove(&pool, b);
      chck_pool_add(&pool, (&(struct item){7, NULL}), &c);
      chck_pool_add(&pool, (&(struct item){8, NULL}), &a);
      chck_pool_add(&pool, (&(struct item){9, NULL}), &b);

      assert(pool.items.used == 7 * sizeof(struct item));
      assert(pool.items.allocated == 32 * sizeof(struct item));

      {
         size_t aa = 0;
         struct item *current;
         chck_pool_for_each(&pool, current) {
            ++aa;
            assert(current->a != 4);
            assert(current->a != 5);
            assert(current != NULL);
         }

         assert(pool.items.count == aa);
      }

      {
         for (uint32_t i = 0; i < 32; ++i)
            chck_pool_add(&pool, NULL, NULL);

         assert(pool.items.used == 39 * sizeof(struct item));
         assert(pool.items.allocated == 64 * sizeof(struct item));

         for (uint32_t i = 0; i < 32; ++i)
            chck_pool_remove(&pool, 7 + i);

         assert(pool.items.used == 7 * sizeof(struct item));
         assert(pool.items.allocated == 8 * sizeof(struct item));
      }

      struct item bars[4] = {{.a = 1}};
      assert(chck_pool_set_c_array(&pool, &bars, 4));
      assert(pool.items.allocated == 4 * sizeof(struct item));
      assert(pool.items.used == 4 * sizeof(struct item));
      assert(!memcmp(pool.items.buffer, bars, sizeof(bars)));

      chck_pool_release(&pool);
      assert(pool.items.allocated == 0);
      assert(pool.items.used == 0);
   }

   /* TEST: iter pool */
   {
      struct chck_iter_pool pool = {0};

      {
         size_t iter = 0;
         assert(!chck_iter_pool_iter(&pool, &iter, false));
         assert(!iter);
         assert(!chck_iter_pool_iter(&pool, &iter, true));
         assert(!iter);
      }

      {
         size_t memb;
         assert(!chck_iter_pool_to_c_array(&pool, &memb) && memb == 0);
      }

      assert(chck_iter_pool(&pool, 32, 2, sizeof(struct item)));
      assert(pool.items.step == 32 * sizeof(struct item));
      assert(pool.items.member == sizeof(struct item));
      assert(pool.items.allocated == 2 * sizeof(struct item));
      assert(pool.items.used == 0);
      assert(pool.items.count == 0);
      assert(pool.items.buffer);

      assert(chck_iter_pool_push_back(&pool, (&(struct item){1, NULL})));
      assert(chck_iter_pool_push_back(&pool, (&(struct item){2, NULL})));
      chck_iter_pool_remove(&pool, 0);
      assert(((struct item*)chck_iter_pool_get(&pool, 0))->a == 2);
      chck_iter_pool_empty(&pool);
      assert(pool.items.allocated == 2 * sizeof(struct item));

      assert(chck_iter_pool_push_front(&pool, (&(struct item){1, NULL})));
      assert(chck_iter_pool_insert(&pool, 55, (&(struct item){2, NULL}))); // same as push_back when index > count
      assert(chck_iter_pool_insert(&pool, 1, (&(struct item){3, NULL})));
      assert(((struct item*)chck_iter_pool_get(&pool, 0))->a == 1);
      assert(((struct item*)chck_iter_pool_get(&pool, 1))->a == 3);
      assert(((struct item*)chck_iter_pool_get(&pool, 2))->a == 2);

      {
         size_t iter = 0;
         struct item *current;
         while ((current = chck_iter_pool_iter(&pool, &iter, false)))
            assert(current != NULL);

         assert(pool.items.count == 3);
         assert(pool.items.used == 3 * sizeof(struct item));
         assert(pool.items.allocated == 34 * sizeof(struct item));
         assert(iter == 3);
      }

      chck_iter_pool_remove(&pool, 1);

      {
         size_t iter = 0;
         struct item *current;
         while ((current = chck_iter_pool_iter(&pool, &iter, true)))
            assert(current != NULL);

         assert(pool.items.count == 2);
         assert(pool.items.used == 2 * sizeof(struct item));
         assert(pool.items.allocated == 34 * sizeof(struct item));
         assert(iter == (size_t)-1);
      }

      void *f;
      assert((f = chck_iter_pool_push_front(&pool, NULL)));
      assert(chck_iter_pool_get(&pool, 0) == f);

      {
         size_t iter = 0;
         struct item *current;
         while ((current = chck_iter_pool_iter(&pool, &iter, false)))
            assert(current != NULL);

         assert(pool.items.count == 3);
         assert(pool.items.used == 3 * sizeof(struct item));
         assert(pool.items.allocated == 34 * sizeof(struct item));
         assert(iter == 3);
      }

      chck_iter_pool_remove(&pool, 2);

      {
         size_t iter = 0;
         struct item *current;
         while ((current = chck_iter_pool_iter(&pool, &iter, false)))
            assert(current != NULL);

         assert(pool.items.count == 2);
         assert(pool.items.used == 2 * sizeof(struct item));
         assert(pool.items.allocated == 34 * sizeof(struct item));
         assert(iter == 2);
      }

      chck_iter_pool_push_back(&pool, NULL);

      {
         size_t iter = 0;
         struct item *current;
         while ((current = chck_iter_pool_iter(&pool, &iter, false)))
            assert(current != NULL);

         assert(pool.items.count == 3);
         assert(pool.items.used == 3 * sizeof(struct item));
         assert(pool.items.allocated == 34 * sizeof(struct item));
         assert(iter == 3);
      }

      struct item *itemA = chck_iter_pool_get(&pool, 0);
      struct item *itemB = chck_iter_pool_get(&pool, 1);
      struct item *itemC = chck_iter_pool_get(&pool, 2);
      itemA->a = 1;
      itemB->a = 2;
      itemC->a = 3;

      assert(chck_iter_pool_get(&pool, 0) == itemA);
      assert(chck_iter_pool_get(&pool, 1) == itemB);
      assert(chck_iter_pool_get(&pool, 2) == itemC);
      assert(chck_iter_pool_get_last(&pool) == itemC);
      assert(&((struct item*)chck_iter_pool_to_c_array(&pool, NULL))[0] == itemA);
      assert(&((struct item*)chck_iter_pool_to_c_array(&pool, NULL))[1] == itemB);
      assert(&((struct item*)chck_iter_pool_to_c_array(&pool, NULL))[2] == itemC);
      chck_iter_pool_for_each_call(&pool, printa);

      {
         for (uint32_t i = 0; i < 32; ++i)
            chck_iter_pool_push_back(&pool, NULL);

         assert(pool.items.used == 35 * sizeof(struct item));
         assert(pool.items.allocated == 66 * sizeof(struct item));

         for (uint32_t i = 0; i < 32; ++i)
            chck_iter_pool_remove(&pool, pool.items.count - 1);

         assert(pool.items.used == 3 * sizeof(struct item));
         assert(pool.items.allocated == 34 * sizeof(struct item));
      }

      struct item bars[4] = {{.a = 1}};
      assert(chck_iter_pool_set_c_array(&pool, &bars, 4));
      assert(pool.items.allocated == 4 * sizeof(struct item));
      assert(pool.items.used == 4 * sizeof(struct item));
      assert(!memcmp(pool.items.buffer, bars, sizeof(bars)));

      chck_iter_pool_release(&pool);
      assert(pool.items.allocated == 0);
      assert(pool.items.used == 0);
   }

   /* TEST: benchmark (many insertions, and removal expanding from center) */
   {
      const uint32_t iters = 0xFFFFF;
      struct chck_pool pool;
      assert(chck_pool(&pool, 32, iters, sizeof(struct item)));
      for (uint32_t i = 0; i < iters; ++i)
         assert(chck_pool_add(&pool, (&(struct item){i, NULL}), NULL));
      assert(pool.items.count == iters);
      assert(pool.items.used == iters * sizeof(struct item));
      for (uint32_t i = iters / 2, d = iters / 2; i < iters; ++i, --d) {
         assert(((struct item*)chck_pool_get(&pool, i))->a == i);
         assert(((struct item*)chck_pool_get(&pool, d))->a == d);
         chck_pool_remove(&pool, i);
         chck_pool_remove(&pool, d);
      }
      assert(pool.items.count == 0);
      assert(pool.items.used == 0);
      chck_pool_release(&pool);
      assert(pool.items.allocated == 0);
   }

   return EXIT_SUCCESS;
}
