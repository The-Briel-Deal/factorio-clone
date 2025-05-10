#include <assert.h>
#include <stdbool.h>

#include "background.h"
#include "gf_math.h"


static void test_background_visible_chunks() {
  struct gf_background background;
  vec2s chunk;
  int chunks_visible;

  background = (struct gf_background){
      .tile_state.tile_size = 64,
      .viewport_bounds      = {
               .bottom = 0.0f, .left = 0.0f, .right = 800.0f, .top = 600.0f}};
  chunk = gf_background_first_visible_chunk(&background);
  assert(chunk.x == 0.0f);
  assert(chunk.y == 0.0f);

  chunks_visible = gf_background_visible_chunk_count(&background);
  // Chunks are 8x8 tiles and tiles are 64x64px so 512x512px for a whole chunk.
  // That means we need at least 4 (2 * 2) chunks to fill the whole 800x600
  // screen.
  assert(chunks_visible == 4);


  background = (struct gf_background){
      .tile_state.tile_size = 64,
      .viewport_bounds      = {
               .bottom = -1.3f, .left = 0.0f, .right = 800.0f, .top = 600.0f}};
  chunk = gf_background_first_visible_chunk(&background);
  assert(chunk.x == (0.0f * GF_CHUNK_TILE_WIDTH));
  assert(chunk.y == (-64.0f * GF_CHUNK_TILE_HEIGHT));

  chunks_visible = gf_background_visible_chunk_count(&background);
  assert(chunks_visible == 4);

  background = (struct gf_background){
      .tile_state.tile_size = 64,
      .viewport_bounds      = {.bottom = -1.3f,
                               .left   = (64.0f * GF_CHUNK_TILE_WIDTH) - 0.0001f,
                               .right  = 800.0f,
                               .top    = 512.0f}};
  chunk = gf_background_first_visible_chunk(&background);
  assert(chunk.x == (0.0f * GF_CHUNK_TILE_WIDTH));
  assert(chunk.y == (-64.0f * GF_CHUNK_TILE_HEIGHT));

  // We need around half of a chunk to fill left->right, and we need just barely
  // over 1 chunk to from bottom->top. So once we round up it should be 1x2
  chunks_visible = gf_background_visible_chunk_count(&background);
  assert(chunks_visible == 2);

  background = (struct gf_background){
      .tile_state.tile_size = 64,
      .viewport_bounds      = {.bottom = -1.3f,
                               .left   = (64.0f * GF_CHUNK_TILE_WIDTH) + 0.0001f,
                               .right  = 800.0f,
                               .top    = 600.0f}};
  chunk = gf_background_first_visible_chunk(&background);
  assert(chunk.x == (64.0f * GF_CHUNK_TILE_WIDTH));
  assert(chunk.y == (-64.0f * GF_CHUNK_TILE_HEIGHT));

  chunks_visible = gf_background_visible_chunk_count(&background);
  assert(chunks_visible == 2);
}

void run_gf_background_tests() {
  test_background_visible_chunks();
}
