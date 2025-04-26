
#include "common.h"
#include "gf_math.h"
#include "log.h"
#include "render.h"
#include "texture.h"
struct gf_background {
  struct gf_obj *obj;
};

STATIC_LIST(gf_background_list, struct gf_background, 128)


static const char *vert_shader_src =
    "#version 450 core\n"
    "layout (location = " TO_STR(GF_ATTRIB_VERT_LOCATION) ") in vec2 aPos;\n"
    "layout (location = " TO_STR(GF_ATTRIB_TEX_COORD_LOCATION) ") in vec2 aTexCoord;\n"
		"layout (location = " TO_STR(GF_UNIFORM_TRANSFORM_MAT_LOCATION) ") uniform mat4 model;"
		"layout (location = " TO_STR(GF_UNIFORM_PROJECTION_MAT_LOCATION) ") uniform mat4 projection;"
		"\n"
		"out vec2 texCoord;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);\n"
		"    texCoord = aTexCoord / 10.0;\n"
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

  background->obj = gf_obj_create_box();
  struct gf_shader *shader =
      gf_compile_shaders(vert_shader_src, frag_shader_src);
  gf_obj_set_shader(background->obj, shader);

  gf_obj_set_texture(background->obj, "backgroundTex", GF_TEXTURE_GRASS_1);
  gf_obj_set_pos(background->obj, (tf_pos){500, 500});
  gf_obj_commit_state(background->obj);

  return background;
}

void gf_background_draw(struct gf_background *background) {
  gf_obj_commit_state(background->obj);
  gf_obj_draw(background->obj);
}
