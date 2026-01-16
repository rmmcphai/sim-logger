#pragma once

#include "logger/log_record.hpp"

namespace sim_logger {

/**
 * @file sink.hpp
 * @brief Defines the sink interface used by the logging pipeline.
 *
 * @details
 * A sink is a destination for fully materialized log records.
 * Examples (later sprints): console sink, file sink, Trick adapter sink.
 *
 * Design constraints (high importance for simulation use):
 *  - Thread-safe: multiple threads may write to the same sink concurrently.
 *  - noexcept: sinks must not throw exceptions back into simulation code.
 *
 * The sink interface is intentionally minimal in Sprint 2.
 */

/**
 * @brief Abstract sink interface.
 */
class ISink {
 public:
  virtual ~ISink() = default;

  /**
   * @brief Consume a fully materialized log record.
   *
   * @note
   * Implementations must be thread-safe and must not throw.
   */
  virtual void write(const LogRecord& record) noexcept = 0;

  /**
   * @brief Flush any buffered output.
   *
   * @note
   * Implementations must be thread-safe and must not throw.
   */
  virtual void flush() noexcept = 0;
};

} // namespace sim_logger
