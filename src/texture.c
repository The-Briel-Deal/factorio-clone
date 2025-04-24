// This file only includes the impl for stb_image.
#include <GL/gl.h>
#include <stdbool.h>

#include "texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void gf_stbi_setup() {
  stbi_set_flip_vertically_on_load(true);
}

void gf_load_texture(struct gf_texture *texture, char *img_path) {
  texture->bit_depth = 8;
  u_int8_t *img_data = stbi_load(img_path, &texture->width, &texture->height,
                                 &texture->num_channels, 0);
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
}
