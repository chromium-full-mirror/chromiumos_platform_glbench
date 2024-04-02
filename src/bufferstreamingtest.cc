// Copyright 2019 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "arraysize.h"
#include "main.h"
#include "testbase.h"
#include "utils.h"

namespace glbench {

class BufferStreamingTest : public TestBase {
 public:
  BufferStreamingTest() : buffer_size_(1 * 1024 * 1024) {
    // clockwise such that the triangle is culled
    const GLfloat vertices[3][2] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
    };

    memcpy(data_, vertices, sizeof(vertices));
    data_size_ = sizeof(vertices);
  }

  virtual ~BufferStreamingTest() {}
  virtual bool TestFunc(uint64_t iterations);
  virtual bool Run();
  virtual const char* Name() const { return "buffer_streaming"; }
  virtual bool IsDrawTest() const { return false; }
  virtual const char* Unit() const { return "mbytes_sec"; }

 private:
  enum streaming_mode {
    STREAMING_MODE_SUBDATA_OVERWRITE,
    STREAMING_MODE_SUBDATA,

    STREAMING_MODE_COUNT,
  };

  const GLsizeiptr buffer_size_;

  GLbyte data_[sizeof(GLfloat) * 2 * 3];
  GLsizeiptr data_size_;

  enum streaming_mode streaming_mode_;

  const char* CurrentStreamingModeName() const {
    switch (streaming_mode_) {
      case STREAMING_MODE_SUBDATA:
        return "subdata";
      case STREAMING_MODE_SUBDATA_OVERWRITE:
        return "subdata_overwrite";
      default:
        return "invalid";
    }
  }

  bool TestSubData(uint64_t iterations);
  bool TestSubDataOverwrite(uint64_t iterations);

  DISALLOW_COPY_AND_ASSIGN(BufferStreamingTest);
};

bool BufferStreamingTest::TestSubData(uint64_t iterations) {
  GLintptr offset = 0;
  GLint index = 0;

  for (uint32_t i = 0; i < iterations; i++) {
    if (offset + data_size_ > buffer_size_) {
      glBufferData(GL_ARRAY_BUFFER, buffer_size_, NULL, GL_STREAM_DRAW);
      offset = 0;
      index = 0;
    }

    glBufferSubData(GL_ARRAY_BUFFER, offset, data_size_, data_);
    glDrawArrays(GL_TRIANGLES, index, 3);

    offset += data_size_;
    index += 3;
  }

  return true;
}

bool BufferStreamingTest::TestSubDataOverwrite(uint64_t iterations) {
  for (uint32_t i = 0; i < iterations; i++) {
    glBufferSubData(GL_ARRAY_BUFFER, 0, data_size_, data_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  return true;
}

bool BufferStreamingTest::TestFunc(uint64_t iterations) {
  // We should always start with a new buffer.
  glBufferData(GL_ARRAY_BUFFER, buffer_size_, NULL, GL_STREAM_DRAW);

  switch (streaming_mode_) {
    case STREAMING_MODE_SUBDATA:
      return TestSubData(iterations);
    case STREAMING_MODE_SUBDATA_OVERWRITE:
      return TestSubDataOverwrite(iterations);
    default:
      return false;
  }
}

bool BufferStreamingTest::Run() {
  const char vs[] =
      "attribute vec4 pos;"
      "void main() {"
      "  gl_Position = pos;"
      "}";

  const char fs[] =
      "void main() {"
      "  gl_FragColor = vec4(1.0);"
      "}";
  GLuint program = InitShaderProgram(vs, fs);

  GLuint buffer;
  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);

  GLint attribute_index = glGetAttribLocation(program, "pos");
  glVertexAttribPointer(attribute_index, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(attribute_index);

  glViewport(0, 0, g_width, g_height);
  // Let's cull after VS such that the tests are not FS or I/O bound.
  glEnable(GL_CULL_FACE);

  for (int i = 0; i < STREAMING_MODE_COUNT; i++) {
    streaming_mode_ = static_cast<enum streaming_mode>(i);

    const std::string name =
        std::string(Name()) + "_" + CurrentStreamingModeName();
    RunTest(this, name.c_str(), data_size_, g_width, g_height, true);
    CHECK(!glGetError());
  }

  glDeleteBuffers(1, &buffer);
  glDeleteProgram(program);

  return true;
}

TestBase* GetBufferStreamingTest() {
  return new BufferStreamingTest;
}

}  // namespace glbench
