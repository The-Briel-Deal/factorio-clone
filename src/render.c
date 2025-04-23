#include <GL/gl.h>
#include <GL/glcorearb.h>
#include <GL/glext.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <wayland-util.h>

#include "common.h"
#include "gf_math.h"
#include "log.h"
#include "render.h"

#define OBJ_LIST_MAX            128
#define SHADER_PROGRAM_LIST_MAX 128

static const GLuint BOX_INDEX_ORDER[] = {0, 1, 3, 0, 2, 3};

struct viewport_dimensions {
  int height;
  int width;
};

// Shared Global State shared by shaders.
struct render_state {
  struct viewport_dimensions viewport;
};

struct render_state render_state = {
    .viewport =
        {
            .height = GF_DEFAULT_WINDOW_HEIGHT,
            .width  = GF_DEFAULT_WINDOW_WIDTH,
        },
};

struct gf_shader {
  GLuint vert;
  GLuint frag;
  GLuint program;
  struct shader_state {
    struct viewport_dimensions last_committed_viewport;
  } state;
};

STATIC_LIST(gf_shader_list, struct gf_shader, SHADER_PROGRAM_LIST_MAX)

struct gf_obj {
  GLuint vbo;
  GLuint vao;
  GLuint ebo;
  struct obj_state {
    struct transform {
      tf_scale scale;
      tf_pos pos;
      radians rotation;
      bool dirty;
    } transform;
  } state;
  struct gf_shader *shader;
};

STATIC_LIST(gf_obj_list, struct gf_obj, OBJ_LIST_MAX)

bool gf_draw_update_window_size(int32_t width, int32_t height) {
  if (height == render_state.viewport.height &&
      width == render_state.viewport.width) {
    return false;
  }
  render_state.viewport =
      (struct viewport_dimensions){.width = width, .height = height};
  return true;
}

void gf_shader_sync_projection_matrix(struct gf_shader *shader) {
  int h = shader->state.last_committed_viewport.height;
  int w = shader->state.last_committed_viewport.width;

  gf_log(INFO_LOG,
         "Syncing projection matrix (h: %i, w: %i) to shader program '%i'.", h,
         w, shader->program);

  assert(h > 0);
  assert(w > 0);
  mat4 projection;
  gf_ortho(0.0f, w, 0, h, -1.0, 1.0, projection);

  glProgramUniformMatrix4fv(shader->program, GF_UNIFORM_PROJECTION_MAT_LOCATION,
                            1, false, (GLfloat *)projection);
}

void gf_shader_commit_state(struct gf_shader *shader) {
  // Only sync if shader viewport out of sync with render state.
  struct viewport_dimensions *last_vp = &shader->state.last_committed_viewport,
                             *curr_vp = &render_state.viewport;
  if (last_vp->height != curr_vp->height || last_vp->width != curr_vp->width) {
    last_vp->height = curr_vp->height;
    last_vp->width  = curr_vp->width;
    gf_shader_sync_projection_matrix(shader);
  }
}

struct gf_shader *gf_compile_shaders(const char *vert_shader_src,
                                     const char *frag_shader_src) {
  if (gf_shader_list.count + 1 >= gf_shader_list.capacity) {
    gf_log(DEBUG_LOG,
           "`gf_shader_list` has a count of '%i', which is greater than "
           "it's capacity of '%i'.",
           gf_shader_list.count, gf_shader_list.capacity);
    return NULL;
  }

  struct gf_shader *shader = &gf_shader_list.items[gf_shader_list.count++];

  shader->vert = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(shader->vert, 1, &vert_shader_src, NULL);
  glCompileShader(shader->vert);

  shader->frag = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(shader->frag, 1, &frag_shader_src, NULL);
  glCompileShader(shader->frag);

  shader->program = glCreateProgram();
  glAttachShader(shader->program, shader->vert);
  glAttachShader(shader->program, shader->frag);
  glLinkProgram(shader->program);

  shader->state = (struct shader_state){
      .last_committed_viewport =
          {
              .height = 0,
              .width  = 0,
          },
  };
  return shader;
}

const struct box_verts square_verts = {
    {
        {.x = 50, .y = 50},
        {1.0f, 1.0f},
    },
    {
        {.x = -50, .y = 50},
        {0.0f, 1.0f},
    },
    {
        {.x = 50, .y = -50},
        {1.0f, 0.0f},
    },
    {
        {.x = -50, .y = -50},
        {0.0f, 0.0f},
    },
};

struct gf_obj *gf_obj_create_box() {
  if (gf_obj_list.count + 1 >= gf_obj_list.capacity) {
    gf_log(DEBUG_LOG,
           "`gf_obj_list` has a count of '%i', which is greater than "
           "it's capacity of '%i'.",
           gf_obj_list.count, gf_obj_list.capacity);
    return NULL;
  }

  struct gf_obj *obj = &gf_obj_list.items[gf_obj_list.count++];

  glCreateBuffers(1, &obj->vbo);
  glNamedBufferStorage(obj->vbo, sizeof(struct box_verts), &square_verts,
                       GL_DYNAMIC_STORAGE_BIT);

  glCreateBuffers(1, &obj->ebo);
  glNamedBufferStorage(obj->ebo, sizeof(BOX_INDEX_ORDER), BOX_INDEX_ORDER,
                       GL_DYNAMIC_STORAGE_BIT);

  glCreateVertexArrays(1, &obj->vao);
  glVertexArrayVertexBuffer(obj->vao, 0, obj->vbo, 0, sizeof(vertex));
  glVertexArrayElementBuffer(obj->vao, obj->ebo);
  glEnableVertexArrayAttrib(obj->vao, 0);
  glVertexArrayAttribFormat(obj->vao, 0, 2, GL_FLOAT, false, 0);
  glVertexArrayAttribBinding(obj->vao, 0, 0);

  obj->state = (struct obj_state){
      .transform =
          {
              .scale.x = 1.0f,
              .scale.y = 1.0f,

              .pos.x = 100.0f,
              .pos.y = 100.0f,

              .dirty = true,
          },
  };
  return obj;
}

bool gf_obj_set_shader(struct gf_obj *obj, struct gf_shader *shader) {
  if (obj->shader == shader) {
    return false;
  }
  obj->shader = shader;
  return true;
}

static void gf_obj_sync_transform(struct gf_obj *obj) {
  gf_log(INFO_LOG,
         "Syncing transformations (scale: { x: '%f', y: '%f'}, pos: { x: '%f', "
         "y: '%f' }) to shader "
         "program '%i'.",
         obj->state.transform.scale.x, obj->state.transform.scale.y,
         obj->state.transform.pos.x, obj->state.transform.pos.y,
         obj->shader->program);

  mat4 model;
  gf_mat4_identity(model);
  gf_mat4_translate2d(model, obj->state.transform.pos);
  gf_mat4_rotate2d(model, obj->state.transform.rotation);
  gf_mat4_scale2d(model, obj->state.transform.scale);

  glProgramUniformMatrix4fv(obj->shader->program,
                            GF_UNIFORM_TRANSFORM_MAT_LOCATION, 1, false,
                            (GLfloat *)model);
}

tf_scale gf_obj_get_scale(struct gf_obj *obj) {
  return obj->state.transform.scale;
}

void gf_obj_set_scale(struct gf_obj *obj, tf_scale scale) {
  obj->state.transform.scale = scale;
  obj->state.transform.dirty = true;
}

void gf_obj_set_pos(struct gf_obj *obj, tf_pos pos) {
  obj->state.transform.pos   = pos;
  obj->state.transform.dirty = true;
}

tf_pos gf_obj_get_pos(struct gf_obj *obj) {
  return obj->state.transform.pos;
}

void gf_obj_set_rotation(struct gf_obj *obj, radians rotation) {
  obj->state.transform.rotation = rotation;
  obj->state.transform.dirty    = true;
}

void gf_obj_rotate_by(struct gf_obj *obj, radians rotation) {
  obj->state.transform.rotation += rotation;
  obj->state.transform.dirty = true;
}

radians gf_obj_get_rotation(struct gf_obj *obj) {
  return obj->state.transform.rotation;
}

void gf_obj_commit_state(struct gf_obj *obj) {
  if (obj->state.transform.dirty == true) {
    obj->state.transform.dirty = false;
    gf_obj_sync_transform(obj);
  }
  gf_shader_commit_state(obj->shader);
}

bool gf_obj_draw(struct gf_obj *obj) {
  glUseProgram(obj->shader->program);
  glBindVertexArray(obj->vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  return true;
}
