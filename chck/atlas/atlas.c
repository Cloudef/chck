#include "atlas.h"
#include <stdlib.h> /* for malloc */
#include <string.h> /* for memset */
#include <limits.h> /* for UINT_MAX */
#include <assert.h> /* for assert */

static void
texture(struct chck_atlas_texture *texture, uint32_t width, uint32_t height)
{
   assert(texture);
   memset(texture, 0, sizeof(struct chck_atlas_texture));
   texture->rect.w = width;
   texture->rect.h = height;
   texture->area = width * height;
   texture->longest_edge = (width >= height ? width : height);
}

static void
texture_place(struct chck_atlas_texture *texture, const uint32_t x, const uint32_t y, const bool flipped)
{
   assert(texture);
   texture->rect.x = x;
   texture->rect.y = y;
   texture->flipped = flipped;
   texture->placed = true;
}

static bool
node_fits(struct chck_atlas_node *node, uint32_t width, uint32_t height, uint32_t *out_edge_count)
{
   assert(node && out_edge_count);

   uint32_t ec = 0;
   if (width == node->rect.w) ec += (height == node->rect.w ? 2 : 1);
   else if (width == node->rect.h) ec += (height == node->rect.w  ? 2 : 1);
   else if (height == node->rect.w) ec++;
   else if (height == node->rect.h) ec++;

   *out_edge_count = ec;
   return ((width <= node->rect.w && height <= node->rect.w) || (height <= node->rect.h && width <= node->rect.h));
}

static bool
node_merge(struct chck_atlas_node *n1, struct chck_atlas_node *n2)
{
   assert(n1 && n2);
   bool ret = false;

   /* if we share the top edge then.. */
   if (n1->rect.x == n2->rect.x && n1->rect.w == n2->rect.w && n1->rect.y == n2->rect.y + n2->rect.h) {
      n1->rect.y = n2->rect.y;
      n1->rect.h += n2->rect.h;
      ret = true;
   }
   /* if we share the bottom edge  */
   else if (n1->rect.x == n2->rect.x && n1->rect.w == n2->rect.w && n1->rect.y + n1->rect.h == n2->rect.y) {
      n1->rect.h += n2->rect.h;
      ret = true;
   }
   /* if we share the left edge */
   else if (n1->rect.y == n2->rect.y && n1->rect.y + n1->rect.h == n2->rect.y && n1->rect.x == n2->rect.x + n2->rect.w) {
      n1->rect.x = n2->rect.x;
      n1->rect.w += n2->rect.w;
      ret = true;
   }
   /* if we share the left edge */
   else if (n1->rect.y == n2->rect.y && n1->rect.y + n1->rect.h == n2->rect.y && n1->rect.x + n1->rect.w == n2->rect.x) {
      n1->rect.w += n2->rect.w;
      ret = true;
   }

   return ret;
}

static bool
atlas_node_add(struct chck_atlas *atlas, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
   assert(atlas);

   struct chck_atlas_node *node;
   if (!(node = calloc(1, sizeof(struct chck_atlas_node))))
      return false;

   node->rect.x = x;
   node->rect.y = y;
   node->rect.w = width;
   node->rect.h = height;
   node->next = atlas->free_list;
   atlas->free_list = node;
   return true;
}

static bool
merge_nodes(struct chck_atlas *atlas)
{
   assert(atlas);

   struct chck_atlas_node *f = atlas->free_list;
   while (f) {
      struct chck_atlas_node *c = atlas->free_list, *prev = NULL;
      while (c) {
         if (f != c) {
            if (node_merge(f, c) && prev) {
               prev->next = c->next;
               free(c);
               return true;
            }
         }
         prev = c;
         c = c->next;
      }
      f = f->next;
   }

   return false;
}

static void
check_dimensions(struct chck_atlas *atlas, uint32_t *in_out_w, uint32_t *in_out_h)
{
   assert(atlas && in_out_w && in_out_h);
   for (uint32_t i = 0; i != atlas->count; ++i) {
      const struct chck_atlas_texture *t = &atlas->textures[i];
      if (t->rect.w > *in_out_w) *in_out_w = t->rect.w;
      if (t->rect.h > *in_out_h) *in_out_h = t->rect.h;
   }
}

bool
chck_atlas(struct chck_atlas *atlas)
{
   assert(atlas);
   memset(atlas, 0, sizeof(struct chck_atlas));
   return true;
}

void
chck_atlas_release(struct chck_atlas *atlas)
{
   if (!atlas)
      return;

   free(atlas->textures);

   if (atlas->free_list) {
      struct chck_atlas_node *next = atlas->free_list, *kill;
      while (next) {
         kill = next;
         next = next->next;
         free(kill);
      }
   }

   memset(atlas, 0, sizeof(chck_atlas));
}

uint32_t
chck_atlas_push(struct chck_atlas *atlas, uint32_t width, uint32_t height)
{
   assert(atlas);

   void *tmp;
   if (!(tmp = realloc(atlas->textures, (atlas->count + 1) * sizeof(struct chck_atlas_texture))))
      return atlas->count;

   atlas->textures = tmp;
   atlas->count += 1;

   texture(&atlas->textures[atlas->count - 1], width, height);

   if (width > atlas->longest_edge)
      atlas->longest_edge = width;

   if (height > atlas->longest_edge)
      atlas->longest_edge = height;

   atlas->total_area += width * height;
   return atlas->count;
}

uint32_t
chck_atlas_pop(struct chck_atlas *atlas)
{
   assert(atlas);

   if (atlas->count <= 0)
      return atlas->count;

   struct chck_atlas_texture *t = &atlas->textures[atlas->count - 1];
   atlas->total_area -= t->rect.w * t->rect.h;

   void *tmp = NULL;
   if (atlas->count > 1) {
      if (!(tmp = realloc(atlas->textures, (atlas->count - 1) * sizeof(struct chck_atlas_texture))))
         return atlas->count;
   } else {
      free(atlas->textures);
   }

   atlas->textures = tmp;
   atlas->count -= 1;
   return atlas->count;
}

const struct chck_atlas_texture*
chck_atlas_get(const struct chck_atlas *atlas, uint32_t index, struct chck_atlas_rect *out_transformed)
{
   assert(atlas && index > 0);

   if (out_transformed)
      memset(out_transformed, 0, sizeof(struct chck_atlas_rect));

   if (index - 1 >= atlas->count)
      return NULL;

   const struct chck_atlas_texture *t = &atlas->textures[index - 1];
   if (out_transformed) {
      out_transformed->x = t->rect.x;
      out_transformed->y = t->rect.y;
      out_transformed->w = (t->flipped ? t->rect.h : t->rect.w);
      out_transformed->h = (t->flipped ? t->rect.w : t->rect.h);
   }
   return t;
}

static inline uint32_t
next_pow_2(uint32_t v)
{
   if (v && !(v & (v - 1)))
      return v;

   uint32_t p;
   for (p = 1; p < v; p = p * 2);
   return p;
}

uint32_t
chck_atlas_pack(struct chck_atlas *atlas, bool force_pot, bool one_px_border, uint32_t *out_w, uint32_t *out_h)
{
   assert(atlas);

   if (one_px_border) {
      for (uint32_t i = 0; i != atlas->count; ++i) {
         struct chck_atlas_texture *t = &atlas->textures[i];
         t->rect.w += 2;
         t->rect.h += 2;
      }
      atlas->longest_edge += 2;
   }

   if (force_pot)
      atlas->longest_edge = next_pow_2(atlas->longest_edge);

   uint32_t width = atlas->longest_edge;
   uint32_t count = (uint32_t)((float)atlas->total_area/(atlas->longest_edge * atlas->longest_edge) + 0.5f);
   uint32_t height = (count + 2) * atlas->longest_edge;

   if (force_pot)
      height = next_pow_2(height);

   if (width > height && height != width * 0.5f) {
      height = width * 0.5f;
      width *= 0.5f;
      check_dimensions(atlas, &width, &height);
   } else if (height > width && width != height * 0.5f) {
      width = height * 0.5f;
      height *= 0.5f;
      check_dimensions(atlas, &width, &height);
   }

   atlas_node_add(atlas, 0, 0, width, height);

   for (uint32_t i = 0; i != atlas->count; ++i) {
      uint32_t longest_edge = 0, most_area = 0, index = 0;
      for(uint32_t i2 = 0; i2 != atlas->count; ++i2) {
         struct chck_atlas_texture *t = &atlas->textures[i2];
         if (t->placed)
            continue;

         if (t->longest_edge > longest_edge) {
            most_area = t->area;
            longest_edge = t->longest_edge;
            index = i2;
         } else if (t->longest_edge == longest_edge && t->area > most_area) {
            most_area = t->area;
            index = i2;
         }
      }

      uint32_t edge_count = 0;
      uint32_t least_y = UINT_MAX;
      uint32_t least_x = UINT_MAX;
      struct chck_atlas_texture *t = &atlas->textures[index];
      struct chck_atlas_node *previous_best_fit = NULL;
      struct chck_atlas_node *best_fit = NULL;
      struct chck_atlas_node *previous = NULL;
      struct chck_atlas_node *search = atlas->free_list;

      while (search) {
         uint32_t ec;
         if (node_fits(search, t->rect.w, t->rect.h, &ec)) {
            if (ec == 2) {
               previous_best_fit = previous;
               best_fit = search;
               edge_count = ec;
               break;
            } else if (search->rect.y < least_y) {
               least_y = search->rect.y;
               least_x = search->rect.x;
               previous_best_fit = previous;
               best_fit = search;
               edge_count = ec;
            } else if (search->rect.y == least_y && search->rect.x < least_x) {
               least_x = search->rect.x;
               previous_best_fit = previous;
               best_fit = search;
               edge_count = ec;
            }
         }
         previous = search;
         search = search->next;
      }

      /* should always find a fit */
      assert(best_fit);

      /* bail out on NDEBUG */
      if (!best_fit)
         goto out;

      switch (edge_count) {
         case 0:
            if (t->longest_edge <= best_fit->rect.w) {
               bool flipped = false;
               uint32_t width = t->rect.w;
               uint32_t height = t->rect.h;

               if (width > height) {
                  width = t->rect.h;
                  height = t->rect.w;
                  flipped  = 1;
               }

               texture_place(t, best_fit->rect.x, best_fit->rect.y, flipped);
               atlas_node_add(atlas, best_fit->rect.x, best_fit->rect.y + height, best_fit->rect.w, best_fit->rect.h - height);

               best_fit->rect.x += width;
               best_fit->rect.w -= width;
               best_fit->rect.h = height;
            } else {
               assert(t->longest_edge <= best_fit->rect.h);
               bool flipped  = false;
               uint32_t width = t->rect.w;
               uint32_t height = t->rect.h;

               if (width > height) {
                  width = t->rect.h;
                  height = t->rect.w;
                  flipped = true;
               }

               texture_place(t, best_fit->rect.x, best_fit->rect.y, flipped);
               atlas_node_add(atlas, best_fit->rect.x + width, best_fit->rect.y, best_fit->rect.w - width, best_fit->rect.h);

               best_fit->rect.y += height;
               best_fit->rect.h -= height;
               best_fit->rect.w = width;
            }
            break;
         case 1:
            if (t->rect.w == best_fit->rect.w) {
               texture_place(t, best_fit->rect.x, best_fit->rect.y, 0);
               best_fit->rect.y += t->rect.h;
               best_fit->rect.h -= t->rect.h;
            } else if (t->rect.h == best_fit->rect.h) {
               texture_place(t, best_fit->rect.x, best_fit->rect.y, 0);
               best_fit->rect.x += t->rect.w;
               best_fit->rect.w -= t->rect.w;
            } else if (t->rect.w == best_fit->rect.h) {
               texture_place(t, best_fit->rect.x, best_fit->rect.y, 1);
               best_fit->rect.x += t->rect.h;
               best_fit->rect.w -= t->rect.h;
            } else if (t->rect.h == best_fit->rect.w) {
               texture_place(t, best_fit->rect.x, best_fit->rect.y, 1);
               best_fit->rect.y += t->rect.w;
               best_fit->rect.h -= t->rect.w;
            }
            break;
         case 2:
            {
               const bool flipped = (t->rect.w != best_fit->rect.w || t->rect.h != best_fit->rect.h);
               texture_place(t, best_fit->rect.x, best_fit->rect.y, flipped);

               if (previous_best_fit) {
                  previous_best_fit->next = best_fit->next;
               } else {
                  atlas->free_list = best_fit->next;
               }

               if (best_fit)
                  free(best_fit);
            }
            break;
      }

      /* merge as much as we can */
      while (merge_nodes(atlas));
   }

   width = 0, height = 0;
   for(uint32_t i = 0; i < atlas->count; ++i) {
      struct chck_atlas_texture *t = &atlas->textures[i];
      if (one_px_border) {
         t->rect.w -= 2;
         t->rect.h -= 2;
         t->rect.x += 1;
         t->rect.y += 1;
      }

      const uint32_t x = (t->flipped ? t->rect.x + t->rect.h : t->rect.x + t->rect.w);
      const uint32_t y = (t->flipped ? t->rect.y + t->rect.w : t->rect.y + t->rect.h);
      width = (x > width ? x : width);
      height = (y > height ? y : height);
   }

   if (force_pot) {
      width = next_pow_2(width);
      height = next_pow_2(height);
   }

out:
   if (out_w) *out_w = width;
   if (out_h) *out_h = height;
   return (width * height) - atlas->total_area;
}
