#include "common.h"
#include "gf_math.h"
#include "log.h"
#include "render.h"
#include "texture.h"
#include <GL/gl.h>
#define GF_BACKGROUND_ATTRIB_TEST_INDEX_LOCATION 2

#define GF_BACKGROUND_UNIFORM_TILE_SIZE          3
#define GF_BACKGROUND_UNIFORM_TILE_WIDTH         4

#define GF_BACKGROUND_DEFAULT_TILE_SIZE          512

struct gf_background {
  struct gf_obj *obj;
  GLuint instance_attr_buf_binding_index;
  GLuint instance_attr_buf_object;
  struct gf_background_state {
    bool dirty;
    int tile_size;
    int tiles_to_fill_screen;
    int tiles_per_row;
  } background_state;
};

STATIC_LIST(gf_background_list, struct gf_background, 128)

static const char *vert_shader_src =
    "#version 450 core\n"
    "\n"
    "layout (location = " TO_STR(GF_ATTRIB_VERT_LOCATION) ") in vec2 aPos;\n"
    "layout (location = " TO_STR(GF_ATTRIB_TEX_COORD_LOCATION) ") in vec2 aTexCoord;\n"
    "layout (location = " TO_STR(GF_BACKGROUND_ATTRIB_TEST_INDEX_LOCATION) ") in int aTestIndex;\n"
    "layout (location = " TO_STR(GF_UNIFORM_TRANSFORM_MAT_LOCATION) ") uniform mat4 model;\n"
    "layout (location = " TO_STR(GF_UNIFORM_PROJECTION_MAT_LOCATION) ") uniform mat4 projection;\n"
    "layout (location = " TO_STR(GF_BACKGROUND_UNIFORM_TILE_SIZE) ") uniform int tileSize;\n"
    "layout (location = " TO_STR(GF_BACKGROUND_UNIFORM_TILE_WIDTH) ") uniform int maxTileWidth;\n"
    "\n"
    "out vec2 texCoord;\n"
    "out vec4 testColor;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    int tileWidth = gl_InstanceID % (maxTileWidth);\n"
    "    int tileHeight = gl_InstanceID / (maxTileWidth);\n"
    "    vec4 tileOffset = vec4(tileSize * tileWidth, tileSize * tileHeight, 0, 0);\n"
         // We need to apply the offset after transformation occurs to prevent 
         // scaling shooting the tile offscreen.
    "    vec4 worldPosition = model * vec4(aPos, 0.0, 1.0);\n"
    "    vec4 offsetWorldPosition = worldPosition + tileOffset;\n"
    "    gl_Position = projection * offsetWorldPosition;\n"
    "    texCoord = vec2(aTexCoord.x * (256.0 / 4096.0), aTexCoord.y * (256.0 / 576.0));\n"
    "    testColor = vec4(0.01 * aTestIndex, 0.0, 0.0, 0.0);\n"
    "}\n";

static const char *frag_shader_src =
    "#version 450 core\n"
    "in vec2 texCoord;\n"
    "in vec4 testColor;\n"
    "out vec4 FragColor;\n"
    "\n"
    "uniform sampler2D backgroundTex;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    FragColor = mix(texture(backgroundTex, texCoord), testColor, 0.5);\n"
    "}\n";

bool gf_background_commit_state(struct gf_background *background) {
  if (background->background_state.dirty) {
    gf_obj_set_int(background->obj, GF_BACKGROUND_UNIFORM_TILE_SIZE,
                   background->background_state.tile_size);
    gf_obj_set_int(background->obj, GF_BACKGROUND_UNIFORM_TILE_WIDTH,
                   background->background_state.tiles_per_row);
    gf_obj_set_scale(background->obj,
                     (tf_scale){background->background_state.tile_size,
                                background->background_state.tile_size});
    float starting_position = background->background_state.tile_size / 2.0f;
    gf_obj_set_pos(background->obj,
                   (tf_pos){starting_position, starting_position});
    int test_buf[128];
    for (int i = 0; i < (sizeof(test_buf) / sizeof(int)); i++) {
      test_buf[i] = i;
    }
    assert(background->background_state.tiles_to_fill_screen < 128);
    gf_obj_update_buffer_data(
        background->instance_attr_buf_object, test_buf,
        background->background_state.tiles_to_fill_screen);
    gf_obj_set_binding_divisor(background->obj,
                               background->instance_attr_buf_binding_index, 6);

    background->background_state.dirty = false;
    return true;
  }
  gf_obj_commit_state(background->obj);
  return false;
}

static void gf_background_set_tile_size(struct gf_background *background,
                                        int tile_size) {
  background->background_state.tile_size = tile_size;
  background->background_state.dirty     = true;
}

struct gf_background *gf_background_create() {
  if (gf_background_list.count + 1 >= gf_background_list.capacity) {
    gf_log(DEBUG_LOG,
           "`gf_background_list` has a count of '%i', which is greater than "
           "it's capacity of '%i'.",
           gf_background_list.count, gf_background_list.capacity);
    return NULL;
  }
  struct gf_background *background =
      &gf_background_list.items[gf_background_list.count++];
  gf_background_set_tile_size(background, GF_BACKGROUND_DEFAULT_TILE_SIZE);

  background->obj = gf_obj_create_box();
  struct gf_shader *shader =
      gf_compile_shaders(vert_shader_src, frag_shader_src);
  gf_obj_set_shader(background->obj, shader);
  gf_obj_set_texture(background->obj, "backgroundTex", GF_TEXTURE_GRASS_1);
  // TODO: Create a means to increment binding index, or keep binding index in
  // macro instead of hardcoding.
  background->instance_attr_buf_binding_index = 1;
  background->instance_attr_buf_object        = gf_obj_create_attr(
      background->obj, background->instance_attr_buf_binding_index,
      GF_BACKGROUND_ATTRIB_TEST_INDEX_LOCATION, GL_INT);

  gf_background_commit_state(background);

  return background;
}

static void gf_background_sync_tiles(struct gf_background *background) {
  const int tile_size = background->background_state.tile_size;
  assert(tile_size != 0);
  const struct viewport_dimensions *viewport = gf_render_get_window_size();
  // Add 1 to height and width to offset int division rounding down.
  int width  = (viewport->width / tile_size) + 1;
  int height = (viewport->height / tile_size) + 1;

  int tiles_to_fill_screen = width * height;
  if (background->background_state.tiles_per_row != width ||
      background->background_state.tiles_to_fill_screen !=
          tiles_to_fill_screen) {
    background->background_state.tiles_per_row        = width;
    background->background_state.tiles_to_fill_screen = tiles_to_fill_screen;
    background->background_state.dirty                = true;
  }
}

void gf_background_draw(struct gf_background *background) {
  gf_background_sync_tiles(background);
  gf_background_commit_state(background);
  gf_obj_draw_instanced(background->obj,
                        background->background_state.tiles_to_fill_screen);
}
