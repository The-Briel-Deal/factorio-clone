#ifndef GF_TEXTURE_H
#define GF_TEXTURE_H

#include <GL/gl.h>
#include <sys/types.h>

void gf_stbi_setup();

struct gf_texture {
  GLuint gl_name;
  int height;
  int width;
  int num_channels;
  int bit_depth;
};

enum gf_texture_type {
  GF_TEXTURE_FACTORIO_ICON,
  GF_TEXTURE_CHARACTER_IDLE_1,
  GF_TEXTURE_CHARACTER_IDLE_MASK_1,
  GF_TEXTURE_CHARACTER_RUNNING_1,
  GF_TEXTURE_CHARACTER_RUNNING_MASK_1,
  GF_TEXTURE_GRASS_1,
};

const struct gf_texture *gf_texture_get(enum gf_texture_type type);

#endif
