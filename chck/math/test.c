#include "math.h"
#include <stdlib.h>

#undef NDEBUG
#include <assert.h>

int main(void)
{
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
      assert(chck_modnf(i, i * 2 + 20) == (int32_t)i);
      assert(chck_modnf(i, i * 2 - 20) == -(int32_t)(i - 20));
      assert(chck_modn(i, i * 2 + 20) == (int32_t)i);
      assert(chck_modn(i, i * 2 - 20) == -(int32_t)(i - 20));
   }

   assert(chck_max32(-20, 20) == 20);
   assert(chck_max32(40, 20) == 40);
   assert(chck_min32(-20, 20) == -20);
   assert(chck_min32(-40, 20) == -40);
   assert(chck_clamp32(40, -20, 60) == 40);
   assert(chck_clamp32(40, 50, 60) == 50);
   assert(chck_clamp32(40, -20, 20) == 20);

   assert(chck_modn(20 - 340, 360) == 40);
   assert(chck_modn(340 - 20, 360) == -40);
   assert(chck_modnf(20 - 340, 360) == 40);
   assert(chck_modnf(340 - 20, 360) == -40);
   return EXIT_SUCCESS;
}
