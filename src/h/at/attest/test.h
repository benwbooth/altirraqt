// Stub - ATTest framework is not built in the Qt port.
// AT_TESTS_ENABLED is never defined, so test bodies are compiled out.
#pragma once
#define AT_DEFINE_TEST(name) static int name##_notest()
#define AT_TEST_ASSERT(x) ((void)0)
#define AT_TEST_ASSERTF(x, ...) ((void)0)
