// This file only includes the impl for stb_image.
#include <GL/gl.h>
#include <stdbool.h>

#include "texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct gf_texture_map_item {
  bool initialized;
  char *img_path;
  struct gf_texture texture;
};

static struct gf_texture_map_item texture_map[] = {
    [GF_TEXTURE_FACTORIO_ICON]    = {.initialized = false,
                                     .img_path    = "static/factorio-icon.png"},
    [GF_TEXTURE_CHARACTER_IDLE_1] = {.initialized = false,
                                     .img_path =
                                         "static/character/level1_idle.png"},
    [GF_TEXTURE_GRASS_1]          = {.initialized = false,
                                     .img_path    = "static/grass-1.png"}};

void gf_stbi_setup() {
  stbi_set_flip_vertically_on_load(true);
}

static void gf_texture_load(struct gf_texture_map_item *texture_map_item) {
  struct gf_texture *texture = &texture_map_item->texture;

  // Texture bit depth is always 8.
  texture->bit_depth = 8;
  u_int8_t *img_data = stbi_load(texture_map_item->img_path, &texture->width,
                                 &texture->height, &texture->num_channels, 0);
  assert(img_data != NULL);
  assert(texture->num_channels == 4);
  glCreateTextures(GL_TEXTURE_2D, 1, &texture->gl_name);
  glTextureParameteri(texture->gl_name, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(texture->gl_name, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTextureParameteri(texture->gl_name, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTextureParameteri(texture->gl_name, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTextureStorage2D(texture->gl_name, 1, GL_RGBA8, texture->width,
                     texture->height);
  glTextureSubImage2D(texture->gl_name, 0, 0, 0, texture->width,
                      texture->height, GL_RGBA, GL_UNSIGNED_BYTE, img_data);

  stbi_image_free(img_data);
  texture_map_item->initialized = true;
}

//! Lazy loads the given texture type.
const struct gf_texture *gf_texture_get(enum gf_texture_type type) {
  struct gf_texture_map_item *texture_map_item = &texture_map[type];
  if (!texture_map_item->initialized) {
    gf_texture_load(texture_map_item);
  }
  return &texture_map_item->texture;
}
