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
 *  - Exception containment: sink implementations may throw, but the logger must
 *    catch and never allow exceptions to propagate into simulation code.
 *
 * In particular, Logger::log(...) is required to be noexcept and must swallow
 * all exceptions arising from sinks and formatting.
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
   * Implementations must be thread-safe.
   *
   * Implementations may throw; any exception must be caught by Logger::log(...)
   * (exceptions must not propagate into simulation code).
   */
  virtual void write(const LogRecord& record) = 0;

  /**
   * @brief Flush any buffered output.
   *
   * @note
   * Implementations must be thread-safe.
   *
   * Implementations may throw; any exception must be caught by Logger::log(...)
   * (exceptions must not propagate into simulation code).
   */
  virtual void flush() = 0;
};

} // namespace sim_logger
