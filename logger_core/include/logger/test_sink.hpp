#pragma once

#include "logger/sink.hpp"

#include <cstddef>
#include <mutex>
#include <vector>

namespace sim_logger {

/**
 * @file test_sink.hpp
 * @brief A sink implementation intended for unit tests.
 *
 * @details
 * TestSink captures LogRecord objects in memory so unit tests can assert
 * on what was emitted by the logging pipeline.
 *
 * Requirements addressed:
 *  - Thread safety: multiple threads may write concurrently.
 *  - Deterministic assertions: tests can snapshot and count captured records.
 */
class TestSink final : public ISink {
 public:
  /**
   * @brief Store a copy of the record for later inspection.
   */
  void write(const LogRecord& record) noexcept override;

  /**
   * @brief No-op for this sink.
   */
  void flush() noexcept override;

  /**
   * @brief Number of records captured so far.
   */
  std::size_t size() const noexcept;

  /**
   * @brief Copy all captured records for stable assertions.
   */
  std::vector<LogRecord> snapshot() const;

  /**
   * @brief Remove all captured records.
   */
  void clear() noexcept;

 private:
  mutable std::mutex mutex_;
  std::vector<LogRecord> records_;
};

} // namespace sim_logger
