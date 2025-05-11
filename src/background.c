#include "background.h"
#include "common.h"
#include "gf_math.h"
#include "log.h"
#include "noise.h"
#include "render.h"
#include "texture.h"

#include <GL/gl.h>
#include <GL/glext.h>
#include <assert.h>
#include <math.h>
#include <stdint.h>


#define GF_BACKGROUND_ATTRIB_TEX_BINDING_INDEX 1

#define GF_BACKGROUND_UBO_TILE_STATE           1

#define GF_BACKGROUND_DEFAULT_TILE_SIZE        128

#define GF_TERRAIN_LOW_RES_1                   0
#define GF_TERRAIN_LOW_RES_2                   1
#define GF_TERRAIN_LOW_RES_3                   2
#define GF_TERRAIN_LOW_RES_4                   3
#define GF_TERRAIN_LOW_RES_5                   4
#define GF_TERRAIN_LOW_RES_6                   5
#define GF_TERRAIN_LOW_RES_7                   6
#define GF_TERRAIN_LOW_RES_8                   7
#define GF_TERRAIN_LOW_RES_9                   8
#define GF_TERRAIN_LOW_RES_10                  9
#define GF_TERRAIN_LOW_RES_11                  10
#define GF_TERRAIN_LOW_RES_12                  11
#define GF_TERRAIN_LOW_RES_13                  12
#define GF_TERRAIN_LOW_RES_14                  13
#define GF_TERRAIN_LOW_RES_15                  14
#define GF_TERRAIN_LOW_RES_16                  15


STATIC_LIST(gf_background_list, struct gf_background, 128)


#define TEX_MAP_H  576
#define TEX_MAP_W  4096

// Normalize height and width to gl coords (0.0-1.0)
#define NH(height) ((float)height) / ((float)TEX_MAP_H)
#define NW(width)  ((float)width) / ((float)TEX_MAP_W)

static const gf_terrain_texture_pos texture_positions[] = {
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

static void gf_background_sync_viewport(struct gf_background *background) {
  const struct viewport_dimensions *viewport_size = gf_render_get_window_size();
  // TODO: Once I center camera on player, we need to take the camera pos into
  // account.
  background->viewport_bounds = gf_camera_get_bounds();
}

//! The size of a chunk in world coords.
STATIC_UNLESS_TEST vec2s
gf_background_chunk_size(const struct gf_background *background) {
  // Since tiles are squares, `tile_size` is just the size of either side.
  int tile_size    = background->tile_state.tile_size;
  vec2s chunk_size = {tile_size * GF_CHUNK_TILE_WIDTH,
                      tile_size * GF_CHUNK_TILE_HEIGHT};
  return chunk_size;
}

STATIC_UNLESS_TEST vec2s
gf_background_first_visible_chunk(const struct gf_background *background) {
  gf_viewport_bounds viewport_bounds = background->viewport_bounds;
  assert(viewport_bounds.top > viewport_bounds.bottom);
  assert(viewport_bounds.right > viewport_bounds.left);

  vec2s viewport_bottom_left = {viewport_bounds.left, viewport_bounds.bottom};
  vec2s chunk_size           = gf_background_chunk_size(background);

  // Divide viewport by chunk size.
  viewport_bottom_left.x /= chunk_size.x;
  viewport_bottom_left.y /= chunk_size.y;

  // Floor viewport to get to nearest chunk.
  viewport_bottom_left.x = floorf(viewport_bottom_left.x);
  viewport_bottom_left.y = floorf(viewport_bottom_left.y);

  // Size viewport back up to world pos.
  viewport_bottom_left.x *= chunk_size.x;
  viewport_bottom_left.y *= chunk_size.y;

  return viewport_bottom_left;
}

STATIC_UNLESS_TEST vec2s
gf_background_visible_chunks(const struct gf_background *background) {
  gf_viewport_bounds viewport_bounds = background->viewport_bounds;
  vec2s chunk_size                   = gf_background_chunk_size(background);
  // We need to round up because we may only see a portion of a chunk on the
  // edges.
  float chunks_wide =
      ceilf((viewport_bounds.right - viewport_bounds.left) / chunk_size.x);
  float chunks_high =
      ceilf((viewport_bounds.top - viewport_bounds.bottom) / chunk_size.y);

  vec2s chunks_in_dirs = (vec2s){chunks_wide + 1, chunks_high + 1};


  return chunks_in_dirs;
}

bool gf_background_commit_state(struct gf_background *background) {
  bool set = false;
  if (background->tile_state_dirty) {
    gf_background_sync_viewport(background);
    gf_obj_set_scale(background->obj,
                     (tf_scale){background->tile_state.tile_size,
                                background->tile_state.tile_size});
    float starting_position = background->tile_state.tile_size / 2.0f;
    gf_obj_set_pos(background->obj,
                   (tf_pos){starting_position + background->offset.x,
                            starting_position + background->offset.y});
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

static void gf_background_update_tiles(struct gf_background *background) {
  for (int i = 0; i < sizeof(background->tile_tex_indices); i++) {
    int col                         = i % background->tile_state.tiles_per_row;
    int row                         = i / background->tile_state.tiles_per_row;
    background->tile_tex_indices[i] = (uint8_t)floorf(
        gf_noise(
            (vec2s){
                .x = col + (background->offset.x /
                            ((float)background->tile_state.tile_size)),
                .y = row + (background->offset.y /
                            ((float)background->tile_state.tile_size)),
            },
            0.2) *
        15);
  }
  background->tile_state_dirty = true;
}

static void gf_background_update_offset(struct gf_background *background,
                                        vec2s offset) {
  background->offset           = offset;
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

  background->tile_state.tiles_per_row = GF_CHUNK_TILE_WIDTH;
  background->tile_state.tile_size     = GF_BACKGROUND_DEFAULT_TILE_SIZE;
  background->tile_count = GF_CHUNK_TILE_WIDTH * GF_CHUNK_TILE_HEIGHT;
  gf_background_update_offset(background, (vec2s){0.0f, 0.0f});
  gf_background_sync_viewport(background);

  gf_background_update_tiles(background);

  struct gf_shader *shader = gf_shader_create_from_paths(
      "shader/background_vert.glsl", "shader/background_frag.glsl");
  background->obj = gf_obj_create_quad(shader);
  gf_obj_set_shader(background->obj, shader);
  gf_obj_set_texture(background->obj, "backgroundTex", GF_TEXTURE_GRASS_1);
  gf_obj_set_vec4v(background->obj, "textureMap",
                   sizeof(texture_positions) / sizeof(*texture_positions),
                   (void *)texture_positions);
  background->tex_offset_attr_buf = gf_obj_create_attr(
      background->obj, GF_BACKGROUND_ATTRIB_TEX_BINDING_INDEX, "aTexIndex",
      GL_BYTE);
  gf_obj_set_binding_divisor(background->obj,
                             GF_BACKGROUND_ATTRIB_TEX_BINDING_INDEX, 1);

  background->tile_state_buf = gf_obj_create_ubo(
      background->obj, sizeof(background->tile_state), "TileState");

  gf_background_commit_state(background);

  return background;
}


void gf_background_draw(struct gf_background *background) {
  vec2s chunk_size  = gf_background_chunk_size(background);
  vec2s first_chunk = gf_background_first_visible_chunk(background);
  gf_log(INFO_LOG, "First Visible Chunk: (x = %f, y = %f)", first_chunk.x,
         first_chunk.y);
  vec2s visible_chunks = gf_background_visible_chunks(background);
  int chunk_count      = visible_chunks.x * visible_chunks.y;
  for (int i = 0; i < chunk_count; i++) {
    int col = i % (int)(visible_chunks.x);
    int row = i / (int)(visible_chunks.x);
    assert(row < visible_chunks.y);
    vec2s offset = {.x = (col * chunk_size.x) + first_chunk.x,
                    .y = (row * chunk_size.y) + first_chunk.y};
    gf_background_update_offset(background, offset);
    gf_background_update_tiles(background);
    gf_background_commit_state(background);
#ifdef GF_DEBUG_NOISE_VIS
    gf_obj_set_int(background->obj, "debug_noise_vis", true);
#endif
    gf_obj_draw_instanced(background->obj, background->tile_count);
#ifdef GF_DEBUG_NOISE_VIS
    gf_obj_set_int(background->obj, "debug_noise_vis", false);
#endif
#ifdef GF_DEBUG_DRAW
    gf_obj_set_int(background->obj, "debug", true);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    gf_obj_draw_instanced(background->obj, background->tile_count);
    gf_obj_set_int(background->obj, "debug", false);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
  }
}
