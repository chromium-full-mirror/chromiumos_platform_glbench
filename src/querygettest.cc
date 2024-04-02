// Copyright 2019 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "main.h"
#include "testbase.h"
#include "utils.h"

#if defined(USE_OPENGLES)
#define GL_ANY_SAMPLES_PASSED GL_ANY_SAMPLES_PASSED_EXT
#define GL_QUERY_RESULT GL_QUERY_RESULT_EXT
#define glBeginQuery glBeginQueryEXT
#define glEndQuery glEndQueryEXT
#define glDeleteQueries glDeleteQueriesEXT
#define glGenQueries glGenQueriesEXT
#define glGetQueryObjectuiv glGetQueryObjectuivEXT
#endif

namespace glbench {

class QueryGetTest : public TestBase {
 public:
  QueryGetTest() {}
  virtual ~QueryGetTest() {}

  virtual bool TestFunc(uint64_t iterations);
  virtual bool Run();
  virtual const char* Name() const { return "query_get"; }
  virtual bool IsDrawTest() const { return true; }
  virtual const char* Unit() const { return "us"; }

 private:
  std::vector<GLuint> queries_;
  GLenum target_;

  DISALLOW_COPY_AND_ASSIGN(QueryGetTest);
};

bool QueryGetTest::TestFunc(uint64_t iterations) {
  CHECK(!glGetError());

  for (uint64_t i = 0; i < iterations; i++) {
    const GLuint query = queries_[i % queries_.size()];

    if (i >= queries_.size()) {
      GLuint v;
      glGetQueryObjectuiv(query, GL_QUERY_RESULT, &v);
    }

    glBeginQuery(target_, query);
    glClear(GL_COLOR_BUFFER_BIT);
    glEndQuery(target_);
    glFlush();
  }

  for (size_t i = 0; i < queries_.size(); i++) {
    if (i >= iterations)
      break;

    GLuint v;
    glGetQueryObjectuiv(queries_[i], GL_QUERY_RESULT, &v);
  }

  CHECK(!glGetError());

  return true;
}

bool QueryGetTest::Run() {
  const char* exts = (const char*)glGetString(GL_EXTENSIONS);
  if (!strstr(exts, "GL_ARB_occlusion_query2") &&
      !strstr(exts, "GL_EXT_occlusion_query_boolean")) {
    printf("# Warning: QueryGetTest could not find query extension(s).\n");
    return false;
  }

  // We allow 5 outstanding queries.
  queries_.resize(5);
  glGenQueries(queries_.size(), queries_.data());

  target_ = GL_ANY_SAMPLES_PASSED;
  RunTest(this, "query_get_any_samples_passed", 1.0, g_width, g_height, false);

  glDeleteQueries(queries_.size(), queries_.data());

  return true;
}

TestBase* GetQueryGetTest() {
  return new QueryGetTest;
}

}  // namespace glbench
