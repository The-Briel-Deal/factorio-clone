#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GL/gl.h>
#include <assert.h>
#include <stdbool.h>
#include <wayland-client-core.h>
#include <wayland-egl-core.h>

#include "egl.h"
#include "log.h"

static const EGLint config_attribs[] = {
    EGL_SURFACE_TYPE,
    EGL_WINDOW_BIT,
    EGL_CONFORMANT,
    EGL_OPENGL_BIT,
    EGL_RENDERABLE_TYPE,
    EGL_OPENGL_BIT,
    EGL_COLOR_BUFFER_TYPE,
    EGL_RGB_BUFFER,

    EGL_RED_SIZE,
    8,
    EGL_GREEN_SIZE,
    8,
    EGL_BLUE_SIZE,
    8,
    EGL_DEPTH_SIZE,
    24,
    EGL_STENCIL_SIZE,
    8,

    EGL_NONE,
};

static const EGLint surface_attribs[] = {
    EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_LINEAR,
    EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
    EGL_NONE,
};

static const EGLint context_attribs[] = {
    EGL_CONTEXT_MAJOR_VERSION,
    4,
    EGL_CONTEXT_MINOR_VERSION,
    5,
    EGL_CONTEXT_OPENGL_PROFILE_MASK,
    EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
#ifndef NDEBUG

    EGL_CONTEXT_OPENGL_DEBUG,
    EGL_TRUE,
#endif
    EGL_NONE,
};

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
                                GLenum severity, GLsizei length,
                                const GLchar *message, const void *userParam) {
  enum log_severity gf_log_severity = -1;
  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH        : gf_log_severity = ERROR_LOG; break;
    case GL_DEBUG_SEVERITY_MEDIUM      : gf_log_severity = DEBUG_LOG; break;
    case GL_DEBUG_SEVERITY_LOW         :
    case GL_DEBUG_SEVERITY_NOTIFICATION: gf_log_severity = INFO_LOG; break;
  }
  // If this isn't set by the above switch, I probably missed a case.
  assert(gf_log_severity != -1);
  gf_log(gf_log_severity, "(GL) %s", message);
}

bool init_egl(struct gf_egl_state *gf_egl_state, struct wl_display *wl_display,
              struct wl_egl_window *wl_egl_window) {
  gf_egl_state->display = eglGetDisplay(wl_display);
  assert(gf_egl_state->display != EGL_NO_DISPLAY);
  EGLBoolean success;
  EGLint major;
  EGLint minor;
  success = eglInitialize(gf_egl_state->display, &major, &minor);
  assert(success);
  gf_log(INFO_LOG, "EGL Version - v%i.%i", major, minor);

  EGLBoolean ok = eglBindAPI(EGL_OPENGL_API);
  assert(ok);

  /* config selection */
  EGLConfig config;
  EGLint num_config;
  success = eglChooseConfig(gf_egl_state->display, config_attribs, &config, 1,
                            &num_config);
  assert(success && num_config == 1);

  /* create context */
  gf_egl_state->context = eglCreateContext(gf_egl_state->display, config,
                                           EGL_NO_CONTEXT, context_attribs);
  assert(gf_egl_state->context != EGL_NO_CONTEXT);

  /* Init egl_surface */
  gf_egl_state->surface = eglCreateWindowSurface(
      gf_egl_state->display, config, (EGLNativeWindowType)wl_egl_window,
      surface_attribs);
  assert(gf_egl_state->surface != NULL);

  eglMakeCurrent(gf_egl_state->display, gf_egl_state->surface,
                 gf_egl_state->surface, gf_egl_state->context);

  // During init, enable debug output
  int flags;
  glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
  if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
    gf_log(DEBUG_LOG, "You are in a debug context!");
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(MessageCallback, 0);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL,
                          GL_TRUE);
  }
  return true;
}
