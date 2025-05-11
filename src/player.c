#include <GL/gl.h>
#include <GL/glext.h>
#include <xkbcommon/xkbcommon.h>

#include "stb_image.h"

#include "common.h"
#include "gf_math.h"
#include "log.h"
#include "player.h"
#include "render.h"
#include "texture.h"

#define PLAYER_SPRITE_SHEET_CELL_HEIGHT       116
#define PLAYER_SPRITE_SHEET_CELL_WIDTH        92
#define PLAYER_SPRITE_SHEET_CELL_COUNT_HEIGHT 8
#define PLAYER_SPRITE_SHEET_CELL_COUNT_WIDTH  22

#define PLAYER_LIST_MAX                       128

#define PLAYER_SPEED                          300.0f
#define PLAYER_LERP                           20.0f

#define PLAYER_TIME_BETWEEN_SPRITE_FRAMES     (1.0f / 10.0f)

enum gf_player_input_state {
  GF_PLAYER_INPUT_UP    = 0b0001,
  GF_PLAYER_INPUT_DOWN  = 0b0010,
  GF_PLAYER_INPUT_RIGHT = 0b0100,
  GF_PLAYER_INPUT_LEFT  = 0b1000,
};

enum gf_player_dir_facing {
  GF_PLAYER_FACING_LEFT_UP    = 0,
  GF_PLAYER_FACING_LEFT       = 1,
  GF_PLAYER_FACING_DOWN_LEFT  = 2,
  GF_PLAYER_FACING_DOWN       = 3,
  GF_PLAYER_FACING_RIGHT_DOWN = 4,
  GF_PLAYER_FACING_RIGHT      = 5,
  GF_PLAYER_FACING_UP_RIGHT   = 6,
  GF_PLAYER_FACING_UP         = 7,
};

struct gf_player {
  struct gf_obj *obj;
  vec2s movement;
  enum gf_player_input_state input_state;
  enum gf_player_dir_facing dir_facing; // Corresponds to sprite sheet y pos.
  uint sprite_col_index;                // Corresponds to sprite sheet x pos.
  float time_till_next_sprite;
};

STATIC_LIST(gf_player_list, struct gf_player, 128)


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


struct gf_player *gf_player_create() {
  if (gf_player_list.count + 1 >= gf_player_list.capacity) {
    gf_log(DEBUG_LOG,
           "`gf_player_list` has a count of '%i', which is greater than "
           "it's capacity of '%i'.",
           gf_player_list.count, gf_player_list.capacity);
    return NULL;
  }

  struct gf_player *player = &gf_player_list.items[gf_player_list.count++];

  player->input_state           = 0b0000;
  player->movement              = (vec2s){.x = 0.0, .y = 0.0};
  player->dir_facing            = GF_PLAYER_FACING_DOWN;
  player->sprite_col_index      = 0;
  player->time_till_next_sprite = PLAYER_TIME_BETWEEN_SPRITE_FRAMES;

  struct gf_shader *shader = gf_shader_create_from_paths(
      "shader/player_vert.glsl", "shader/player_frag.glsl");
  player->obj = gf_obj_create_quad(shader);
  gf_obj_set_scale(player->obj, (vec2s){100, 100});
  gf_obj_set_texture(player->obj, "playerTex", GF_TEXTURE_CHARACTER_IDLE_1);
  gf_obj_set_texture(player->obj, "playerTexMask",
                     GF_TEXTURE_CHARACTER_IDLE_MASK_1);
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
  gf_log(INFO_LOG, "Delta Time: '%f'", delta_time);
  vec2s movement_vector = {.x = 0.0, .y = 0.0};
  if (player->input_state & GF_PLAYER_INPUT_UP) {
    movement_vector.y += 1.0;
  }
  if (player->input_state & GF_PLAYER_INPUT_RIGHT) {
    movement_vector.x += 1.0;
  }
  if (player->input_state & GF_PLAYER_INPUT_DOWN) {
    movement_vector.y -= 1.0;
  }
  if (player->input_state & GF_PLAYER_INPUT_LEFT) {
    movement_vector.x -= 1.0;
  }
  // There is probably a better way to do this.
  if (movement_vector.y < 0.0f) {
    if (movement_vector.x > 0.0f)
      player->dir_facing = GF_PLAYER_FACING_RIGHT_DOWN;
    else if (movement_vector.x == 0.0f)
      player->dir_facing = GF_PLAYER_FACING_DOWN;
  }
  if (movement_vector.x < 0.0f) {
    if (movement_vector.y < 0.0f)
      player->dir_facing = GF_PLAYER_FACING_DOWN_LEFT;
    else if (movement_vector.y == 0.0f)
      player->dir_facing = GF_PLAYER_FACING_LEFT;
  }
  if (movement_vector.y > 0.0f) {
    if (movement_vector.x < 0.0f)
      player->dir_facing = GF_PLAYER_FACING_LEFT_UP;
    else if (movement_vector.x == 0.0f)
      player->dir_facing = GF_PLAYER_FACING_UP;
  }
  if (movement_vector.x > 0.0f) {
    if (movement_vector.y > 0.0f)
      player->dir_facing = GF_PLAYER_FACING_UP_RIGHT;
    else if (movement_vector.y == 0.0f)
      player->dir_facing = GF_PLAYER_FACING_RIGHT;
  }

  gf_vec2s_normalize(&movement_vector);
  gf_vec2s_lerp(&player->movement, &movement_vector, PLAYER_LERP * delta_time,
                &player->movement);

  vec2s move_by = player->movement;
  gf_vec2s_scale(&move_by, PLAYER_SPEED * delta_time);
  gf_log(INFO_LOG, "Velocity: {x = %f, y = %f}", move_by.x, move_by.y);

  player->time_till_next_sprite -= delta_time;

  if (player->time_till_next_sprite <= 0.0f) {
    player->time_till_next_sprite += PLAYER_TIME_BETWEEN_SPRITE_FRAMES;
    player->sprite_col_index =
        (player->sprite_col_index + 1) % PLAYER_SPRITE_SHEET_CELL_COUNT_WIDTH;
  }

  vec2s pos = gf_obj_get_pos(player->obj);

  pos.x += move_by.x;
  pos.y += move_by.y;

  gf_obj_set_pos(player->obj, pos);
}

void gf_player_focus_camera(const struct gf_player *player) {
  gf_camera_set_focused_obj(player->obj);
}

void gf_player_draw(struct gf_player *player) {
#ifdef GF_DEBUG_PLAYER_INPUT
  gf_debug_log_player_input(player->input_state);
#endif
  int sprite_index =
      (player->dir_facing * PLAYER_SPRITE_SHEET_CELL_COUNT_WIDTH) +
      player->sprite_col_index;
  assert(sprite_index < (PLAYER_SPRITE_SHEET_CELL_COUNT_WIDTH *
                         PLAYER_SPRITE_SHEET_CELL_COUNT_HEIGHT));
  // TODO: Figure out a better way to manage player state. (probably use the
  // same commit system used in background and obj)
  gf_obj_set_int(player->obj, "spriteIndex", sprite_index);
  gf_obj_commit_state(player->obj);
  gf_obj_draw(player->obj);
}
