#include "minitest.hpp"

int main() {
  int failed = 0;
  for (const auto& tc : sim_logger_tests::registry()) {
    try {
      tc.fn();
      std::cout << "[PASS] " << tc.name << "\n";
    } catch (const sim_logger_tests::TestFailure& e) {
      ++failed;
      std::cout << "[FAIL] " << tc.name << "\n";
      std::cout << "       " << e.what() << "\n";
    } catch (const std::exception& e) {
      ++failed;
      std::cout << "[FAIL] " << tc.name << "\n";
      std::cout << "       unexpected exception: " << e.what() << "\n";
    } catch (...) {
      ++failed;
      std::cout << "[FAIL] " << tc.name << "\n";
      std::cout << "       unknown exception\n";
    }
  }

  if (failed != 0) {
    std::cout << failed << " test(s) failed\n";
    return 1;
  }
  return 0;
}
