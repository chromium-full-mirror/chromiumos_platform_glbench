// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define WAFFLE_API_VERSION 0x0106

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <waffle.h>

#include "platform.h"
#include "workload.h"

#if THIS_IS(PLATFORM_GLX)
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "waffle_glx.h"
#define PLATFORM_TYPE WAFFLE_PLATFORM_GLX
#elif THIS_IS(PLATFORM_NULL)
#include "waffle_null.h"
#define PLATFORM_TYPE WAFFLE_PLATFORM_NULL
#elif THIS_IS(PLATFORM_X11_EGL)
#include "waffle_x11_egl.h"
#define PLATFORM_TYPE WAFFLE_PLATFORM_X11_EGL
#endif

using namespace std::chrono;

#if defined(USE_OPENGLES)
#define PLATFORM_API WAFFLE_CONTEXT_OPENGL_ES2
const std::string kVertexHeader = R"STRING(
#version 320 es

)STRING";

const std::string kFragmentHeader = R"STRING(
#version 320 es
precision mediump float;

)STRING";
#else
#define _NET_WM_STATE_REMOVE  0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE  2

#define PLATFORM_API WAFFLE_CONTEXT_OPENGL

const std::string kVertexHeader = R"STRING(
#version 430 core

)STRING";

const std::string kFragmentHeader = R"STRING(
#version 430 core

)STRING";
#endif

const std::string kVertexShader = R"STRING(
layout (location = 0) in vec2 pos;
layout (location = 1) uniform float mod_size;
layout (location = 2) uniform float line_idx;
layout (location = 3) uniform float start_size;
out vec2 instance_color;
out float blue_intensity;
void main()
{
    float gli = float(gl_InstanceID);
    float h = gli / mod_size;
    float l = mod(gli, mod_size);
    float vx = 0.5f - l / mod_size;
    float vy = 0.5f - h / mod_size;
    float a = mod(line_idx, mod_size);
    blue_intensity = mod_size / start_size;
    if (blue_intensity >= 1.0f) {
      blue_intensity = 1.0f;
    }
    if (abs(a - l) < mod_size * 0.05) {
      instance_color = vec2(1, 1);
    } else {
      instance_color = vec2(vx + 0.5f, vy + 0.5f);
      blue_intensity = 0.4f;
    }

    gl_Position =
        vec4(pos.x - 2.0f * vx, pos.y + 2.0f * vy, 0, 1.0f);
}
)STRING";

const std::string kFragmentShader = R"STRING(
in vec2 instance_color;
in float blue_intensity;
out vec4 frag_color;
void main()
{
  frag_color = vec4(instance_color.x, instance_color.y, blue_intensity, 1.0f);
}
)STRING";

unsigned int CreateShader(int* error) {
  // Build and compile our shader programs.
  // Vertex shader.
  unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  auto temp = kVertexHeader + kVertexShader;
  const char *full_vertex = temp.data();
  glShaderSource(vertex_shader, 1, &full_vertex, NULL);
  glCompileShader(vertex_shader);
  // check for shader compile errors
  int success;
  char info_log[512];
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
    std::cout << "Error: Vertex shader compilation failed." << info_log
              << std::endl;
  }
  // Fragment shader.
  unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  temp = kFragmentHeader + kFragmentShader;
  const char *full_fragment = temp.data();
  glShaderSource(fragment_shader, 1, &full_fragment, NULL);
  glCompileShader(fragment_shader);
  // check for shader compile errors
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
    std::cout << "Error: Fragment shader compilation failed." << info_log
              << std::endl;
  }
  // link shaders
  unsigned int shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);
  // check for linking errors
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  if (!success) {
    glGetProgramInfoLog(shader_program, 512, NULL, info_log);
    std::cout << "Error: Shader linking failed." << info_log << std::endl;
    *error = -1;
  }

  return shader_program;
}

void CreateContext(
  struct waffle_display **display,
  struct waffle_config **config,
  struct waffle_window **window,
  struct waffle_context **ctx,
  int swap_interval,
  int window_width,
  int window_height,
  bool fullscreen) {
  const int32_t init_attrs[] = {
      WAFFLE_PLATFORM, PLATFORM_TYPE,
      0,
  };

  const int32_t config_attrs[] = {
                             WAFFLE_CONTEXT_API, PLATFORM_API,
                             WAFFLE_RED_SIZE, 8,
                             WAFFLE_GREEN_SIZE, 8,
                             WAFFLE_BLUE_SIZE, 8,
                             WAFFLE_ALPHA_SIZE, 8,
                             WAFFLE_DEPTH_SIZE, 8,
                             WAFFLE_STENCIL_SIZE, 8,
                             WAFFLE_DOUBLE_BUFFERED, true,
                             0};

  waffle_init(init_attrs);
  *display = waffle_display_connect(NULL);

  *config = waffle_config_choose(*display, config_attrs);
  if (fullscreen) {
    const intptr_t attrib[] = {WAFFLE_WINDOW_FULLSCREEN, 1, 0};
    *window = waffle_window_create2(*config, attrib);

#if THIS_IS(PLATFORM_GLX)
    struct waffle_glx_window *window_glx =
        waffle_window_get_native(*window)->glx;
    Display *display_x11 = window_glx->xlib_display;
    Window window_x11 = (Window) window_glx->xlib_window;

    XEvent ev;
    Atom atom;

    ev.type = ClientMessage;
    ev.xclient.window = window_x11;
    ev.xclient.send_event = True;
    ev.xclient.message_type = XInternAtom(display_x11, "_NET_WM_STATE", False);
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = _NET_WM_STATE_ADD;
    atom = XInternAtom(display_x11, "_NET_WM_STATE_FULLSCREEN", False);
    ev.xclient.data.l[1] = atom;
    ev.xclient.data.l[2] = 0;
    ev.xclient.data.l[3] = 1;
    XSendEvent(display_x11, DefaultRootWindow(display_x11), False,
        SubstructureNotifyMask | SubstructureRedirectMask, &ev);
#endif
  } else {
    *window = waffle_window_create(*config, window_width, window_height);
  }

  waffle_window_show(*window);
  *ctx = waffle_context_create(*config, NULL);
  waffle_make_current(*display, *window, *ctx);

  union waffle_native_display *native_display =
      waffle_display_get_native(*display);
#if THIS_IS(PLATFORM_GLX)
  glXSwapIntervalEXT(native_display->glx->xlib_display,
      glXGetCurrentDrawable(), swap_interval);
#elif THIS_IS(PLATFORM_NULL)
  eglSwapInterval(native_display->null->egl_display, swap_interval);
#elif THIS_IS(PLATFORM_X11_EGL)
  eglSwapInterval(native_display->x11_egl->egl_display, swap_interval);
#endif
}

int main(int argc, char* argv[]) {
  // TODO(mrfemi): parse workload scale from command line. At least as a
  // parameter. (0.99f)
  bool vsync = true;
  bool fullscreen = false;
  bool target_refresh_rate = true;
  int total_frames = 240;
  int sleep_cpu_milliseconds = 0;
  int width = 1280;
  int height = 720;
  double display_scale = 1.0;
  double gpu_workload_ms = 1000 / 60.0;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--vsync") {
      vsync = true;
    } else if (arg == "--no-vsync") {
      vsync = false;
    } else if (arg == "--fullscreen") {
      fullscreen = true;
    } else if (arg == "--no-fullscreen") {
      fullscreen = false;
    } else if (arg == "--target-refresh-rate") {
      target_refresh_rate = true;
    } else if (arg == "--no-target-refresh-rate") {
      target_refresh_rate = false;
    } else if (arg == "--sleep-cpu-milliseconds" && (i + 1) < argc) {
      sleep_cpu_milliseconds = std::stod(argv[i + 1]);
      ++i;
    } else if (arg == "--total-frames" && (i + 1) < argc) {
      total_frames = std::stod(argv[i + 1]);
      ++i;
    } else if (arg == "--width" && (i + 1) < argc) {
      width = std::stoi(argv[i + 1]);
      ++i;
    } else if (arg == "--height" && (i + 1) < argc) {
      height = std::stoi(argv[i + 1]);
      ++i;
    } else if (arg == "--display-scale" && (i + 1) < argc) {
      display_scale = std::stod(argv[i + 1]);
      ++i;
    } else if (arg == "--gpu-workload-ms" && (i + 1) < argc) {
      gpu_workload_ms = std::stod(argv[i + 1]);
      ++i;
    } else {
      std::cout << "Error: Parsing command line - check args: "
                << arg << std::endl;
      return -1;
    }
  }
  std::cout << "Settings: "
            << "\nvsync=" << (vsync ? "True" : "False")
            << "\nwidth=" << width
            << "\nheight=" << height
            << "\ntotal_frames=" << total_frames
            << "\nfullscreen=" << (fullscreen ? "True" : "False")
            << "\ntarget_refresh_rate=" << (target_refresh_rate ?
                                            "True" : "False")
            << "\nsleep_cpu_milliseconds=" << sleep_cpu_milliseconds
            << "\ndisplay_scale=" << display_scale
            << "\ngpu_workload_ms=" << gpu_workload_ms
            << std::endl;

  // Context creation.
  struct waffle_display *display;
  struct waffle_config *config;
  struct waffle_window *window;
  struct waffle_context *ctx;
  // Enable or disable vsync
  int swap_interval = vsync ? 1 : 0;

  width = static_cast<int>(floor(width * display_scale));
  height = static_cast<int>(floor(height * display_scale));
  CreateContext(&display, &config, &window, &ctx, swap_interval, width, height,
                fullscreen);
  int error = 0;
  auto shader_program = CreateShader(&error);
  std::cout << "Platform OpenGL version: "
            << glGetString(GL_VERSION) << std::endl;
  if (error < 0)
    return -1;
  glUseProgram(shader_program);

  // String format floating points to 4 decimal places.
  std::cout << std::fixed << std::setprecision(4);
  std::vector<double> cpu_times(total_frames);

  // Set up workload class.
  auto workload = new Workload();
  if(!workload->Initialize(total_frames))
    return -1;

  // Check if we should target the display refresh rate.
  if (target_refresh_rate) {
    waffle_window_swap_buffers(window);
    auto start_time = steady_clock::now();
    int num_swaps = 0;
    while (num_swaps < 60) {
      ++num_swaps;
      workload->Draw();
      waffle_window_swap_buffers(window);
    }
    auto current_time = steady_clock::now();
    auto total_time = duration<double, std::milli>(
        current_time - start_time).count();
    gpu_workload_ms = total_time / num_swaps;
    std::cout << "Detecting refresh rate of " << gpu_workload_ms << " ms.\n"
              << std::endl;
  }
  // Adjust the workload so the gpu spends gpu_workload_ms rendering a frame.
  workload->CalibrateWorkload(gpu_workload_ms);
  waffle_window_swap_buffers(window);
  waffle_window_swap_buffers(window);
  waffle_window_swap_buffers(window);

  // Display loop.
  auto first_time = steady_clock::now();
  auto prev_time = first_time;
  waffle_window_swap_buffers(window);
  for (int frame = 0; frame < total_frames; ++frame) {
    auto current_time = steady_clock::now();
    cpu_times[frame] =
        duration<double, std::milli>(current_time - prev_time).count();
    prev_time = current_time;

    if (cpu_times[frame] > gpu_workload_ms * 1.5) {
      // TODO(mrfemi): consider this a dropped frame and mark somehow
      // But this can also be processed from the logs.
      workload->DecreaseWorkload(/*record_quads=*/true);
    }

    // Do not bother drawing if sleep_cpu_milliseconds has been set.
    if (sleep_cpu_milliseconds == 0) {
      workload->StartTimer(frame);
      workload->Draw();
      workload->EndTimer();
    } else {
      std::this_thread::sleep_for(milliseconds(sleep_cpu_milliseconds));
    }

    waffle_window_swap_buffers(window);
  }
  auto current_time = steady_clock::now();
  auto total_time = duration<double>(
      current_time - first_time).count();

  // Print performance numbers to stdout.
  workload->PrintQuadHistory();
  for (int i = 0; i < total_frames; ++i) {
    std::cout << "frame " << i << ": "
              << "cpu_elapsed_time: " << cpu_times[i] << " ms" << std::endl;
    if (sleep_cpu_milliseconds > 0) {
      std::cout << "frame sleep: "
                << sleep_cpu_milliseconds << " ms" << std::endl;
      continue;
    }
    auto gpu_elapsed_time = workload->GetFrameGpuTime(i);
    std::cout << "frame " << i << ": "
              << "gpu_elapsed_time: " << gpu_elapsed_time << " ms" << std::endl;
  }
  workload->TakeDown();

  std::cout << "Avg: " << total_frames / total_time << " fps for "
            << total_time << " secs. "
            << total_frames << " frames rendered." << std::endl;
  return 0;
}
