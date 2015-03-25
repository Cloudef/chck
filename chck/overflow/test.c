#include "overflow.h"
#include <stdlib.h>
#include <limits.h>

#undef NDEBUG
#include <assert.h>

int main(void)
{
#define test_of(T, n, s) \
   { \
      T r; \
      assert(chck_add_of##n(1, s, &r)); \
      assert(chck_add_of##n(s, 1, &r)); \
      assert(chck_sub_of##n(((T)~0 == s ? 0 : -2), s, &r)); \
      assert(chck_mul_of##n(3, s, &r)); \
      assert(chck_mul_of##n(-3, s, &r)); \
      assert(!chck_add_of##n(4, 4, &r)); \
      assert(!chck_sub_of##n(8, 4, &r)); \
      assert(!chck_mul_of##n(5, 5, &r)); \
   }

   // TEST: overflow functions
   test_of(size_t, sz, SIZE_MAX);
   test_of(uint64_t, u64, UINT64_MAX);
   test_of(int64_t, 64, INT64_MAX);
   test_of(uint32_t, u32, UINT32_MAX);
   test_of(int32_t, 32, INT32_MAX);
   test_of(uint16_t, u16, UINT16_MAX);
   test_of(int16_t, 16, INT16_MAX);
   test_of(uint8_t, u8, UINT8_MAX);
   test_of(int8_t, 8, INT8_MAX);

   // TEST: overflow aware alloc functions
   {
      void *ptr;
      assert((ptr = chck_malloc_add_of(8, 8))); free(ptr);
      assert(!(ptr = chck_malloc_add_of(SIZE_MAX, 8))); free(ptr);
      assert(!(ptr = chck_malloc_add_of(8, SIZE_MAX))); free(ptr);

      assert((ptr = chck_malloc_sub_of(8, 4))); free(ptr);
      assert(!(ptr = chck_malloc_sub_of(4, 8))); free(ptr);

      assert((ptr = chck_malloc_mul_of(8, 8))); free(ptr);
      assert(!(ptr = chck_malloc_mul_of(8, SIZE_MAX))); free(ptr);
      assert(!(ptr = chck_malloc_mul_of(SIZE_MAX, 8))); free(ptr);

      assert((ptr = chck_calloc_add_of(8, 8))); free(ptr);
      assert(!(ptr = chck_calloc_add_of(SIZE_MAX, 8))); free(ptr);
      assert(!(ptr = chck_calloc_add_of(8, SIZE_MAX))); free(ptr);

      assert((ptr = chck_malloc_sub_of(8, 4))); free(ptr);
      assert(!(ptr = chck_malloc_sub_of(4, 8))); free(ptr);

      assert((ptr = chck_calloc_of(8, 8))); free(ptr);
      assert(!(ptr = chck_calloc_of(8, SIZE_MAX))); free(ptr);
      assert(!(ptr = chck_calloc_of(SIZE_MAX, 8))); free(ptr);

      assert((ptr = chck_realloc_add_of(ptr, 8, 8))); free(ptr);
      assert(!(ptr = chck_realloc_add_of(ptr, SIZE_MAX, 8))); free(ptr);
      assert(!(ptr = chck_realloc_add_of(ptr, 8, SIZE_MAX))); free(ptr);
      ptr = NULL;

      assert((ptr = chck_realloc_sub_of(ptr, 8, 4))); free(ptr);
      assert(!(ptr = chck_realloc_sub_of(ptr, 4, 8))); free(ptr);
      ptr = NULL;

      assert((ptr = chck_realloc_mul_of(ptr, 8, 8))); free(ptr);
      assert(!(ptr = chck_realloc_mul_of(ptr, 8, SIZE_MAX))); free(ptr);
      assert(!(ptr = chck_realloc_mul_of(ptr, SIZE_MAX, 8))); free(ptr);
   }

   return EXIT_SUCCESS;
}
