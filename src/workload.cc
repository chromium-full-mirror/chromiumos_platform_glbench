// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cmath>
#include <iostream>
#include <vector>

#include "workload.h"

// Vertex buffer creation.
bool Workload::Initialize(int frame_count) {
#if defined (USE_OPENGLES)
  const char* exts = (const char*)glGetString(GL_EXTENSIONS);
  if (!strstr(exts, "GL_EXT_disjoint_timer_query")) {
    printf("# Error: QueryGetTest could not find query extension(s).\n");
    return false;
  }
#endif

  queries_.resize(frame_count);
  quad_counts_.resize(frame_count);
  glGenQueries(queries_.size(), queries_.data());

  glGenVertexArrays(1, &VAO_);
  glGenBuffers(1, &VBO_);
  glGenBuffers(1, &EBO_);
  glBindVertexArray(VAO_);

  glBindBuffer(GL_ARRAY_BUFFER, VBO_);
  glBufferData(GL_ARRAY_BUFFER,
               vertices_.size() * sizeof(GLfloat),
               vertices_.data(),
               GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               indices_.size() * sizeof(GLuint),
               indices_.data(),
               GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glUniform1f(1, 10); // uniform float mod_size
  glUniform1f(2, 1);  // uniform float line_idx
  glUniform1f(3, 10); // uniform float start_size
  return true;
}

// Bind vertex buffer with a set number of instanced quads.
void Workload::UpdateWorkload(int num_quads) {
  num_quads_ = num_quads;
  // triangle size needs to be scaled by the sqrt of the number of triangles.
  GLfloat mod_size = std::floor(std::sqrt(num_quads_));
  glUniform1f(1, mod_size);
}

void Workload::Draw() {
  // Clear the screen before any rendering happens.
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Create a vertical bar that moves across the screen.
  // Lets user know that application hasn't hung, and may help with detecting
  // screen tearing.
  static uint line_idx{1};
  ++line_idx;
  glUniform1f(2, line_idx);

  glDrawElementsInstanced(
        GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, num_quads_);
}

bool NearlyEqual(double gpu_elapsed_time, double gpu_workload_ms) {
  double workload_ms_error = 0.1f;
  return abs(gpu_elapsed_time - gpu_workload_ms) < workload_ms_error;
}

void Workload::IncreaseWorkload() {
  UpdateWorkload(num_quads_ * gpu_workload_increase_);
}

void Workload::DecreaseWorkload(bool record_quads) {
  UpdateWorkload(static_cast<int>(std::floor(
      num_quads_ * gpu_workload_precision_)));
  // num_quads_ has been updated. store new value for logging
  if (record_quads) {
    quad_counts_[num_throttles_] = num_quads_;
    ++num_throttles_;
  }
}

void Workload::CalibrateWorkload(double gpu_workload_ms) {
  // give 100 frames max to get close to workload
  int max_frames = 100;
  int loading_window = 10;
  // Start at a reasonable number of quads.
  UpdateWorkload(10000);

  double gpu_elapsed_time = 0;
  int calibrate_count = 0;
  for (int num_frames = 0; num_frames < max_frames; ++num_frames) {
    if (num_frames > loading_window) {
      if (NearlyEqual(gpu_elapsed_time, gpu_workload_ms)){
        if (calibrate_count++ == 1) break;
      } else if (gpu_elapsed_time < gpu_workload_ms) {
        IncreaseWorkload();
        std::cout << "Updated num quads to: " << num_quads_ << std::endl;
      } else {
        DecreaseWorkload();
        std::cout << "Updated num quads to: " << num_quads_ << std::endl;
      }
    }

    UpdateWorkload(num_quads_);
    StartTimer(num_frames);
    Draw();
    EndTimer();
    gpu_elapsed_time = GetFrameGpuTime(num_frames);
    std::cout << "calibrating: gpu_elapsed_time: "
              << gpu_elapsed_time << " ms" << std::endl;
  }
  glUniform1f(3, std::floor(std::sqrt(num_quads_)));
  if (calibrate_count == 0) {
    std::cout << "Warning: Did not hit calibration target.\n" << std::endl;
  }
  if (calibrate_count > 0) {
    std::cout << "Finished calibrating workload.\n" << std::endl;
  }
}

void Workload::StartTimer(int frame) {
  const GLuint query = queries_[frame % queries_.size()];
  glBeginQuery(GL_TIME_ELAPSED, query);
}

void Workload::EndTimer() {
  glEndQuery(GL_TIME_ELAPSED);
}

double Workload::GetFrameGpuTime(int frame) {
  GLuint gpu_elapsed_time;
  glGetQueryObjectuiv(queries_[frame], GL_QUERY_RESULT, &gpu_elapsed_time);
  return NS_TO_MS(gpu_elapsed_time);
}

void Workload::PrintQuadHistory() {
  for (int i = 0; i < num_throttles_; ++i) {
    std::cout << "Quad count decreased to: " << quad_counts_[i] << std::endl;
  }
}

void Workload::TakeDown() {
  glDeleteQueries(queries_.size(), queries_.data());
}
