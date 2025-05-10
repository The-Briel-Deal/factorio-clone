#ifndef GF_BACKGROUND_H
#define GF_BACKGROUND_H

#include <GL/gl.h>
#include <stdbool.h>

#include "gf_math.h"

#define GF_CHUNK_TILE_WIDTH  8
#define GF_CHUNK_TILE_HEIGHT 8

struct gf_background;

struct gf_background *gf_background_create();

void gf_background_draw(struct gf_background *background);

// The definitions below only exist for testing. Please do not use these.

struct gf_quad_bounds {
  float top;
  float bottom;
  float left;
  float right;
};

typedef struct gf_quad_bounds gf_terrain_texture_pos;
typedef struct gf_quad_bounds gf_viewport_bounds;


struct gf_background {
  struct gf_obj *obj;
  GLuint tex_offset_attr_buf;
  uint8_t tile_tex_indices[GF_CHUNK_TILE_HEIGHT * GF_CHUNK_TILE_WIDTH];

  bool tile_state_dirty;
  GLuint tile_state_buf;
  struct gf_tile_state {
    int tile_size;
    int tiles_per_row;
  } tile_state;

  int tile_count;

	vec2s offset;
  gf_viewport_bounds viewport_bounds;
};

#ifdef GF_TESTING
vec2s gf_background_first_visible_chunk(const struct gf_background *background);
vec2s gf_background_visible_chunks(const struct gf_background *background);
vec2s gf_background_chunk_size(const struct gf_background *background);
#endif

#endif
