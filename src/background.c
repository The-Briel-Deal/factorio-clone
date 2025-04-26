
#include "common.h"
#include "log.h"
#include "render.h"
struct gf_background {
  struct gf_obj *obj;
};
STATIC_LIST(gf_background_list, struct gf_background, 128)

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
  struct gf_shader* shader = gf_compile_shaders();
}
