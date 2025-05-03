#include "common.h"
#include "gf_math.h"
#include "log.h"
#include "render.h"
#include "texture.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <stdint.h>

#define GF_BACKGROUND_ATTRIB_TEX_BINDING_INDEX  1

#define GF_BACKGROUND_ATTRIB_TEX_INDEX_LOCATION 2
#define GF_BACKGROUND_UNIFORM_TEX_MAP           5

#define GF_BACKGROUND_UBO_TILE_STATE            1

#define GF_BACKGROUND_DEFAULT_TILE_SIZE         128

#define GF_TERRAIN_LOW_RES_1                    0
#define GF_TERRAIN_LOW_RES_2                    1
#define GF_TERRAIN_LOW_RES_3                    2
#define GF_TERRAIN_LOW_RES_4                    3
#define GF_TERRAIN_LOW_RES_5                    4
#define GF_TERRAIN_LOW_RES_6                    5
#define GF_TERRAIN_LOW_RES_7                    6
#define GF_TERRAIN_LOW_RES_8                    7
#define GF_TERRAIN_LOW_RES_9                    8
#define GF_TERRAIN_LOW_RES_10                   9
#define GF_TERRAIN_LOW_RES_11                   10
#define GF_TERRAIN_LOW_RES_12                   11
#define GF_TERRAIN_LOW_RES_13                   12
#define GF_TERRAIN_LOW_RES_14                   13
#define GF_TERRAIN_LOW_RES_15                   14
#define GF_TERRAIN_LOW_RES_16                   15

// TODO: Rename `instance_attr_*` to something better.
struct gf_background {
  struct gf_obj *obj;
  GLuint tex_offset_attr_buf;
  uint8_t tile_tex_indices[1024];

  bool tile_state_dirty;
  GLuint tile_state_buf;
  struct gf_tile_state {
    int tile_size;
    int tiles_per_row;
  } tile_state;

  int tile_count;
};

STATIC_LIST(gf_background_list, struct gf_background, 128)

struct gf_terrain_texture_pos {
  float top;
  float bottom;
  float left;
  float right;
};

#define TEX_MAP_H  576
#define TEX_MAP_W  4096

// Normalize height and width to gl coords (0.0-1.0)
#define NH(height) ((float)height) / ((float)TEX_MAP_H)
#define NW(width)  ((float)width) / ((float)TEX_MAP_W)

static const struct gf_terrain_texture_pos texture_positions[] = {
    [GF_TERRAIN_LOW_RES_1]  = {.top    = NH(TEX_MAP_H),
                               .bottom = NH(TEX_MAP_H - 64),
                               .left   = NW(0 + (64 * 0)),
                               .right  = NW(64 + (64 * 0))},
    [GF_TERRAIN_LOW_RES_2]  = {.top    = NH(TEX_MAP_H),
                               .bottom = NH(TEX_MAP_H - 64),
                               .left   = NW(0 + (64 * 1)),
                               .right  = NW(64 + (64 * 1))},
    [GF_TERRAIN_LOW_RES_3]  = {.top    = NH(TEX_MAP_H),
                               .bottom = NH(TEX_MAP_H - 64),
                               .left   = NW(0 + (64 * 2)),
                               .right  = NW(64 + (64 * 2))},
    [GF_TERRAIN_LOW_RES_4]  = {.top    = NH(TEX_MAP_H),
                               .bottom = NH(TEX_MAP_H - 64),
                               .left   = NW(0 + (64 * 3)),
                               .right  = NW(64 + (64 * 3))},
    [GF_TERRAIN_LOW_RES_5]  = {.top    = NH(TEX_MAP_H),
                               .bottom = NH(TEX_MAP_H - 64),
                               .left   = NW(0 + (64 * 4)),
                               .right  = NW(64 + (64 * 4))},
    [GF_TERRAIN_LOW_RES_6]  = {.top    = NH(TEX_MAP_H),
                               .bottom = NH(TEX_MAP_H - 64),
                               .left   = NW(0 + (64 * 5)),
                               .right  = NW(64 + (64 * 5))},
    [GF_TERRAIN_LOW_RES_7]  = {.top    = NH(TEX_MAP_H),
                               .bottom = NH(TEX_MAP_H - 64),
                               .left   = NW(0 + (64 * 6)),
                               .right  = NW(64 + (64 * 6))},
    [GF_TERRAIN_LOW_RES_8]  = {.top    = NH(TEX_MAP_H),
                               .bottom = NH(TEX_MAP_H - 64),
                               .left   = NW(0 + (64 * 7)),
                               .right  = NW(64 + (64 * 7))},
    [GF_TERRAIN_LOW_RES_9]  = {.top    = NH(TEX_MAP_H),
                               .bottom = NH(TEX_MAP_H - 64),
                               .left   = NW(0 + (64 * 8)),
                               .right  = NW(64 + (64 * 8))},
    [GF_TERRAIN_LOW_RES_10] = {.top    = NH(TEX_MAP_H),
                               .bottom = NH(TEX_MAP_H - 64),
                               .left   = NW(0 + (64 * 9)),
                               .right  = NW(64 + (64 * 9))},
    [GF_TERRAIN_LOW_RES_11] = {.top    = NH(TEX_MAP_H),
                               .bottom = NH(TEX_MAP_H - 64),
                               .left   = NW(0 + (64 * 10)),
                               .right  = NW(64 + (64 * 10))},
    [GF_TERRAIN_LOW_RES_12] = {.top    = NH(TEX_MAP_H),
                               .bottom = NH(TEX_MAP_H - 64),
                               .left   = NW(0 + (64 * 11)),
                               .right  = NW(64 + (64 * 11))},
    [GF_TERRAIN_LOW_RES_13] = {.top    = NH(TEX_MAP_H),
                               .bottom = NH(TEX_MAP_H - 64),
                               .left   = NW(0 + (64 * 12)),
                               .right  = NW(64 + (64 * 12))},
    [GF_TERRAIN_LOW_RES_14] = {.top    = NH(TEX_MAP_H),
                               .bottom = NH(TEX_MAP_H - 64),
                               .left   = NW(0 + (64 * 13)),
                               .right  = NW(64 + (64 * 13))},
    [GF_TERRAIN_LOW_RES_15] = {.top    = NH(TEX_MAP_H),
                               .bottom = NH(TEX_MAP_H - 64),
                               .left   = NW(0 + (64 * 14)),
                               .right  = NW(64 + (64 * 14))},
    [GF_TERRAIN_LOW_RES_16] = {.top    = NH(TEX_MAP_H),
                               .bottom = NH(TEX_MAP_H - 64),
                               .left   = NW(0 + (64 * 15)),
                               .right  = NW(64 + (64 * 15))},
};

bool gf_background_commit_state(struct gf_background *background) {
  bool set = false;
  if (background->tile_state_dirty) {
    gf_obj_set_scale(background->obj,
                     (tf_scale){background->tile_state.tile_size,
                                background->tile_state.tile_size});
    float starting_position = background->tile_state.tile_size / 2.0f;
    gf_obj_set_pos(background->obj,
                   (tf_pos){starting_position, starting_position});
    gf_obj_update_buffer_data(background->tex_offset_attr_buf,
                              background->tile_tex_indices,
                              sizeof(background->tile_tex_indices));
    gf_obj_update_buffer_data(background->tile_state_buf,
                              &background->tile_state,
                              sizeof(background->tile_state));

    background->tile_state_dirty = false;
    set                          = true;
  }
  gf_obj_commit_state(background->obj);

  return set;
}

static void gf_background_set_tile_size(struct gf_background *background,
                                        int tile_size) {
  background->tile_state.tile_size = tile_size;
  background->tile_state_dirty     = true;
}
static void gf_background_init_tiles(struct gf_background *background) {
  for (int i = 0; i < sizeof(background->tile_tex_indices); i++) {
    background->tile_tex_indices[i] = i % 16;
  }
  background->tile_state_dirty = true;
}

struct gf_background *gf_background_create() {
  if (gf_background_list.count + 1 >= gf_background_list.capacity) {
    gf_log(DEBUG_LOG,
           "`gf_background_list` has a count of '%i', which is greater than "
           "it's capacity of '%i'.",
           gf_background_list.count, gf_background_list.capacity);
    return NULL;
  }
  struct gf_background *background =
      &gf_background_list.items[gf_background_list.count++];
  gf_background_set_tile_size(background, GF_BACKGROUND_DEFAULT_TILE_SIZE);
  gf_background_init_tiles(background);

  background->obj          = gf_obj_create_box();
  struct gf_shader *shader = gf_shader_create_from_paths(
      "shader/background_vert.glsl", "shader/background_frag.glsl");
  gf_obj_set_shader(background->obj, shader);
  gf_obj_set_texture(background->obj, "backgroundTex", GF_TEXTURE_GRASS_1);
  gf_obj_set_vec4v(background->obj, GF_BACKGROUND_UNIFORM_TEX_MAP,
                   sizeof(texture_positions) / sizeof(*texture_positions),
                   (void *)texture_positions);
  background->tex_offset_attr_buf = gf_obj_create_attr(
      background->obj, GF_BACKGROUND_ATTRIB_TEX_BINDING_INDEX,
      GF_BACKGROUND_ATTRIB_TEX_INDEX_LOCATION, GL_BYTE);
  gf_obj_set_binding_divisor(background->obj,
                             GF_BACKGROUND_ATTRIB_TEX_BINDING_INDEX, 1);

  background->tile_state_buf =
      gf_obj_create_ubo(background->obj, sizeof(background->tile_state),
                        "TileState", GF_BACKGROUND_UBO_TILE_STATE);

  gf_background_commit_state(background);

  return background;
}

static void gf_background_sync_tiles(struct gf_background *background) {
  const int tile_size = background->tile_state.tile_size;
  assert(tile_size != 0);
  const struct viewport_dimensions *viewport = gf_render_get_window_size();
  // Add 1 to height and width to offset int division rounding down.
  int width  = (viewport->width / tile_size) + 1;
  int height = (viewport->height / tile_size) + 1;

  int tiles_to_fill_screen = width * height;
  if (background->tile_state.tiles_per_row != width ||
      background->tile_count != tiles_to_fill_screen) {
    background->tile_state.tiles_per_row = width;
    background->tile_count               = tiles_to_fill_screen;
    background->tile_state_dirty         = true;
  }
}

void gf_background_draw(struct gf_background *background) {
  gf_background_sync_tiles(background);
  gf_background_commit_state(background);
  gf_obj_draw_instanced(background->obj, background->tile_count);
#ifdef GF_DEBUG_DRAW
  gf_obj_set_int_by_name(background->obj, "debug", true);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  gf_obj_draw_instanced(background->obj, background->tile_count);
  gf_obj_set_int_by_name(background->obj, "debug", false);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
}
