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
#ifndef GF_TESTING
#define STATIC_UNLESS_TEST static
#else
#define STATIC_UNLESS_TEST
#endif
