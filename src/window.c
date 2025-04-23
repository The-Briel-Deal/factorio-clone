
#include <GL/gl.h>

#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-egl-core.h>
#include <xkbcommon/xkbcommon.h>

#include "common.h"
#include "cursor-shape.h"
#include "render.h"
#include "xdg-shell.h"

#include "log.h"
#include "window.h"

// TODO: Turn this into a list of input listeners.
static gf_keyboard_input_listener keyboard_input_listener = NULL;
static void *keyboard_input_listener_data                 = NULL;

void gf_window_register_input_listener(gf_keyboard_input_listener listener,
                                       void *data) {
  keyboard_input_listener      = listener;
  keyboard_input_listener_data = data;
}

/* wl_registry Listener */
static void global_registry_handler(void *data, struct wl_registry *registry,
                                    uint32_t id, const char *interface,
                                    uint32_t version) {
  struct gf_window *window = data;

  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    window->compositor =
        wl_registry_bind(registry, id, &wl_compositor_interface, 6);
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    window->xdg_wm_base =
        wl_registry_bind(registry, id, &xdg_wm_base_interface, 5);
  } else if (strcmp(interface, wp_cursor_shape_manager_v1_interface.name) ==
             0) {
    window->cursor_shape_manager = wl_registry_bind(
        registry, id, &wp_cursor_shape_manager_v1_interface, 1);
  } else if (strcmp(interface, wl_seat_interface.name) == 0) {
    window->wl_seat = wl_registry_bind(registry, id, &wl_seat_interface, 9);
  }
}
static void global_registry_remover(void *data, struct wl_registry *registry,
                                    uint32_t id) {
  gf_log(INFO_LOG, "wl_registry.remove() called");
}
static const struct wl_registry_listener registry_listener = {
    global_registry_handler,
    global_registry_remover,
};


/* xdg_wm_base Listener */
static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base,
                             uint32_t serial) {
  xdg_wm_base_pong(xdg_wm_base, serial);
}
static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};


/* xdg_surface Listener */
static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
                                  uint32_t serial) {
  xdg_surface_ack_configure(xdg_surface, serial);
}
static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};


/* xdg_toplevel Listener */
int32_t old_w = GF_DEFAULT_WINDOW_WIDTH, old_h = GF_DEFAULT_WINDOW_HEIGHT;
static void xdg_toplevel_configure(void *data,
                                   struct xdg_toplevel *xdg_toplevel, int32_t w,
                                   int32_t h, struct wl_array *states) {
  struct gf_window *window = data;
  // no window geometry event, ignore
  if (w == 0 && h == 0)
    return;

  // window resized
  if (old_w != w || old_h != h) {
    old_w = w;
    old_h = h;

    glViewport(0, 0, w, h);
    wl_egl_window_resize(window->wl_egl_window, w, h, 0, 0);
    wl_surface_commit(window->surface);
    gf_draw_update_window_size(w, h);
  }
}
static void xdg_toplevel_wm_capabilities(void *data,
                                         struct xdg_toplevel *xdg_toplevel,
                                         struct wl_array *capabilities) {
  int *capability;
  wl_array_for_each(capability, capabilities) {
    switch (*capability) {
      case XDG_TOPLEVEL_WM_CAPABILITIES_WINDOW_MENU:
        gf_log(INFO_LOG,
               "xdg_toplevel wm_capability: "
               "XDG_TOPLEVEL_WM_CAPABILITIES_WINDOW_MENU");
        break;
      case XDG_TOPLEVEL_WM_CAPABILITIES_MAXIMIZE:
        gf_log(INFO_LOG,
               "xdg_toplevel wm_capability: "
               "XDG_TOPLEVEL_WM_CAPABILITIES_MAXIMIZE");
        break;
      case XDG_TOPLEVEL_WM_CAPABILITIES_FULLSCREEN:
        gf_log(INFO_LOG,
               "xdg_toplevel wm_capability: "
               "XDG_TOPLEVEL_WM_CAPABILITIES_FULLSCREEN");
        break;
      case XDG_TOPLEVEL_WM_CAPABILITIES_MINIMIZE:
        gf_log(INFO_LOG,
               "xdg_toplevel wm_capability: "
               "XDG_TOPLEVEL_WM_CAPABILITIES_MINIMIZE");
        break;
    }
  }
}
static const struct xdg_toplevel_listener toplevel_listener = {
    .configure       = xdg_toplevel_configure,
    .wm_capabilities = xdg_toplevel_wm_capabilities,
};


static void wl_pointer_leave(void *data, struct wl_pointer *wl_pointer,
                             uint32_t serial, struct wl_surface *surface) {
  gf_log(INFO_LOG, "wl_pointer.leave() called");
}
static void wl_pointer_enter(void *data, struct wl_pointer *wl_pointer,
                             uint32_t serial, struct wl_surface *surface,
                             wl_fixed_t surface_x, wl_fixed_t surface_y) {
  gf_log(INFO_LOG, "wl_pointer.enter() called");
  struct gf_window *window = data;

  wp_cursor_shape_device_v1_set_shape(window->cursor_shape_device, serial,
                                      WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_HELP);
}

static void wl_pointer_frame(void *data, struct wl_pointer *wl_pointer) {
  gf_log(INFO_LOG, "wl_pointer.frame() called");
}

static void wl_pointer_motion(void *data, struct wl_pointer *wl_pointer,
                              uint32_t time, wl_fixed_t surface_x,
                              wl_fixed_t surface_y) {
  gf_log(INFO_LOG, "wl_pointer.motion() called");
}

static void wl_pointer_button(void *data, struct wl_pointer *wl_pointer,
                              uint32_t serial, uint32_t time, uint32_t button,
                              uint32_t state) {
  gf_log(INFO_LOG, "wl_pointer.button() called");
}

// TODO: Add axis (scrolling) events.
static const struct wl_pointer_listener pointer_listener = {
    .leave  = wl_pointer_leave,
    .enter  = wl_pointer_enter,
    .frame  = wl_pointer_frame,
    .motion = wl_pointer_motion,
    .button = wl_pointer_button,
};

static void wl_keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard,
                               uint32_t format, int32_t fd, uint32_t size) {
  struct gf_window *window = data;

  gf_log(INFO_LOG, "wl_keyboard.keymap() called");
  assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);

  char *keymap_shm = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  assert(keymap_shm != MAP_FAILED);
  window->xkb_keymap = xkb_keymap_new_from_string(
      window->xkb_context, keymap_shm, XKB_KEYMAP_FORMAT_TEXT_V1,
      XKB_KEYMAP_COMPILE_NO_FLAGS);
  assert(window->xkb_keymap != NULL);

  int e;
  e = munmap(keymap_shm, size);
  assert(e == 0);
  e = close(fd);
  assert(e == 0);

  window->xkb_state = xkb_state_new(window->xkb_keymap);
  assert(window->xkb_state != NULL);
}

static void wl_keyboard_enter(void *data, struct wl_keyboard *wl_keyboard,
                              uint32_t serial, struct wl_surface *surface,
                              struct wl_array *keys) {
  gf_log(INFO_LOG, "wl_keyboard.enter() called");
}
static void wl_keyboard_leave(void *data, struct wl_keyboard *wl_keyboard,
                              uint32_t serial, struct wl_surface *surface) {
  gf_log(INFO_LOG, "wl_keyboard.leave() called");
}

static void wl_keyboard_key(void *data, struct wl_keyboard *wl_keyboard,
                            uint32_t serial, uint32_t time, uint32_t key,
                            uint32_t state) {
  struct gf_window *window = data;
  xkb_keysym_t keysym = xkb_state_key_get_one_sym(window->xkb_state, key + 8);
  gf_log(INFO_LOG, "wl_keyboard.key() called");
  if (keyboard_input_listener != NULL && keyboard_input_listener_data != NULL) {
    keyboard_input_listener(keysym, state == WL_KEYBOARD_KEY_STATE_PRESSED,
                            keyboard_input_listener_data);
  } else {
    gf_log(DEBUG_LOG,
           "wl_keyboard.key() was called, but keyboard_input_listener or "
           "keyboard_input_listener_data was NULL");
  }
}

static void wl_keyboard_modifiers(void *data, struct wl_keyboard *wl_keyboard,
                                  uint32_t serial, uint32_t mods_depressed,
                                  uint32_t mods_latched, uint32_t mods_locked,
                                  uint32_t group) {
  gf_log(INFO_LOG, "wl_keyboard.modifiers() called");
}

static void wl_keyboard_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
                                    int32_t rate, int32_t delay) {
  gf_log(INFO_LOG, "wl_keyboard.repeat_info() called");
}


static const struct wl_keyboard_listener keyboard_listener = {
    .keymap      = wl_keyboard_keymap,
    .enter       = wl_keyboard_enter,
    .leave       = wl_keyboard_leave,
    .key         = wl_keyboard_key,
    .modifiers   = wl_keyboard_modifiers,
    .repeat_info = wl_keyboard_repeat_info,
};

// TODO: Rename to gf_window_init
bool init_gf_window(struct gf_window *window) {
  int err;

  /* connect to compositor */
  window->display = wl_display_connect(NULL);
  assert(window->display != NULL);

  struct wl_registry *registry = wl_display_get_registry(window->display);
  wl_registry_add_listener(registry, &registry_listener, window);

  wl_display_dispatch(window->display);
  wl_display_roundtrip(window->display);
  assert(window->xdg_wm_base != NULL);
  assert(window->compositor != NULL);

  xdg_wm_base_add_listener(window->xdg_wm_base, &xdg_wm_base_listener, NULL);

  /* create wl_surface */
  window->surface = wl_compositor_create_surface(window->compositor);
  assert(window->surface != NULL);

  /* create xdg_surface */
  window->xdg_surface =
      xdg_wm_base_get_xdg_surface(window->xdg_wm_base, window->surface);
  assert(window->xdg_surface != NULL);

  err = xdg_surface_add_listener(window->xdg_surface, &xdg_surface_listener,
                                 NULL);
  assert(err == 0);

  /* create toplevel */
  window->xdg_toplevel = xdg_surface_get_toplevel(window->xdg_surface);
  assert(window->xdg_toplevel);
  xdg_toplevel_set_title(window->xdg_toplevel, "test title");
  err = xdg_toplevel_add_listener(window->xdg_toplevel, &toplevel_listener,
                                  window);
  assert(err == 0);
  wl_surface_commit(window->surface);

  /* create wl window */
  window->wl_egl_window = wl_egl_window_create(
      window->surface, GF_DEFAULT_WINDOW_WIDTH, GF_DEFAULT_WINDOW_HEIGHT);
  assert(window->wl_egl_window != NULL);

  /* get pointer */
  window->wl_pointer          = wl_seat_get_pointer(window->wl_seat);
  window->cursor_shape_device = wp_cursor_shape_manager_v1_get_pointer(
      window->cursor_shape_manager, window->wl_pointer);

  wl_pointer_add_listener(window->wl_pointer, &pointer_listener, window);

  /* get keyboard & make xkb_context */
  window->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

  window->wl_keyboard = wl_seat_get_keyboard(window->wl_seat);
  wl_keyboard_add_listener(window->wl_keyboard, &keyboard_listener, window);

  return window;
}
