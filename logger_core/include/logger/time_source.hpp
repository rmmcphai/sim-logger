#pragma once

#include <cstdint>

namespace sim_logger {

/**
 * @file time_source.hpp
 * @brief Defines the interface for supplying time information to log records.
 *
 * @details
 * Canonical API (as locked by the current unit tests):
 * - sim_time() returns simulation time in seconds (double).
 * - mission_elapsed() returns MET in seconds (double).
 * - wall_time_ns() returns a monotonic timestamp in nanoseconds (int64_t).
 *
 * Locked design intent:
 * - The logging system uses a single global ITimeSource instance as the
 *   authoritative source of time (see global_time.hpp).
 * - Record construction reads time and stores it into LogRecord.
 * - Logger and sinks treat time values as immutable record data.
 */
class ITimeSource {
 public:
  virtual ~ITimeSource() = default;

  /**
   * @brief Return simulation time in seconds.
   */
  virtual double sim_time() noexcept = 0;

  /**
   * @brief Return mission elapsed time (MET) in seconds.
   */
  virtual double mission_elapsed() noexcept = 0;

  /**
   * @brief Return a monotonic timestamp in nanoseconds.
   */
  virtual int64_t wall_time_ns() noexcept = 0;
};

}  // namespace sim_logger
