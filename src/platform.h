// Copyright 2024 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BENCH_GL_PLATFORM_H_
#define BENCH_GL_PLATFORM_H_

#define ID_PLATFORM_GLX 1
#define ID_PLATFORM_X11_EGL 2
#define ID_PLATFORM_NULL 3

#define CONCAT(a, b) a##b
#define PLATFORM_ID(x) CONCAT(ID_, x)
#define PLATFORM_ENUM(x) CONCAT(WAFFLE_, x)
#define THIS_IS(x) PLATFORM_ID(x) == PLATFORM_ID(PLATFORM)

#endif  // BENCH_GL_PLATFORM_H_
