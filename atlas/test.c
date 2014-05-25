#include "atlas.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

int main(void)
{
   /* TEST: atlas packing */
   {
      chckAtlas *atlas = chckAtlasNew();
      assert(atlas != NULL);

      assert(chckAtlasCount(atlas) == 0);

      assert(chckAtlasPush(atlas, 320, 320) == 1);
      assert(chckAtlasCount(atlas) == 1);

      chckAtlasPop(atlas);
      assert(chckAtlasCount(atlas) == 0);

      assert(chckAtlasPush(atlas, 320, 320) == 1);
      assert(chckAtlasPush(atlas, 32, 32) == 2);
      assert(chckAtlasPush(atlas, 32, 288) == 3);
      assert(chckAtlasCount(atlas) == 3);

      unsigned int w, h;
      unsigned int free = chckAtlasPack(atlas, 0, 0, &w, &h);

      printf("%ux%u (%u)\n", w, h, free);
      assert(w == 352 && h == 320);

      unsigned int x, y;
      assert(chckAtlasGetTextureLocation(atlas, 1, &x, &y, &w, &h) == 0);
      assert(x == 0 && y == 0 && w == 320 && h == 320);

      assert(chckAtlasGetTextureLocation(atlas, 2, &x, &y, &w, &h) == 0);
      assert(x == 320 && y == 288 && w == 32 && h == 32);

      assert(chckAtlasGetTextureLocation(atlas, 3, &x, &y, &w, &h) == 0);
      assert(x == 320 && y == 0 && w == 32 && h == 288);

      chckAtlasReset(atlas);
      assert(chckAtlasPush(atlas, 320, 320) == 1);
      assert(chckAtlasPush(atlas, 32, 32) == 2);
      assert(chckAtlasPush(atlas, 32, 288) == 3);
      assert(chckAtlasCount(atlas) == 3);

      free = chckAtlasPack(atlas, 1, 0, &w, &h);
      printf("%ux%u (%u)\n", w, h, free);
      assert(w == 512 && h == 512);

      assert(chckAtlasGetTextureLocation(atlas, 1, &x, &y, &w, &h) == 0);
      assert(x == 0 && y == 0 && w == 320 && h == 320);

      assert(chckAtlasGetTextureLocation(atlas, 2, &x, &y, &w, &h) == 0);
      assert(x == 320 && y == 288 && w == 32 && h == 32);

      assert(chckAtlasGetTextureLocation(atlas, 3, &x, &y, &w, &h) == 0);
      assert(x == 320 && y == 0 && w == 32 && h == 288);

      chckAtlasFree(atlas);
   }
   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
