#include <gtest/gtest.h>

TEST(SampleTest, HelloWorld) {
  auto fn = []() { return "Hello, World!"; };
  ASSERT_EQ(fn(), "Hello, World!");
}