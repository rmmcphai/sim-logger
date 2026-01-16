#include "logger/test_sink.hpp"

namespace sim_logger {

void TestSink::write(const LogRecord& record) noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  records_.push_back(record);
}

void TestSink::flush() noexcept {
  // No buffered output.
}

std::size_t TestSink::size() const noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  return records_.size();
}

std::vector<LogRecord> TestSink::snapshot() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return records_;
}

void TestSink::clear() noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  records_.clear();
}

} // namespace sim_logger
