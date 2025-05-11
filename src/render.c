#include <GL/gl.h>
#include <GL/glcorearb.h>
#include <GL/glext.h>
#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <wayland-util.h>

#include "background.h"
#include "common.h"
#include "gf_math.h"
#include "log.h"
#include "render.h"
#include "texture.h"

#define OBJ_LIST_MAX            128
#define SHADER_PROGRAM_LIST_MAX 128

static const GLuint BOX_INDEX_ORDER[] = {0, 1, 3, 0, 2, 3};

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
  GLuint ubo_mat;
  GLsizei ubo_count;
  GLuint ubos[16];
  struct obj_state {
    struct transform {
      tf_scale scale;
      tf_pos pos;
      radians rotation;
      bool dirty;
    } transform;
  } state;
  struct gf_shader *shader;
  const struct gf_texture *texture;
  int texture_location;
  vec2s last_commited_cam_pos;
};

STATIC_LIST(gf_obj_list, struct gf_obj, OBJ_LIST_MAX)

const static struct gf_obj *camera_focused_obj = NULL;

void gf_camera_set_focused_obj(const struct gf_obj *obj) {
  camera_focused_obj = obj;
}

struct gf_quad_bounds gf_camera_get_bounds() {

  int h = render_state.viewport.height, w = render_state.viewport.width;

  assert(h > 0);
  assert(w > 0);

  vec2s focused_pos = {0.0, 0.0};
  if (camera_focused_obj != NULL)
    focused_pos = camera_focused_obj->state.transform.pos;

  struct gf_quad_bounds bounds = {.left   = (focused_pos.x - (0.5 * w)),
                                  .right  = (focused_pos.x + (0.5 * w)),
                                  .bottom = (focused_pos.y - (0.5 * h)),
                                  .top    = (focused_pos.y + (0.5 * h))};

  gf_log(INFO_LOG,
         "Viewport Bounds: (top = %f, bottom = %f, left = %f, right = %f)",
         bounds.top, bounds.bottom, bounds.left, bounds.right);

  return bounds;
}

// TODO: Use a bool to know if viewport in shader is dirty.
bool gf_render_update_window_size(int32_t width, int32_t height) {
  if (height == render_state.viewport.height &&
      width == render_state.viewport.width) {
    return false;
  }
  render_state.viewport =
      (struct viewport_dimensions){.width = width, .height = height};
  return true;
}

const struct viewport_dimensions *gf_render_get_window_size() {
  return &render_state.viewport;
}

void gf_obj_sync_projection_matrix(struct gf_obj *obj) {
  struct gf_quad_bounds bounds = gf_camera_get_bounds();

  mat4 projection;
  gf_ortho(bounds.left, bounds.right, bounds.bottom, bounds.top, -1.0, 1.0,
           projection);

  glNamedBufferSubData(obj->ubo_mat, 0, sizeof(projection), projection);
}

static bool gf_obj_camera_has_changed(const struct gf_obj *obj) {
  if (camera_focused_obj == NULL)
    return false;

  vec2s cam_pos = camera_focused_obj->state.transform.pos;
  if (cam_pos.x != obj->last_commited_cam_pos.x)
    return true;
  if (cam_pos.y != obj->last_commited_cam_pos.y)
    return true;
  return false;
}

void gf_obj_commit_projection(struct gf_obj *obj) {
  // Only sync if shader viewport out of sync with render state.
  struct viewport_dimensions *last_vp =
                                 &obj->shader->state.last_committed_viewport,
                             *curr_vp = &render_state.viewport;
  if (last_vp->height != curr_vp->height || last_vp->width != curr_vp->width ||
      gf_obj_camera_has_changed(obj)) {
    last_vp->height = curr_vp->height;
    last_vp->width  = curr_vp->width;
    gf_obj_sync_projection_matrix(obj);
  }
  if (camera_focused_obj != NULL) {
    obj->last_commited_cam_pos = camera_focused_obj->state.transform.pos;
  }
}

static struct gf_shader *gf_shader_alloc() {
  if (gf_shader_list.count + 1 >= gf_shader_list.capacity) {
    gf_log(DEBUG_LOG,
           "`gf_shader_list` has a count of '%i', which is greater than "
           "it's capacity of '%i'.",
           gf_shader_list.count, gf_shader_list.capacity);
    return NULL;
  }

  return &gf_shader_list.items[gf_shader_list.count++];
}

static void gf_shader_compile_check_err(GLuint shader) {
  GLint compiled;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (compiled == GL_FALSE) {
    char buf[1024];
    int buf_size = sizeof(buf);
    int len;
    glGetShaderInfoLog(shader, buf_size, &len, buf);
    gf_log(ERROR_LOG, "SHADER COMPILE FAIL - %.*s", len, buf);
  }
}

static int gf_shader_compile_from_path(GLenum type, const char *path) {
  int fshader = open(path, O_RDONLY);
  assert(fshader != -1);
  struct stat stat;
  int e = fstat(fshader, &stat);
  assert(e != -1);
  const char *shader_str =
      mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, fshader, 0);
  assert(shader_str != NULL);

  GLuint gl_shader = glCreateShader(type);
  // Needs to be cast to a int since I pass len by ptr.
  int shader_len = stat.st_size;
  glShaderSource(gl_shader, 1, &shader_str, &shader_len);
  glCompileShader(gl_shader);
#ifndef NDEBUG
  gf_shader_compile_check_err(gl_shader);
#endif
  munmap((void *)shader_str, stat.st_size);
  return gl_shader;
}

static void gf_shader_program_check_err(GLuint program) {
  GLint linked;
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  if (linked == GL_FALSE) {
    char buf[1024];
    int buf_size = sizeof(buf);
    int len;
    glGetProgramInfoLog(program, buf_size, &len, buf);
    gf_log(ERROR_LOG, "SHADER PROGRAM LINK FAIL - %.*s", len, buf);
  }
}


static struct gf_shader *gf_shader_program_create(GLuint vs, GLuint fs) {
  struct gf_shader *shader = gf_shader_alloc();

  shader->vert    = vs;
  shader->frag    = fs;
  shader->state   = (struct shader_state){0};
  shader->program = glCreateProgram();
  glAttachShader(shader->program, shader->vert);
  glAttachShader(shader->program, shader->frag);
  glLinkProgram(shader->program);
#ifndef NDEBUG
  gf_shader_program_check_err(shader->program);
#endif
  return shader;
}

struct gf_shader *gf_shader_create_from_paths(const char *vert_shader_path,
                                              const char *frag_shader_path) {
  int vs = gf_shader_compile_from_path(GL_VERTEX_SHADER, vert_shader_path);
  int fs = gf_shader_compile_from_path(GL_FRAGMENT_SHADER, frag_shader_path);

  return gf_shader_program_create(vs, fs);
}

// TODO: Cache uniform locations.
static int gf_shader_get_uni_location(struct gf_shader *shader,
                                      const char *name) {
  GLint loc = glGetUniformLocation(shader->program, name);
  assert(loc != -1);
  return loc;
}

// TODO: Cache attr locations.
static int gf_shader_get_attr_location(struct gf_shader *shader,
                                       const char *name) {
  GLint loc = glGetAttribLocation(shader->program, name);
  assert(loc != -1);
  return loc;
}

const struct box_verts square_verts = {
    {
        {.x = 0.5, .y = 0.5},
        {1.0f, 1.0f},
    },
    {
        {.x = -0.5, .y = 0.5},
        {0.0f, 1.0f},
    },
    {
        {.x = 0.5, .y = -0.5},
        {1.0f, 0.0f},
    },
    {
        {.x = -0.5, .y = -0.5},
        {0.0f, 0.0f},
    },
};

void gf_obj_setup_attrs(struct gf_obj *obj) {
  assert(obj->shader != NULL);
  GLint pos_location = gf_shader_get_attr_location(obj->shader, "aPos");
  glEnableVertexArrayAttrib(obj->vao, pos_location);
  glVertexArrayAttribFormat(obj->vao, pos_location,
                            sizeof(vertex) / sizeof(GLfloat), GL_FLOAT, false,
                            offsetof(struct tex_vert, pos));
  glVertexArrayAttribBinding(obj->vao, pos_location, 0);


  GLint tex_coord_location =
      gf_shader_get_attr_location(obj->shader, "aTexCoord");
  glEnableVertexArrayAttrib(obj->vao, tex_coord_location);
  glVertexArrayAttribFormat(obj->vao, tex_coord_location,
                            sizeof(vec2) / sizeof(GLfloat), GL_FLOAT, false,
                            offsetof(struct tex_vert, tex_coord));
  glVertexArrayAttribBinding(obj->vao, tex_coord_location, 0);
}


GLuint gf_obj_create_ubo(struct gf_obj *obj, uint size, char *name) {
  assert(obj->ubo_count < sizeof(obj->ubos));

  // Binding point is going to be the next available index in our ubo list.
  GLsizei binding_point = obj->ubo_count++;
  GLuint *ubo           = &obj->ubos[binding_point];

  // Create buf.
  glCreateBuffers(1, ubo);
  glNamedBufferData(*ubo, size, NULL, GL_DYNAMIC_DRAW);

  // Bind to block index.
  GLuint block_index = glGetUniformBlockIndex(obj->shader->program, name);
  glUniformBlockBinding(obj->shader->program, block_index, binding_point);

  return *ubo;
}

void gf_obj_bind_ubos(struct gf_obj *obj) {
  GLsizei count = obj->ubo_count;

  glBindBuffersBase(GL_UNIFORM_BUFFER, 0, count, obj->ubos);
}

struct gf_obj *gf_obj_create_quad(struct gf_shader *shader) {
  if (gf_obj_list.count + 1 >= gf_obj_list.capacity) {
    gf_log(DEBUG_LOG,
           "`gf_obj_list` has a count of '%i', which is greater than "
           "it's capacity of '%i'.",
           gf_obj_list.count, gf_obj_list.capacity);
    return NULL;
  }

  struct gf_obj *obj         = &gf_obj_list.items[gf_obj_list.count++];
  obj->shader                = shader;
  obj->last_commited_cam_pos = (vec2s){0.0f, 0.0f};

  glCreateBuffers(1, &obj->vbo);
  glNamedBufferStorage(obj->vbo, sizeof(struct box_verts), &square_verts,
                       GL_DYNAMIC_STORAGE_BIT);

  glCreateBuffers(1, &obj->ebo);
  glNamedBufferStorage(obj->ebo, sizeof(BOX_INDEX_ORDER), BOX_INDEX_ORDER,
                       GL_DYNAMIC_STORAGE_BIT);

  glCreateVertexArrays(1, &obj->vao);
  glVertexArrayVertexBuffer(obj->vao, 0, obj->vbo, 0, sizeof(struct tex_vert));
  glVertexArrayElementBuffer(obj->vao, obj->ebo);

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
  gf_obj_setup_attrs(obj);
  obj->ubo_count = 0;
  obj->ubo_mat   = gf_obj_create_ubo(obj, sizeof(mat4) * 2, "Matrices");
  return obj;
}

GLuint gf_obj_create_attr(const struct gf_obj *obj, GLuint binding_index,
                          const char *name, GLenum type) {
  GLuint location = gf_shader_get_attr_location(obj->shader, name);

  GLuint instanced_attr_buffer;
  glCreateBuffers(1, &instanced_attr_buffer);
  // TODO: Use an argument for initial size.
  glNamedBufferData(instanced_attr_buffer, 1024 * sizeof(uint8_t), NULL,
                    GL_DYNAMIC_DRAW);
  // TODO: Use an argument for stride.
  glVertexArrayVertexBuffer(obj->vao, 1, instanced_attr_buffer, 0, 1);
  glEnableVertexArrayAttrib(obj->vao, location);
  // TODO: Use an argument for the size of a single attrib.
  glVertexArrayAttribIFormat(obj->vao, location, 1, type, 0);
  glVertexArrayAttribBinding(obj->vao, location, 1);

  return instanced_attr_buffer;
}

void gf_obj_update_buffer_data(GLuint buffer, const void *data, int size) {
  if (size != 0) {
    glNamedBufferSubData(buffer, 0, size, data);
  }
}

void gf_obj_set_binding_divisor(const struct gf_obj *obj, GLuint binding_index,
                                GLuint divisor) {
  glVertexArrayBindingDivisor(obj->vao, binding_index, divisor);
}

bool gf_obj_set_shader(struct gf_obj *obj, struct gf_shader *shader) {
  if (obj->shader == shader) {
    return false;
  }
  obj->shader = shader;
  return true;
}

bool gf_obj_set_texture(struct gf_obj *obj, const char *name,
                        enum gf_texture_type type) {
  obj->texture          = gf_texture_get(type);
  obj->texture_location = glGetUniformLocation(obj->shader->program, name);
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

  glNamedBufferSubData(obj->ubo_mat, sizeof(model), sizeof(model), model);
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
  gf_obj_commit_projection(obj);
}

void gf_obj_set_int(struct gf_obj *obj, char *name, int val) {
  int location = glGetUniformLocation(obj->shader->program, name);
  glProgramUniform1i(obj->shader->program, location, val);
}
void gf_obj_set_vec4v(struct gf_obj *obj, char *name, int count, void *val) {
  int location = gf_shader_get_uni_location(obj->shader, name);
  glProgramUniform4fv(obj->shader->program, location, count, val);
}

bool gf_obj_draw(struct gf_obj *obj) {
  glUseProgram(obj->shader->program);
  glProgramUniform1i(obj->shader->program, obj->texture_location, 0);
  glBindTextureUnit(0, obj->texture->gl_name);
  glBindVertexArray(obj->vao);
  gf_obj_bind_ubos(obj);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  return true;
}

bool gf_obj_draw_instanced(struct gf_obj *obj, int instance_count) {
  glUseProgram(obj->shader->program);
  glProgramUniform1i(obj->shader->program, obj->texture_location, 0);
  glBindTextureUnit(0, obj->texture->gl_name);
  glBindVertexArray(obj->vao);
  gf_obj_bind_ubos(obj);
  glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, instance_count);
  return true;
}
