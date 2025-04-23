#include <GL/gl.h>
#include <GL/glext.h>
#include <sys/types.h>
#include <xkbcommon/xkbcommon.h>

#include "stb_image.h"

#include "common.h"
#include "gf_math.h"
#include "log.h"
#include "player.h"
#include "render.h"

#define PLAYER_LIST_MAX     128

#define PLAYER_SPEED        300.0f
#define PLAYER_LERP         5.0f

#define PLAYER_TEXTURE_PATH "static/factorio-icon.png"

enum gf_player_input_state {
  GF_PLAYER_INPUT_UP    = 0b0001,
  GF_PLAYER_INPUT_DOWN  = 0b0010,
  GF_PLAYER_INPUT_RIGHT = 0b0100,
  GF_PLAYER_INPUT_LEFT  = 0b1000,
};

struct gf_player {
  struct gf_obj *obj;
  vec2s movement;
  enum gf_player_input_state input_state;
};

STATIC_LIST(gf_player_list, struct gf_player, 128)

const char *vert_shader_src =
    "#version 450 core\n"
    "layout (location = " TO_STR(GF_ATTRIB_VERT_LOCATION) ") in vec2 aPos;\n"
		"layout (location = " TO_STR(GF_UNIFORM_TRANSFORM_MAT_LOCATION) ") uniform mat4 model;"
		"layout (location = " TO_STR(GF_UNIFORM_PROJECTION_MAT_LOCATION) ") uniform mat4 projection;"
		"\n"
    "out vec4 vertexColor;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);\n"
    "    vertexColor = vec4(0.5, 0.0, 0.0, 1.0);\n"
    "}\n";

const char *frag_shader_src =
    "#version 450 core\n"
    "in vec4 vertexColor;\n"
    "out vec4 FragColor;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    FragColor = vertexColor;\n"
    "}\n";

#ifdef GF_DEBUG_PLAYER_INPUT

static void gf_debug_log_player_input(enum gf_player_input_state input_state) {
  char log_str[128] = "Player input state = [ ";
  if (input_state & GF_PLAYER_INPUT_UP) {
    strcat(log_str, "Up, ");
  }
  if (input_state & GF_PLAYER_INPUT_DOWN) {
    strcat(log_str, "Down, ");
  }
  if (input_state & GF_PLAYER_INPUT_RIGHT) {
    strcat(log_str, "Right, ");
  }
  if (input_state & GF_PLAYER_INPUT_LEFT) {
    strcat(log_str, "Left, ");
  }

  strcat(log_str, "]");

  gf_log(DEBUG_LOG, log_str);
}

#endif

static GLuint gf_player_load_texture() {
  GLuint texture;
  int width, height, nrChannels;
  u_int8_t *img_data =
      stbi_load(PLAYER_TEXTURE_PATH, &width, &height, &nrChannels, 0);
  assert(img_data != NULL);
  assert(nrChannels == 4);
  glCreateTextures(GL_TEXTURE_2D, 1, &texture);
  glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTextureStorage2D(texture, 1, GL_RGBA8, width, height);

  return texture;
}

struct gf_player *gf_player_create() {
  if (gf_player_list.count + 1 >= gf_player_list.capacity) {
    gf_log(DEBUG_LOG,
           "`gf_player_list` has a count of '%i', which is greater than "
           "it's capacity of '%i'.",
           gf_player_list.count, gf_player_list.capacity);
    return NULL;
  }
  gf_player_load_texture();

  struct gf_player *player = &gf_player_list.items[gf_player_list.count++];
  player->input_state      = 0b0000;
  player->movement         = (vec2s){.x = 0.0, .y = 0.0};

  player->obj = gf_obj_create_box();
  struct gf_shader *shader =
      gf_compile_shaders(vert_shader_src, frag_shader_src);
  gf_obj_set_shader(player->obj, shader);
  gf_obj_commit_state(player->obj);

  return player;
}

void gf_player_input_listener(xkb_keysym_t key, bool pressed, void *data) {
  struct gf_player *player = data;
  if (pressed) {
    switch (key) {
      case XKB_KEY_w: player->input_state |= GF_PLAYER_INPUT_UP; break;
      case XKB_KEY_a: player->input_state |= GF_PLAYER_INPUT_LEFT; break;
      case XKB_KEY_s: player->input_state |= GF_PLAYER_INPUT_DOWN; break;
      case XKB_KEY_d: player->input_state |= GF_PLAYER_INPUT_RIGHT; break;
    }
  } else {
    switch (key) {
      case XKB_KEY_w: player->input_state &= ~GF_PLAYER_INPUT_UP; break;
      case XKB_KEY_a: player->input_state &= ~GF_PLAYER_INPUT_LEFT; break;
      case XKB_KEY_s: player->input_state &= ~GF_PLAYER_INPUT_DOWN; break;
      case XKB_KEY_d: player->input_state &= ~GF_PLAYER_INPUT_RIGHT; break;
    }
  }
#ifdef GF_DEBUG_PLAYER_INPUT
  gf_log(DEBUG_LOG, "Key %s: '%i'", pressed ? "Pressed" : "Released", key);
#endif
}

void gf_player_update_state(struct gf_player *player, double delta_time) {
  gf_log(DEBUG_LOG, "Delta Time: '%f'", delta_time);
  vec2s movement_vector = {.x = 0.0, .y = 0.0};
  if (player->input_state & GF_PLAYER_INPUT_UP)
    movement_vector.y += 1.0;
  if (player->input_state & GF_PLAYER_INPUT_RIGHT)
    movement_vector.x += 1.0;
  if (player->input_state & GF_PLAYER_INPUT_DOWN)
    movement_vector.y -= 1.0;
  if (player->input_state & GF_PLAYER_INPUT_LEFT)
    movement_vector.x -= 1.0;

  gf_vec2s_normalize(&movement_vector);
  gf_vec2s_lerp(&player->movement, &movement_vector, PLAYER_LERP * delta_time,
                &player->movement);

  vec2s move_by = player->movement;
  gf_vec2s_scale(&move_by, PLAYER_SPEED * delta_time);
  gf_log(INFO_LOG, "Velocity: {x = %f, y = %f}", move_by.x, move_by.y);

  vec2s pos = gf_obj_get_pos(player->obj);

  pos.x += move_by.x;
  pos.y += move_by.y;

  gf_obj_set_pos(player->obj, pos);
}

void gf_player_draw(struct gf_player *player) {
#ifdef GF_DEBUG_PLAYER_INPUT
  gf_debug_log_player_input(player->input_state);
#endif
  gf_obj_commit_state(player->obj);
  gf_obj_draw(player->obj);
}
