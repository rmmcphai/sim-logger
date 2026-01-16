#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace sim_logger_tests {

struct TestCase {
  std::string name;
  std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
  static std::vector<TestCase> r;
  return r;
}

inline void add_test(std::string name, std::function<void()> fn) {
  registry().push_back(TestCase{std::move(name), std::move(fn)});
}

class TestFailure final : public std::exception {
 public:
  explicit TestFailure(std::string msg) : msg_(std::move(msg)) {}
  const char* what() const noexcept override { return msg_.c_str(); }

 private:
  std::string msg_;
};

} // namespace sim_logger_tests

#define SIM_LOGGER_TEST_CONCAT_IMPL(a, b) a##b
#define SIM_LOGGER_TEST_CONCAT(a, b) SIM_LOGGER_TEST_CONCAT_IMPL(a, b)

#define TEST_CASE(NAME, TAGS)                                                     \
  static void SIM_LOGGER_TEST_CONCAT(test_fn_, __LINE__)();                       \
  namespace {                                                                     \
  const bool SIM_LOGGER_TEST_CONCAT(test_reg_, __LINE__) = []() {                 \
    ::sim_logger_tests::add_test((NAME), &SIM_LOGGER_TEST_CONCAT(test_fn_, __LINE__)); \
    return true;                                                                  \
  }();                                                                            \
  }                                                                              \
  static void SIM_LOGGER_TEST_CONCAT(test_fn_, __LINE__)()

#define REQUIRE(COND)                                                             \
  do {                                                                            \
    if (!(COND)) {                                                                \
      std::ostringstream oss;                                                     \
      oss << "REQUIRE failed: " #COND " at " << __FILE__ << ":" << __LINE__; \
      throw ::sim_logger_tests::TestFailure(oss.str());                           \
    }                                                                             \
  } while (0)

#define REQUIRE_FALSE(COND) REQUIRE(!(COND))
