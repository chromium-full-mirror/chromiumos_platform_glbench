#ifndef BENCH_GL_ALL_TESTS_H_
#define BENCH_GL_ALL_TESTS_H_

namespace glbench {

class TestBase;

TestBase* GetAttributeFetchShaderTest();
TestBase* GetBufferStreamingTest();
TestBase* GetBufferUploadTest();
TestBase* GetBufferUploadSubTest();
TestBase* GetClearTest();
TestBase* GetContextTest();
TestBase* GetDrawSizeTest();
TestBase* GetFboFillRateTest();
TestBase* GetFillRateTest();
TestBase* GetQueryGetTest();
TestBase* GetReadPixelTest();
TestBase* GetSwapTest();
TestBase* GetTextureRebindTest();
TestBase* GetTextureReuseTest();
TestBase* GetTextureUpdateTest();
TestBase* GetTextureUploadTest();
TestBase* GetTriangleSetupTest();
TestBase* GetVaryingsAndDdxyShaderTest();
TestBase* GetWindowManagerCompositingTest(bool scissor);
TestBase* GetYuvToRgbTest();

}  // namespace glbench

#endif  // BENCH_GL_ALL_TESTS_H_
