#include <assert.h>
#include <stdbool.h>

#include "background.h"
#include "gf_math.h"

static void assert_chunks_cover_viewport(gf_viewport_bounds viewport,
                                         vec2s chunks_in_dirs,
                                         vec2s first_chunk, vec2s chunk_size) {
  struct gf_quad_bounds chunks_bounds = {
      .right  = first_chunk.x + (chunks_in_dirs.x * chunk_size.x),
      .top    = first_chunk.y + (chunks_in_dirs.y * chunk_size.y),
      .left   = first_chunk.x,
      .bottom = first_chunk.y,
  };

  assert(viewport.right <= chunks_bounds.right);
  assert(viewport.top <= chunks_bounds.top);
  assert(viewport.left >= chunks_bounds.left);
  assert(viewport.bottom >= chunks_bounds.bottom);
}

static void test_background_visible_chunks() {
  struct gf_background background;
  vec2s chunk_size;
  vec2s first_chunk;
  vec2s chunks_visible;


  background = (struct gf_background){
      .tile_state.tile_size = 64,
      .viewport_bounds      = {
               .bottom = 0.0f, .left = 0.0f, .right = 800.0f, .top = 600.0f}};

  chunk_size = gf_background_chunk_size(&background);
  assert(chunk_size.x == 512);
  assert(chunk_size.y == 512);

  first_chunk = gf_background_first_visible_chunk(&background);
  assert(first_chunk.x == 0.0f);
  assert(first_chunk.y == 0.0f);

  chunks_visible = gf_background_visible_chunks(&background);
  // Chunks are 8x8 tiles and tiles are 64x64px so 512x512px for a whole chunk.
  // That means we need at least 4 (2 * 2) chunks to fill the whole 800x600
  // screen.
  assert(chunks_visible.x == 2);
  assert(chunks_visible.y == 2);
  assert_chunks_cover_viewport(background.viewport_bounds, chunks_visible,
                               first_chunk, chunk_size);


  background = (struct gf_background){
      .tile_state.tile_size = 64,
      .viewport_bounds      = {
               .bottom = -1.3f, .left = 0.0f, .right = 800.0f, .top = 600.0f}};

  chunk_size = gf_background_chunk_size(&background);
  assert(chunk_size.x == 512);
  assert(chunk_size.y == 512);

  first_chunk = gf_background_first_visible_chunk(&background);
  assert(first_chunk.x == (0.0f * GF_CHUNK_TILE_WIDTH));
  assert(first_chunk.y == (-64.0f * GF_CHUNK_TILE_HEIGHT));

  chunks_visible = gf_background_visible_chunks(&background);
  assert(chunks_visible.x == 2);
  assert(chunks_visible.y == 2);

  assert_chunks_cover_viewport(background.viewport_bounds, chunks_visible,
                               first_chunk, chunk_size);


  background = (struct gf_background){
      .tile_state.tile_size = 64,
      .viewport_bounds      = {.bottom = -1.3f,
                               .left   = (64.0f * GF_CHUNK_TILE_WIDTH) - 0.0001f,
                               .right  = 800.0f,
                               .top    = 512.0f}};

  chunk_size = gf_background_chunk_size(&background);
  assert(chunk_size.x == 512);
  assert(chunk_size.y == 512);

  first_chunk = gf_background_first_visible_chunk(&background);
  assert(first_chunk.x == (0.0f * GF_CHUNK_TILE_WIDTH));
  assert(first_chunk.y == (-64.0f * GF_CHUNK_TILE_HEIGHT));

  // We need around half of a chunk to fill left->right, and we need just barely
  // over 1 chunk to from bottom->top. So once we round up it should be 1x2
  chunks_visible = gf_background_visible_chunks(&background);
  assert(chunks_visible.x == 1);
  assert(chunks_visible.y == 2);

  assert_chunks_cover_viewport(background.viewport_bounds, chunks_visible,
                               first_chunk, chunk_size);


  background = (struct gf_background){
      .tile_state.tile_size = 64,
      .viewport_bounds      = {.bottom = -1.3f,
                               .left   = (64.0f * GF_CHUNK_TILE_WIDTH) + 0.0001f,
                               .right  = 800.0f,
                               .top    = 600.0f}};

  chunk_size = gf_background_chunk_size(&background);
  assert(chunk_size.x == 512);
  assert(chunk_size.y == 512);

  first_chunk = gf_background_first_visible_chunk(&background);
  assert(first_chunk.x == (64.0f * GF_CHUNK_TILE_WIDTH));
  assert(first_chunk.y == (-64.0f * GF_CHUNK_TILE_HEIGHT));

  chunks_visible = gf_background_visible_chunks(&background);
  assert(chunks_visible.x == 1);
  assert(chunks_visible.y == 2);

  assert_chunks_cover_viewport(background.viewport_bounds, chunks_visible,
                               first_chunk, chunk_size);

  background = (struct gf_background){
      .tile_state.tile_size = 64,
      .viewport_bounds      = {
               .bottom = 400.0f, .left = 400.0f, .right = 600.0f, .top = 600.0f}};

  chunk_size = gf_background_chunk_size(&background);
  assert(chunk_size.x == 512);
  assert(chunk_size.y == 512);

  first_chunk = gf_background_first_visible_chunk(&background);
  assert(first_chunk.x == 0.0f);
  assert(first_chunk.y == 0.0f);

  chunks_visible = gf_background_visible_chunks(&background);
  assert(chunks_visible.x == 1);
  assert(chunks_visible.y == 1);

  assert_chunks_cover_viewport(background.viewport_bounds, chunks_visible,
                               first_chunk, chunk_size);
}

void run_gf_background_tests() {
  test_background_visible_chunks();
}
