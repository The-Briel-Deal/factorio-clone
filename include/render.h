#ifndef GF_DRAW_H
#define GF_DRAW_H

#include <GL/gl.h>
#include <stdbool.h>
#include <stdint.h>

#include "gf_math.h"
#include "texture.h"

struct triangle_verts {
  vertex v1;
  vertex v2;
  vertex v3;
};

struct tex_vert {
  vertex pos;
  vec2 tex_coord;
};

struct box_verts {
  struct tex_vert top_right;
  struct tex_vert top_left;
  struct tex_vert bottom_right;
  struct tex_vert bottom_left;
};

struct gf_obj;
struct gf_shader;


bool gf_draw_update_window_size(int32_t height, int32_t width);
void gf_shader_commit_state(struct gf_shader *shader);

struct gf_obj *gf_obj_create_box();
bool gf_obj_set_shader(struct gf_obj *obj, struct gf_shader *shader);
bool gf_obj_set_texture(struct gf_obj *obj, const char *name,
                        enum gf_texture_type type);

void gf_obj_set_uniform_int(struct gf_obj *obj, GLint location, GLint value);

tf_scale gf_obj_get_scale(struct gf_obj *obj);
void gf_obj_set_scale(struct gf_obj *obj, tf_scale scale);

tf_pos gf_obj_get_pos(struct gf_obj *obj);
void gf_obj_set_pos(struct gf_obj *obj, tf_pos pos);

radians gf_obj_get_rotation(struct gf_obj *obj);
void gf_obj_set_rotation(struct gf_obj *obj, radians rotation);
void gf_obj_rotate_by(struct gf_obj *obj, radians rotation);

void gf_obj_set_int(struct gf_obj *obj, int location, int val);

void gf_obj_commit_state(struct gf_obj *obj);
bool gf_obj_draw(struct gf_obj *obj);
bool gf_obj_draw_instanced(struct gf_obj *obj, int instance_count);

struct gf_shader *gf_compile_shaders(const char *vert_shader_src,
                                     const char *frag_shader_src);

#endif
