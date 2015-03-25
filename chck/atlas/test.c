#include "atlas.h"
#include <stdlib.h>
#include <stdio.h>

#undef NDEBUG
#include <assert.h>

int main(void)
{
   /* TEST: atlas packing */
   {
      struct chck_atlas atlas;
      assert(chck_atlas(&atlas));
      assert(atlas.count == 0);

      assert(chck_atlas_push(&atlas, 320, 320));
      assert(atlas.count == 1);

      assert(chck_atlas_pop(&atlas) == 0);
      assert(atlas.count == 0);

      assert(chck_atlas_push(&atlas, 320, 320) == 1);
      assert(chck_atlas_push(&atlas, 32, 32) == 2);
      assert(chck_atlas_push(&atlas, 32, 288) == 3);
      assert(atlas.count == 3);
      assert(atlas.longest_edge == 320 && atlas.total_area == 320 * 320 + 32 * 32 + 32 * 288);

      uint32_t w, h;
      uint32_t free = chck_atlas_pack(&atlas, false, false, &w, &h);

      printf("%ux%u (%u)\n", w, h, free);
      assert(w == 352 && h == 320 && free == w * h - atlas.total_area);

      struct chck_atlas_rect tr;
      const struct chck_atlas_texture *t;
      assert((t = chck_atlas_get(&atlas, 1, &tr)));
      assert(tr.x == 0 && tr.y == 0 && tr.w == 320 && tr.h == 320);
      assert(!t->flipped && t->placed && t->longest_edge == 320 && t->area == 320 * 320);

      assert((t = chck_atlas_get(&atlas, 2, &tr)));
      assert(tr.x == 320 && tr.y == 288 && tr.w == 32 && tr.h == 32);
      assert(!t->flipped && t->placed && t->longest_edge == 32 && t->area == 32 * 32);

      assert((t = chck_atlas_get(&atlas, 3, &tr)));
      assert(tr.x == 320 && tr.y == 0 && tr.w == 32 && tr.h == 288);
      assert(!t->flipped && t->placed && t->longest_edge == 288 && t->area == 32 * 288);

      chck_atlas_release(&atlas);
   }

   /* TEST: atlas packing (forced pot) */
   {
      struct chck_atlas atlas;
      assert(chck_atlas(&atlas));
      assert(chck_atlas_push(&atlas, 320, 320) == 1);
      assert(chck_atlas_push(&atlas, 32, 32) == 2);
      assert(chck_atlas_push(&atlas, 32, 288) == 3);
      assert(atlas.count == 3 && atlas.total_area == 320 * 320 + 32 * 32 + 32 * 288);

      uint32_t w, h;
      uint32_t free = chck_atlas_pack(&atlas, true, false, &w, &h);
      printf("%ux%u (%u)\n", w, h, free);
      assert(w == 512 && h == 512 && free == w * h - atlas.total_area);

      struct chck_atlas_rect tr;
      const struct chck_atlas_texture *t;
      assert((t = chck_atlas_get(&atlas, 1, &tr)));
      assert(tr.x == 0 && tr.y == 0 && tr.w == 320 && tr.h == 320);
      assert(!t->flipped && t->placed && t->longest_edge == 320 && t->area == 320 * 320);

      assert((t = chck_atlas_get(&atlas, 2, &tr)));
      assert(tr.x == 320 && tr.y == 288 && tr.w == 32 && tr.h == 32);
      assert(!t->flipped && t->placed && t->longest_edge == 32 && t->area == 32 * 32);

      assert((t = chck_atlas_get(&atlas, 3, &tr)));
      assert(tr.x == 320 && tr.y == 0 && tr.w == 32 && tr.h == 288);
      assert(!t->flipped && t->placed && t->longest_edge == 288 && t->area == 32 * 288);

      chck_atlas_release(&atlas);
   }

   return EXIT_SUCCESS;
}
