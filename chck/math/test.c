#include "math.h"
#include <stdlib.h>

#undef NDEBUG
#include <assert.h>

#pragma GCC diagnostic ignored "-Wsuggest-attribute=pure"

int main(void)
{
   assert(chck_equalld(10.0, 10.000000000001l, 1000000.0l));
   assert(!chck_equalld(10.0, 10.000000000001l, 1.0l));
   assert(chck_equal(10.0, 10.000000000001, 1000.0));
   assert(!chck_equal(10.0, 10.000000000001, 1.0));
   assert(chck_equalf(10.0f, 10.00001f, 10.0f));
   assert(!chck_equalf(10.0f, 10.00001f, 1.0f));

   for (uint32_t i = 0xFFFF; i < 0xFFFFFF; ++i) {
      assert(chck_minu32(i, i - 20) == i - 20);
      assert(chck_minu32(i, i + 20) == i);
      assert(chck_maxu32(i, i - 20) == i);
      assert(chck_maxu32(i, i + 20) == i + 20);
      assert(chck_clampu32(i, i - 20, i + 20) == i);
      assert(chck_clampu32(i, i - 40, i - 20) == i - 20);
      assert(chck_clampu32(i, i + 20, i + 40) == i + 20);
      assert(chck_modn32(i, i * 2 + 20) == (int32_t)i);
      assert(chck_modn32(i, i * 2 - 20) == -(int32_t)(i - 20));
      assert((int32_t)chck_modnf(i, i * 2 + 20) == (int32_t)i);
      assert((int32_t)chck_modnf(i, i * 2 - 20) == -(int32_t)(i - 20));
      assert((int32_t)chck_modn(i, i * 2 + 20) == (int32_t)i);
      assert((int32_t)chck_modn(i, i * 2 - 20) == -(int32_t)(i - 20));
   }

   assert(chck_max32(-20, 20) == 20);
   assert(chck_max32(40, 20) == 40);
   assert(chck_min32(-20, 20) == -20);
   assert(chck_min32(-40, 20) == -40);
   assert(chck_clamp32(40, -20, 60) == 40);
   assert(chck_clamp32(40, 50, 60) == 50);
   assert(chck_clamp32(40, -20, 20) == 20);

   assert((int32_t)chck_modn(20 - 340, 360) == 40);
   assert((int32_t)chck_modn(340 - 20, 360) == -40);
   assert((int32_t)chck_modnf(20 - 340, 360) == 40);
   assert((int32_t)chck_modnf(340 - 20, 360) == -40);

   assert(chck_npotu8(3) == 4);
   assert(chck_npotu8(1) == 1);
   assert(chck_npotu16(25) == 32);
   assert(chck_npotu16(48) == 64);
   assert(chck_npotu32(0) == 1);
   assert(chck_npotu32(4097) == 8192);
   assert(chck_npotsz(1025) == 2048);
   return EXIT_SUCCESS;
}
