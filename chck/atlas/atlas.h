#ifndef __chck_atlas_h__
#define __chck_atlas_h__

#include <stdint.h>
#include <stdbool.h>

struct chck_atlas_rect {
   uint32_t x, y, w, h;
};

struct chck_atlas_texture {
   struct chck_atlas_rect rect;

   // area (w * h)
   uint32_t area;

   // longest edge (w > h : h > w)
   uint32_t longest_edge;

   // is texture flipped (90 degrees) and/or placed
   bool flipped, placed;
};

struct chck_atlas_node {
   struct chck_atlas_node *next;
   struct chck_atlas_rect rect;
};

struct chck_atlas {
   // which nodes are not yet placed
   struct chck_atlas_node *free_list;

   // textures in this atlas
   struct chck_atlas_texture *textures;
   uint32_t count;

   // longest edge (w > h : h > w)
   uint32_t longest_edge;

   // total area (w * h * textures)
   uint32_t total_area;
};

bool chck_atlas(struct chck_atlas *atlas);
void chck_atlas_release(struct chck_atlas *atlas);
uint32_t chck_atlas_push(struct chck_atlas *atlas, uint32_t width, uint32_t height);
uint32_t chck_atlas_pop(struct chck_atlas *atlas);
const struct chck_atlas_texture* chck_atlas_get(const struct chck_atlas *atlas, uint32_t index, struct chck_atlas_rect *out_transformed);

// returns total unusued area
uint32_t chck_atlas_pack(struct chck_atlas *atlas, bool force_pot, bool one_px_border, uint32_t *out_w, uint32_t *out_h);

#endif /* __chck_atlas_h__ */
