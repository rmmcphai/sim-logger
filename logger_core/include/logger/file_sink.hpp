#pragma once

#include "logger/pattern_formatter.hpp"
#include "logger/sink.hpp"

#include <cstdio>
#include <mutex>
#include <string>

namespace sim_logger {

/**
 * @brief Synchronous file sink (append-only). No rotation (Sprint 3).
 *
 * Thread-safety:
 * - Serializes writes and flushes using an internal mutex.
 *
 * Error behavior:
 * - Throws std::runtime_error on open/write/flush failures.
 * - Logger is responsible for exception containment.
 */
class FileSink final : public ISink {
 public:
  /**
   * @param path Output file path (opened in append mode).
   * @param formatter Formatter used to render records.
   * @param durable_flush If true, flush() will attempt an fsync() on POSIX.
   */
  FileSink(std::string path, PatternFormatter formatter, bool durable_flush = false);

  ~FileSink() override;

  void write(const LogRecord& record) override;
  void flush() override;

  const std::string& path() const noexcept { return path_; }
  bool durable_flush() const noexcept { return durable_flush_; }

 private:
  std::string path_;
  PatternFormatter formatter_;
  bool durable_flush_{false};

  std::FILE* file_{nullptr};
  std::mutex mu_;

  void open_or_throw();
  void close_noexcept() noexcept;
};

}  // namespace sim_logger
