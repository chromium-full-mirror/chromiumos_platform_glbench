// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define WAFFLE_API_VERSION 0x0106

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <waffle.h>

#include "workload.h"

#if defined(USE_OPENGLES)
#include <epoxy/gl.h>
#include <epoxy/egl.h>
#include "waffle_null.h"

#elif defined(USE_OPENGL)
#include <epoxy/gl.h>
#include <epoxy/glx.h>
#include "waffle_glx.h"

#else
#error bad graphics backend
#endif

using namespace std::chrono;

#if defined(USE_OPENGLES)
#define PLATFORM_API WAFFLE_CONTEXT_OPENGL_ES2
#define PLATFORM_TYPE WAFFLE_PLATFORM_NULL
const std::string kVertexHeader = R"STRING(
#version 320 es

)STRING";

const std::string kFragmentHeader = R"STRING(
#version 320 es
precision mediump float;

)STRING";
#else
#define PLATFORM_API WAFFLE_CONTEXT_OPENGL
#define PLATFORM_TYPE WAFFLE_PLATFORM_GLX

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
out vec2 instance_color;
void main()
{
    float gli = float(gl_InstanceID);
    float h = gli / mod_size;
    float l = mod(gli, mod_size);
    float vx = 0.5f - l / mod_size;
    float vy = 0.5f - h / mod_size;
    float a = mod(line_idx, mod_size);
    if (abs(a - l) < mod_size * 0.05) {
      instance_color = vec2(1, 1);
    } else {
      instance_color = vec2(vx + 0.5f, vy + 0.5f);
    }

    gl_Position =
        vec4(pos.x - 2.0f * vx, pos.y + 2.0f * vy, 0, 1.0f);
}
)STRING";

const std::string kFragmentShader = R"STRING(
in vec2 instance_color;
out vec4 frag_color;
void main()
{
  frag_color = vec4(instance_color.x, instance_color.y, 0.4f, 1.0f);
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
  int swap_interval) {
  const int32_t init_attrs[] = {
      WAFFLE_PLATFORM, PLATFORM_TYPE,
      0,
  };

  const int32_t config_attrs[] = {
                             WAFFLE_CONTEXT_API, PLATFORM_API,
                             WAFFLE_RED_SIZE, 1,
                             WAFFLE_GREEN_SIZE, 1,
                             WAFFLE_BLUE_SIZE, 1,
                             WAFFLE_ALPHA_SIZE, 1,
                             WAFFLE_DEPTH_SIZE, 1,
                             WAFFLE_STENCIL_SIZE, 1,
                             WAFFLE_DOUBLE_BUFFERED, true,
                             0};
  const int32_t window_width = 640;
  const int32_t window_height = 480;

  waffle_init(init_attrs);
  *display = waffle_display_connect(NULL);

  *config = waffle_config_choose(*display, config_attrs);
  *window = waffle_window_create(*config, window_width, window_height);

  waffle_window_show(*window);
  *ctx = waffle_context_create(*config, NULL);
  waffle_make_current(*display, *window, *ctx);
#if defined (USE_OPENGLES)
  eglSwapInterval(waffle_display_get_native(*display)->null->egl_display,
      swap_interval);
#elif defined (USE_OPENGL)
  glXSwapIntervalEXT(waffle_display_get_native(*display)->glx->xlib_display,
      glXGetCurrentDrawable(), swap_interval);
#endif
}

int main(int argc, char* argv[]) {
  // TODO(mrfemi): Check for GLX_MESA_swap_control extension and set vsync
  // appropriately.
  // TODO(mrfemi): parse workload scale from command line. At least as a
  // parameter. (0.99f)
  bool vsync = true;
  bool fullscreen = true;
  bool target_refresh_rate = true;
  int sleep_cpu_microseconds = 0;
  double display_scale = 1.0;
  double gpu_workload_ms = 1000 / 60.0;

  // Enable or disable vsync
  int swap_interval = vsync ? 1 : 0;

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
    } else if (arg == "--sleep-cpu-microseconds" && (i + 1) < argc) {
      sleep_cpu_microseconds = std::stoi(argv[i + 1]);
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
            << "\nfullscreen=" << (fullscreen ? "True" : "False")
            << "\ntarget_refresh_rate=" << (target_refresh_rate ?
                                            "True" : "False")
            << "\nsleep_cpu_microseconds=" << sleep_cpu_microseconds
            << "\ndisplay_scale=" << display_scale
            << "\ngpu_workload_ms=" << gpu_workload_ms
            << std::endl;

  // Context creation.
  struct waffle_display *display;
  struct waffle_config *config;
  struct waffle_window *window;
  struct waffle_context *ctx;
  CreateContext(&display, &config, &window, &ctx, swap_interval);

  int error = 0;
  auto shader_program = CreateShader(&error);
  std::cout << "Platform OpenGL version: "
            << glGetString(GL_VERSION) << std::endl;
  if (error < 0)
    return -1;
  glUseProgram(shader_program);

  // String format floating points to 4 decimal places.
  std::cout << std::fixed << std::setprecision(4);

  // Set number of frames to test.
  int total_frames = 1000;
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
      workload->DecreaseWorkload();
    }

    workload->StartTimer(frame);
    workload->Draw();
    workload->EndTimer();

    waffle_window_swap_buffers(window);
  }
  auto current_time = steady_clock::now();
  auto total_time = std::chrono::duration_cast<seconds>(
      current_time - first_time).count();

  for (int i = 0; i < total_frames; ++i) {
    std::cout << "frame " << i << ": "
              << "cpu_elapsed_time: " << cpu_times[i] << " ms" << std::endl;
    auto gpu_elapsed_time = workload->GetFrameGpuTime(i);
    std::cout << "frame " << i << ": "
              << "gpu_elapsed_time: " << gpu_elapsed_time << " ms" << std::endl;
  }

  std::cout << "Avg: " << total_frames / total_time << " fps for "
            << std::chrono::seconds(total_time).count() << " secs. "
            << total_frames << " frames rendered." << std::endl;
  return 0;
}
