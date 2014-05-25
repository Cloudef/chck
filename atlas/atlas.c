#include "atlas.h"
#include <stdlib.h> /* for malloc */
#include <string.h> /* for memset */
#include <limits.h> /* for UINT_MAX */
#include <assert.h> /* for assert */

struct rect {
   unsigned int x, w, y, h;
};

struct texture {
   struct rect rect;
   unsigned int area;
   unsigned int longestEdge;
   unsigned char flipped;
   unsigned char placed;
};

struct node {
   struct node *next;
   struct rect rect;
};

typedef struct _chckAtlas {
   struct node *freeList;
   struct texture *textures;
   unsigned int longestEdge;
   unsigned int totalArea;
   unsigned int debugCount;
   unsigned int textureCount;
} _chckAtlas;

static void textureInit(struct texture *texture, const unsigned int width, const unsigned int height)
{
   assert(texture);
   memset(texture, 0, sizeof(struct texture));
   texture->rect.w = width;
   texture->rect.h = height;
   texture->area = width * height;
   texture->longestEdge = (width >= height ? width : height);
}

static void texturePlace(struct texture *texture, const unsigned int x, const unsigned int y, const int flipped)
{
   assert(texture);
   texture->rect.x = x;
   texture->rect.y = y;
   texture->flipped = flipped;
   texture->placed = 1;
}

static int nodeFits(struct node *node, const unsigned int width, const unsigned int height, unsigned int *outEdgeCount)
{
   assert(node && outEdgeCount);

   unsigned int ec = 0;
   if (width == node->rect.w) ec += (height == node->rect.w ? 2 : 1);
   else if (width == node->rect.h) ec += (height == node->rect.w  ? 2 : 1);
   else if (height == node->rect.w) ec++;
   else if (height == node->rect.h) ec++;

   *outEdgeCount = ec;
   return ((width <= node->rect.w && height <= node->rect.w) || (height <= node->rect.h && width <= node->rect.h));
}

static int nodeMerge(struct node *n1, struct node *n2)
{
   int ret = 0;

   /* if we share the top edge then.. */
   if (n1->rect.x == n2->rect.x && n1->rect.w == n2->rect.w && n1->rect.y == n2->rect.y + n2->rect.h) {
      n1->rect.y = n2->rect.y;
      n1->rect.h += n2->rect.h;
      ret = 1;
   }
   /* if we share the bottom edge  */
   else if (n1->rect.x == n2->rect.x && n1->rect.w == n2->rect.w && n1->rect.y + n1->rect.h == n2->rect.y) {
      n1->rect.h += n2->rect.h;
      ret = 1;
   }
   /* if we share the left edge */
   else if (n1->rect.y == n2->rect.y && n1->rect.y + n1->rect.h == n2->rect.y && n1->rect.x == n2->rect.x + n2->rect.w) {
      n1->rect.x = n2->rect.x;
      n1->rect.w += n2->rect.w;
      ret = 1;
   }
   /* if we share the left edge */
   else if (n1->rect.y == n2->rect.y && n1->rect.y + n1->rect.h == n2->rect.y && n1->rect.x + n1->rect.w == n2->rect.x) {
      n1->rect.w += n2->rect.w;
      ret = 1;
   }

   return ret;
}

static void atlasNodeNew(chckAtlas *atlas, const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height)
{
   struct node *node = calloc(1, sizeof(struct node));

   if (!node)
      return;

   node->rect.x = x;
   node->rect.y = y;
   node->rect.w = width;
   node->rect.h = height;
   node->next = atlas->freeList;
   atlas->freeList = node;
}

static int nextPow2(const unsigned int v)
{
   if (v && !(v & (v - 1)))
      return v;

   unsigned int p;
   for (p = 1; p < v; p = p * 2);
   return p;
}

static int mergeNodes(chckAtlas *atlas)
{
   struct node *f = atlas->freeList;
   while (f) {
      struct node *c = atlas->freeList, *prev = NULL;
      while (c) {
         if (f != c) {
            if (nodeMerge(f, c)) {
               assert(prev);
               prev->next = c->next;
               free(c);
               return 1;
            }
         }
         prev = c;
         c = c->next;
      }
      f = f->next;
   }
   return 0;
}

static void checkDimensions(chckAtlas *atlas, unsigned int *inOutWidth, unsigned int *inOutHeight)
{
   assert(atlas && inOutWidth && inOutHeight);
   unsigned int i;
   for (i = 0; i != atlas->textureCount; ++i) {
      struct texture *t = &atlas->textures[i];
      if (t->rect.w > *inOutWidth) *inOutWidth = t->rect.w;
      if (t->rect.h > *inOutHeight) *inOutHeight = t->rect.h;
   }
}

chckAtlas* chckAtlasNew(void)
{
   return calloc(1, sizeof(chckAtlas));
}

void chckAtlasFree(chckAtlas *atlas)
{
   assert(atlas);
   chckAtlasReset(atlas);
   free(atlas);
}

void chckAtlasReset(chckAtlas *atlas)
{
   if(atlas->textures)
      free(atlas->textures);

   if (atlas->freeList) {
      struct node *next = atlas->freeList, *kill;
      while (next) {
         kill = next;
         next = next->next;
         free(kill);
      }
   }

   memset(atlas, 0, sizeof(chckAtlas));
}

unsigned int chckAtlasCount(chckAtlas *atlas)
{
   assert(atlas);
   return atlas->textureCount;
}

unsigned int chckAtlasPush(chckAtlas *atlas, const unsigned int width, const unsigned int height)
{
   assert(atlas);

   void *tmp;
   if (!(tmp = realloc(atlas->textures, (atlas->textureCount + 1) * sizeof(struct texture))))
      return 0;

   atlas->textures = tmp;
   atlas->textureCount += 1;

   textureInit(&atlas->textures[atlas->textureCount - 1], width, height);

   if (width > atlas->longestEdge)
      atlas->longestEdge = width;

   if (height > atlas->longestEdge)
      atlas->longestEdge = height;

   atlas->totalArea += width * height;
   return atlas->textureCount;
}

void chckAtlasPop(chckAtlas *atlas)
{
   assert(atlas);

   if (atlas->textureCount <= 0)
      return;

   struct texture *t = &atlas->textures[atlas->textureCount - 1];
   atlas->totalArea -= t->rect.w * t->rect.h;

   void *tmp = NULL;
   if (atlas->textureCount > 1) {
      if (!(tmp = realloc(atlas->textures, (atlas->textureCount - 1) * sizeof(struct texture))))
         return;
   } else {
      free(atlas->textures);
   }

   atlas->textures = tmp;
   atlas->textureCount -= 1;
}

int chckAtlasGetTextureLocation(const chckAtlas *atlas, const unsigned int index, unsigned int *outX, unsigned int *outY, unsigned int *outWidth, unsigned int *outHeight)
{
   assert(atlas && index > 0);

   if (outX) *outX = 0;
   if (outY) *outY = 0;
   if (outWidth) *outWidth = 0;
   if (outHeight) *outHeight = 0;

   if (index - 1 >= atlas->textureCount)
      return 0;

   const struct texture *t = &atlas->textures[index - 1];
   if (outX) *outX = t->rect.x;
   if (outY) *outY = t->rect.y;
   if (outWidth) *outWidth = (t->flipped ? t->rect.h : t->rect.w);
   if (outHeight) *outHeight = (t->flipped ? t->rect.w : t->rect.h);
   return t->flipped;
}

int chckAtlasPack(chckAtlas *atlas, const int forcePowerOfTwo, const int onePixelBorder, unsigned int *outWidth, unsigned int *outHeight)
{
   assert(atlas);

   if (onePixelBorder) {
      unsigned int i;
      for (i = 0; i != atlas->textureCount; ++i) {
         struct texture *t = &atlas->textures[i];
         t->rect.w += 2;
         t->rect.h += 2;
      }
      atlas->longestEdge += 2;
   }

   if (forcePowerOfTwo)
      atlas->longestEdge = nextPow2(atlas->longestEdge);

   unsigned int width = atlas->longestEdge;
   unsigned int count = (unsigned int)((float)atlas->totalArea/(atlas->longestEdge * atlas->longestEdge) + 0.5f);
   unsigned int height = (count + 2) * atlas->longestEdge;

   if (forcePowerOfTwo)
      height = nextPow2(height);

   if (width > height && height != width * 0.5f) {
      height = width * 0.5f;
      width *= 0.5f;
      checkDimensions(atlas, &width, &height);
   } else if (height > width && width != height * 0.5f) {
      width = height * 0.5f;
      height *= 0.5f;
      checkDimensions(atlas, &width, &height);
   }

   atlas->debugCount = 0;
   atlasNodeNew(atlas, 0, 0, width, height);

   unsigned int i;
   for (i = 0; i != atlas->textureCount; ++i) {
      unsigned int i2;
      unsigned int longestEdge = 0, mostArea = 0, index = 0;
      for(i2 = 0; i2 != atlas->textureCount; ++i2) {
         struct texture *t = &atlas->textures[i2];
         if (t->placed)
            continue;

         if (t->longestEdge > longestEdge) {
            mostArea = t->area;
            longestEdge = t->longestEdge;
            index = i2;
         } else if (t->longestEdge == longestEdge && t->area > mostArea) {
            mostArea = t->area;
            index = i2;
         }
      }

      unsigned int edgeCount = 0;
      unsigned int leastY = UINT_MAX;
      unsigned int leastX = UINT_MAX;
      struct texture *t = &atlas->textures[index];
      struct node *previousBestFit = NULL;
      struct node *bestFit = NULL;
      struct node *previous = NULL;
      struct node *search = atlas->freeList;

      while (search) {
         unsigned int ec;
         if (nodeFits(search, t->rect.w, t->rect.h, &ec)) {
            if (ec == 2) {
               previousBestFit = previous;
               bestFit = search;
               edgeCount = ec;
               break;
            } else if (search->rect.y < leastY) {
               leastY = search->rect.y;
               leastX = search->rect.x;
               previousBestFit = previous;
               bestFit = search;
               edgeCount = ec;
            } else if (search->rect.y == leastY && search->rect.x < leastX) {
               leastX = search->rect.x;
               previousBestFit = previous;
               bestFit = search;
               edgeCount = ec;
            }
         }
         previous = search;
         search = search->next;
      }

      /* should always find a fit */
      assert(bestFit);

      switch (edgeCount) {
         case 0:
            if (t->longestEdge <= bestFit->rect.w) {
               int flipped = 0;
               unsigned int width = t->rect.w;
               unsigned int height = t->rect.h;

               if (width > height) {
                  width = t->rect.h;
                  height = t->rect.w;
                  flipped  = 1;
               }

               texturePlace(t, bestFit->rect.x, bestFit->rect.y, flipped);
               atlasNodeNew(atlas, bestFit->rect.x, bestFit->rect.y + height, bestFit->rect.w, bestFit->rect.h - height);

               bestFit->rect.x += width;
               bestFit->rect.w -= width;
               bestFit->rect.h = height;
            } else {
               assert(t->longestEdge <= bestFit->rect.h);
               int flipped  = 0;
               unsigned int width = t->rect.w;
               unsigned int height = t->rect.h;

               if (width > height) {
                  width = t->rect.h;
                  height = t->rect.w;
                  flipped  = 1;
               }

               texturePlace(t, bestFit->rect.x, bestFit->rect.y, flipped);
               atlasNodeNew(atlas, bestFit->rect.x + width, bestFit->rect.y, bestFit->rect.w - width, bestFit->rect.h);

               bestFit->rect.y += height;
               bestFit->rect.h -= height;
               bestFit->rect.w = width;
            }
            break;
         case 1:
            if (t->rect.w == bestFit->rect.w) {
               texturePlace(t, bestFit->rect.x, bestFit->rect.y, 0);
               bestFit->rect.y += t->rect.h;
               bestFit->rect.h -= t->rect.h;
            } else if (t->rect.h == bestFit->rect.h) {
               texturePlace(t, bestFit->rect.x, bestFit->rect.y, 0);
               bestFit->rect.x += t->rect.w;
               bestFit->rect.w -= t->rect.w;
            } else if (t->rect.w == bestFit->rect.h) {
               texturePlace(t, bestFit->rect.x, bestFit->rect.y, 1);
               bestFit->rect.x += t->rect.h;
               bestFit->rect.w -= t->rect.h;
            } else if (t->rect.h == bestFit->rect.w) {
               texturePlace(t, bestFit->rect.x, bestFit->rect.y, 1);
               bestFit->rect.y += t->rect.w;
               bestFit->rect.h -= t->rect.w;
            }
            break;
         case 2:
            {
               int flipped = (t->rect.w != bestFit->rect.w || t->rect.h != bestFit->rect.h);
               texturePlace(t, bestFit->rect.x, bestFit->rect.y, flipped);

               if (previousBestFit) {
                  previousBestFit->next = bestFit->next;
               } else {
                  atlas->freeList = bestFit->next;
               }

               if (bestFit)
                  free(bestFit);
            }
            break;
      }

      /* merge as much as we can */
      while (mergeNodes(atlas));
   }

   width = 0, height = 0;
   for(i = 0; i < atlas->textureCount; ++i) {
      struct texture *t = &atlas->textures[i];
      if (onePixelBorder) {
         t->rect.w -= 2;
         t->rect.h -= 2;
         t->rect.x += 1;
         t->rect.y += 1;
      }

      unsigned int x = (t->flipped ? t->rect.x + t->rect.h : t->rect.x + t->rect.w);
      unsigned int y = (t->flipped ? t->rect.y + t->rect.w : t->rect.y + t->rect.h);
      width = (x > width ? x : width);
      height = (y > height ? y : height);
   }

   if (forcePowerOfTwo) {
      width = nextPow2(width);
      height = nextPow2(height);
   }

   if (outWidth) *outWidth = width;
   if (outHeight) *outHeight = height;
   return (width * height) - atlas->totalArea;
}

/* vim: set ts=8 sw=3 tw=0 :*/
