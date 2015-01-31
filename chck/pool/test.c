#include "pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

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
   /* TEST: pool */
   {
      struct chck_pool pool;
      assert(chck_pool(&pool, 32, 3, sizeof(struct item)));

      assert(chck_pool_add(&pool, (&(struct item){1, NULL}), NULL));
      assert(chck_pool_add(&pool, (&(struct item){2, NULL}), NULL));
      chck_pool_remove(&pool, 0);
      assert(((struct item*)chck_pool_get(&pool, 1))->a == 2);
      chck_pool_release(&pool);

      size_t a, b, c;
      assert(chck_pool_add(&pool, (&(struct item){1, NULL}), &a));
      assert(chck_pool_add(&pool, (&(struct item){2, NULL}), &b));
      assert(chck_pool_add(&pool, (&(struct item){3, NULL}), &c));

      assert(a == 0 && b == 1 && c == 2);
      assert(a != b && b != c && a != c);

      size_t iter = 0;
      struct item *current;
      while ((current = chck_pool_iter(&pool, &iter)))
         assert(current != NULL);

      assert(pool.items.count == 3);
      assert(iter == 3);
      chck_pool_remove(&pool, b);

      iter = 0;
      while ((current = chck_pool_iter(&pool, &iter)))
         assert(current != NULL);

      assert(pool.items.count == 2);
      assert(iter == 3);

      assert(chck_pool_add(&pool, NULL, NULL));

      iter = 0;
      while ((current = chck_pool_iter(&pool, &iter)))
         assert(current != NULL);

      assert(pool.items.count == 3);
      assert(iter == 3);

      chck_pool_remove(&pool, c);

      iter = 0;
      while ((current = chck_pool_iter(&pool, &iter)))
         assert(current != NULL);

      assert(pool.items.count == 2);
      assert(iter == 2);

      chck_pool_add(&pool, NULL, NULL);

      iter = 0;
      while ((current = chck_pool_iter(&pool, &iter)))
         assert(current != NULL);

      assert(pool.items.count == 3);
      assert(iter == 3);

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

      chck_pool_release(&pool);

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

      iter = 0;
      size_t aa = 0;
      while ((current = chck_pool_iter(&pool, &iter))) {
         ++aa;
         assert(((struct item*)current)->a != 4);
         assert(((struct item*)current)->a != 5);
         assert(current != NULL);
      }

      printf("%zu, %zu\n", aa, pool.items.count);
      assert(pool.items.count == aa);

      chck_pool_release(&pool);
   }

   /* TEST: iter pool */
   {
      struct chck_iter_pool pool;
      assert(chck_iter_pool(&pool, 32, 3, sizeof(struct item)));

      assert(chck_iter_pool_push_back(&pool, (&(struct item){1, NULL})));
      assert(chck_iter_pool_push_back(&pool, (&(struct item){2, NULL})));
      chck_iter_pool_remove(&pool, 0);
      assert(((struct item*)chck_iter_pool_get(&pool, 0))->a == 2);
      chck_iter_pool_release(&pool);

      assert(chck_iter_pool_push_back(&pool, (&(struct item){1, NULL})));
      assert(chck_iter_pool_push_back(&pool, NULL));
      assert(chck_iter_pool_push_back(&pool, NULL));

      size_t iter = 0;
      struct item *current;
      while ((current = chck_iter_pool_iter(&pool, &iter)))
         assert(current != NULL);

      assert(pool.items.count == 3);
      assert(iter == 3);
      chck_iter_pool_remove(&pool, 1);

      iter = 0;
      while ((current = chck_iter_pool_iter(&pool, &iter)))
         assert(current != NULL);

      assert(pool.items.count == 2);
      assert(iter == 2);

      void *f;
      assert((f = chck_iter_pool_push_front(&pool, NULL)));
      assert(chck_iter_pool_get(&pool, 0) == f);

      iter = 0;
      while ((current = chck_iter_pool_iter(&pool, &iter)))
         assert(current != NULL);

      assert(pool.items.count == 3);
      assert(iter == 3);

      chck_iter_pool_remove(&pool, 2);

      iter = 0;
      while ((current = chck_iter_pool_iter(&pool, &iter)))
         assert(current != NULL);

      assert(pool.items.count == 2);
      assert(iter == 2);

      chck_iter_pool_push_back(&pool, NULL);

      iter = 0;
      while ((current = chck_iter_pool_iter(&pool, &iter)))
         assert(current != NULL);

      assert(pool.items.count == 3);
      assert(iter == 3);

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

      chck_iter_pool_release(&pool);
   }

   /* TEST: ring pool */
   {
      struct chck_ring_pool pool;
      assert(chck_ring_pool(&pool, 32, 3, sizeof(struct item)));

      void *a, *b, *c;
      a = chck_ring_pool_push_back(&pool, (&(struct item){1, NULL}));
      b = chck_ring_pool_push_back(&pool, (&(struct item){2, NULL}));
      c = chck_ring_pool_push_back(&pool, (&(struct item){3, NULL}));

      assert(a != NULL && b != NULL && c != NULL);
      assert(a != b && b != c && a != c);

      size_t iter = 0;
      struct item *current;
      while ((current = chck_ring_pool_iter(&pool, &iter)))
         assert(current != NULL);

      assert(pool.items.count == 3);
      assert(iter == 3);

      assert(((struct item*)chck_ring_pool_pop_first(&pool))->a == 1);

      iter = 0;
      while ((current = chck_ring_pool_iter(&pool, &iter)))
         assert(current != NULL);

      assert(pool.items.count == 2);
      assert(iter == 2);

      assert(chck_ring_pool_push_front(&pool, NULL));

      iter = 0;
      while ((current = chck_ring_pool_iter(&pool, &iter)))
         assert(current != NULL);

      assert(pool.items.count == 3);
      assert(iter == 3);

      assert(((struct item*)chck_ring_pool_pop_last(&pool))->a == 3);

      iter = 0;
      while ((current = chck_ring_pool_iter(&pool, &iter)))
         assert(current != NULL);

      assert(pool.items.count == 2);
      assert(iter == 2);

      chck_ring_pool_push_back(&pool, NULL);

      iter = 0;
      while ((current = chck_ring_pool_iter(&pool, &iter)))
         assert(current != NULL);

      assert(pool.items.count == 3);
      assert(iter == 3);

      chck_ring_pool_release(&pool);

      chck_ring_pool_push_back(&pool, (&(struct item){1, NULL}));
      chck_ring_pool_push_front(&pool, (&(struct item){3, NULL}));
      chck_ring_pool_push_back(&pool, (&(struct item){2, NULL}));

      assert(((struct item*)chck_ring_pool_to_c_array(&pool, NULL))[0].a == 3);
      assert(((struct item*)chck_ring_pool_to_c_array(&pool, NULL))[1].a == 1);
      assert(((struct item*)chck_ring_pool_to_c_array(&pool, NULL))[2].a == 2);
      chck_ring_pool_for_each_call(&pool, printa);

      assert(((struct item*)chck_ring_pool_pop_last(&pool))->a == 2);
      assert(((struct item*)chck_ring_pool_pop_first(&pool))->a == 3);
      assert(((struct item*)chck_ring_pool_pop_last(&pool))->a == 1);

      chck_ring_pool_release(&pool);
   }

   /* TEST: benchmark (many insertions, and removal expanding from center) */
   {
      static const uint32_t iters = 0xFFFFF;
      struct chck_pool pool;
      assert(chck_pool(&pool, 32, iters, sizeof(struct item)));
      for (uint32_t i = 0; i < iters; ++i)
         assert(chck_pool_add(&pool, (&(struct item){i, NULL}), NULL));
      assert(pool.items.count == iters);
      for (uint32_t i = iters / 2, d = iters / 2; i < iters; ++i, --d) {
         assert(((struct item*)chck_pool_get(&pool, i))->a == i);
         assert(((struct item*)chck_pool_get(&pool, d))->a == d);
         chck_pool_remove(&pool, i);
         chck_pool_remove(&pool, d);
      }
      assert(pool.items.count == 0);
      chck_pool_release(&pool);
   }

   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
