// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPOSITOR_GL_WORKLOAD_H_
#define COMPOSITOR_GL_WORKLOAD_H_

#include "platform.h"

#include <epoxy/gl.h>

#if THIS_IS(PLATFORM_GLX)
#include <epoxy/glx.h>
#else // EGL platforms
#include <epoxy/egl.h>
#endif

#define NS_TO_MS(x) x / 1000000.0

class Workload {
 private:
  // Adjust workload by increasing or decreasing number of quads drawn.
  void UpdateWorkload(int num_quads);

  std::vector<GLuint> queries_;
  std::vector<int> quad_counts_;
  int num_throttles_ = 0;
  int num_quads_ = 100;
  double gpu_workload_precision_ = 0.99f;
  double gpu_workload_increase_ = 1.1;
  GLuint VBO_;
  GLuint VAO_;
  GLuint EBO_;

  std::vector<GLfloat> vertices_ = {
       0.05f,  0.05f, // top right vertex
       0.05f, -0.05f, // bottom right
      -0.05f, -0.05f, // bottom left
      -0.05f,  0.05f, // top left
  };
  std::vector<GLuint> indices_ = {
      0, 1, 3,  // first triangle
      1, 2, 3   // second triangle
  };

 public:
  // Set up GL buffer objects.
  bool Initialize(int frame_count);
  // Do some rendering work on the GPU based on a calibrated workload.
  void Draw();
  // Updates num_quads so that gpu_workload_ms of work is done on the GPU.
  void CalibrateWorkload(double gpu_workload_ms);
  void IncreaseWorkload();
  void DecreaseWorkload(bool record_quads = false);
  // Use the timers to measure how much time was spent on the GPU.
  // Start the timer. Must be followed by EndTimer() before it can be called
  // again. Use GetFrameGpuTime() to get the elapsed time for that time.
  void StartTimer(int frame);
  // Stop the timer. Call after StartTimer() to store elapsed time for a frame.
  void EndTimer();
  // Get the time spent on the GPU for a particular frame. Must have called
  // StartTimer()/EndTimer() for that frame or value wiill be wrong.
  double GetFrameGpuTime(int frame);
  // Print num_quads every time the workload was decreased.
  void PrintQuadHistory();
  // Unset GL state and delete objects.
  void TakeDown();
};

#endif // COMPOSITOR_GL_WORKLOAD_H_
