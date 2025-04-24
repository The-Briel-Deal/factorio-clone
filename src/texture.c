// This file only includes the impl for stb_image.
#include <stdbool.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void gf_stbi_setup() {
  stbi_set_flip_vertically_on_load(true);
}
