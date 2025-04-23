#ifndef GF_COMMON_H
#define GF_COMMON_H

#include <GL/gl.h>
#include <assert.h>
#include <sys/types.h>

#include "stb_image.h"

#define GF_DEFAULT_WINDOW_HEIGHT           600
#define GF_DEFAULT_WINDOW_WIDTH            800

#define GF_ATTRIB_VERT_LOCATION            0
#define GF_ATTRIB_TEX_COORD_LOCATION       1

#define GF_UNIFORM_PROJECTION_MAT_LOCATION 0
#define GF_UNIFORM_TRANSFORM_MAT_LOCATION  1
#define GF_UNIFORM_PLAYER_TEX_LOCATION     2

#define STR_HELPER(val)                    #val
#define TO_STR(val)                        STR_HELPER(val)

#define STATIC_LIST(name, type, size)                                          \
  struct name {                                                                \
    int count;                                                                 \
    int capacity;                                                              \
    type items[size];                                                          \
  };                                                                           \
                                                                               \
  static struct name name = {                                                  \
      .count    = 0,                                                           \
      .capacity = size,                                                        \
  };

void gf_stbi_setup();

static inline GLuint gf_load_texture(char *img_path) {
  GLuint texture;
  int width, height, nrChannels;
  u_int8_t *img_data = stbi_load(img_path, &width, &height, &nrChannels, 0);
  assert(img_data != NULL);
  assert(nrChannels == 4);
  glCreateTextures(GL_TEXTURE_2D, 1, &texture);
  glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTextureStorage2D(texture, 1, GL_RGBA8, width, height);
  glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA,
                      GL_UNSIGNED_BYTE, img_data);

  stbi_image_free(img_data);
  return texture;
}


#endif
