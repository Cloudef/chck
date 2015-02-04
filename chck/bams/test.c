#include "bams.h"
#include "math/math.h"
#include <stdlib.h>
#include <assert.h>

int main(void)
{
   for (uint32_t x = 0; x < 360; x += 10) {
      assert(bams64_from_double(x, 360.0) == (uint64_t)((x / 360.0) * (uint64_t)~0));
      assert(bams64_from_float(x, 360.0f) == (uint64_t)((x / 360.0f) * (uint64_t)~0));
      assert(chck_equal(double_from_bams64(bams64_from_double(x, 360.0), 360.0), x, 1.0));
      assert(chck_equalf(float_from_bams64(bams64_from_float(x, 360.0f), 360.0f), x, 1.0));
      assert(bams32_from_double(x, 360.0) == (uint32_t)((x / 360.0) * (uint32_t)~0));
      assert(bams32_from_float(x, 360.0f) == (uint32_t)((x / 360.0f) * (uint32_t)~0));
      assert(chck_equal(double_from_bams32(bams32_from_double(x, 360.0), 360.0), x, pow(1000.0, 3)));
      assert(chck_equalf(float_from_bams32(bams32_from_float(x, 360.0f), 360.0f), x, 1.0));
      assert(bams16_from_double(x, 360.0) == (uint16_t)((x / 360.0) * ((uint16_t)~0 + 1)));
      assert(bams16_from_float(x, 360.0f) == (uint16_t)((x / 360.0f) * ((uint16_t)~0 + 1)));
      assert(chck_equal(double_from_bams16(bams16_from_double(x, 360.0), 360.0), x, pow(1000.0, 4)));
      assert(chck_equalf(float_from_bams16(bams16_from_float(x, 360.0f), 360.0f), x, pow(1000.0, 2)));
      assert(bams8_from_double(x, 360.0) == (uint8_t)((x / 360.0) * ((uint8_t)~0 + 1)));
      assert(bams8_from_float(x, 360.0f) == (uint8_t)((x / 360.0f) * ((uint8_t)~0 + 1)));
      assert(chck_equal(double_from_bams8(bams8_from_double(x, 360.0), 360.0), x, pow(1000.0, 5)));
      assert(chck_equalf(float_from_bams8(bams8_from_float(x, 360.0f), 360.0f), x, pow(1000.0, 3)));
   }
   return EXIT_SUCCESS;
}
