#include "common.h"
#include "gf_math.h"
#include "log.h"
#include "render.h"
#include "texture.h"

#define GF_BACKGROUND_UNIFORM_TILE_SIZE  3
#define GF_BACKGROUND_UNIFORM_TILE_WIDTH 4

#define GF_BACKGROUND_DEFAULT_TILE_SIZE  128

struct gf_background {
  struct gf_obj *obj;
  struct gf_background_state {
    bool dirty;
    int tileSize;
  } background_state;
};

STATIC_LIST(gf_background_list, struct gf_background, 128)

static const char *vert_shader_src =
    "#version 450 core\n"
    "\n"
    "layout (location = " TO_STR(GF_ATTRIB_VERT_LOCATION) ") in vec2 aPos;\n"
    "layout (location = " TO_STR(GF_ATTRIB_TEX_COORD_LOCATION) ") in vec2 aTexCoord;\n"
    "layout (location = " TO_STR(GF_UNIFORM_TRANSFORM_MAT_LOCATION) ") uniform mat4 model;\n"
    "layout (location = " TO_STR(GF_UNIFORM_PROJECTION_MAT_LOCATION) ") uniform mat4 projection;\n"
    "layout (location = " TO_STR(GF_BACKGROUND_UNIFORM_TILE_SIZE) ") uniform int tileSize;\n"
    "layout (location = " TO_STR(GF_BACKGROUND_UNIFORM_TILE_WIDTH) ") uniform int maxTileWidth;\n"
    "\n"
    "out vec2 texCoord;\n"
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
    "    texCoord = vec2(aTexCoord.x * (64.0 / 4096.0), aTexCoord.y * (64.0 / 576.0));\n"
    "}\n";

static const char *frag_shader_src =
    "#version 450 core\n"
    "in vec2 texCoord;\n"
    "out vec4 FragColor;\n"
    "\n"
    "uniform sampler2D backgroundTex;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    FragColor = texture(backgroundTex, texCoord);\n"
    "}\n";

bool gf_background_commit_state(struct gf_background *background) {
  if (background->background_state.dirty) {
    gf_obj_set_int(background->obj, GF_BACKGROUND_UNIFORM_TILE_SIZE,
                   background->background_state.tileSize);
    gf_obj_set_scale(background->obj,
                     (tf_scale){background->background_state.tileSize,
                                background->background_state.tileSize});
    float starting_position = background->background_state.tileSize / 2.0f;
    gf_obj_set_pos(background->obj,
                   (tf_pos){starting_position, starting_position});
    background->background_state.dirty = false;
    return true;
  }
  gf_obj_commit_state(background->obj);
  return false;
}

static void gf_background_set_tile_size(struct gf_background *background,
                                        int tile_size) {
  background->background_state.tileSize = tile_size;
  background->background_state.dirty    = true;
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
  gf_background_commit_state(background);

  return background;
}

static int get_tiles_to_fill_screen(int tile_size) {
  assert(tile_size != 0);
  const struct viewport_dimensions *viewport = gf_render_get_window_size();
  // Add 1 to height and width to offset int division rounding down.
  int width  = (viewport->width / tile_size) + 1;
  int height = (viewport->height / tile_size) + 1;

  return width * height;
}

void gf_background_draw(struct gf_background *background) {
  gf_background_commit_state(background);
  int tiles_to_draw =
      get_tiles_to_fill_screen(background->background_state.tileSize);
  gf_obj_set_int(background->obj, GF_BACKGROUND_UNIFORM_TILE_WIDTH,
                 (gf_render_get_window_size()->width /
                  background->background_state.tileSize) +
                     1);
  gf_obj_draw_instanced(background->obj, tiles_to_draw);
}
