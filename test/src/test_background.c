
#include <assert.h>
#include <stdbool.h>

#include "background.h"
#include "gf_math.h"


static void test_background_first_visible_chunk() {
  struct gf_background background;
  vec2s chunk;

  background = (struct gf_background){
      .tile_state.tile_size = 64,
      .viewport_bounds      = {
               .bottom = 0.0f, .left = 0.0f, .right = 800.0f, .top = 600.0f}};
  chunk = gf_background_first_visible_chunk(&background);
  assert(chunk.x == 0.0f);
  assert(chunk.y == 0.0f);


  background = (struct gf_background){
      .tile_state.tile_size = 64,
      .viewport_bounds      = {
               .bottom = -1.3f, .left = 0.0f, .right = 800.0f, .top = 600.0f}};
  chunk = gf_background_first_visible_chunk(&background);
  assert(chunk.x == (0.0f * GF_CHUNK_TILE_WIDTH));
  assert(chunk.y == (-64.0f * GF_CHUNK_TILE_HEIGHT));

  background = (struct gf_background){
      .tile_state.tile_size = 64,
      .viewport_bounds      = {
               .bottom = -1.3f, .left = (64.0f * GF_CHUNK_TILE_WIDTH) - 0.0001f, .right = 800.0f, .top = 600.0f}};
  chunk = gf_background_first_visible_chunk(&background);
  assert(chunk.x == (0.0f * GF_CHUNK_TILE_WIDTH));
  assert(chunk.y == (-64.0f * GF_CHUNK_TILE_HEIGHT));


  background = (struct gf_background){
      .tile_state.tile_size = 64,
      .viewport_bounds      = {
               .bottom = -1.3f, .left = (64.0f * GF_CHUNK_TILE_WIDTH) + 0.0001f, .right = 800.0f, .top = 600.0f}};
  chunk = gf_background_first_visible_chunk(&background);
  assert(chunk.x == (64.0f * GF_CHUNK_TILE_WIDTH));
  assert(chunk.y == (-64.0f * GF_CHUNK_TILE_HEIGHT));
}

void run_gf_background_tests() {
  test_background_first_visible_chunk();
}
