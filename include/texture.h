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

void gf_load_texture(struct gf_texture *texture, char *img_path);

#endif
