#pragma once

#include "logger/time_source.hpp"

#include <memory>

namespace sim_logger {

/**
 * @file global_time.hpp
 * @brief Provides the single global time source used by the logging system.
 *
 * @details
 * Locked design intent:
 * - The logging system uses one global ITimeSource instance as the authoritative
 *   source of wall time, sim time, and MET.
 *
 * Initialization:
 * - If no time source is explicitly installed, a process-local fallback
 *   DummyTimeSource is used (defaults are stable and deterministic).
 *
 * Thread-safety:
 * - All accessors are thread-safe.
 * - Installing/changing the global source is expected to be rare (startup).
 */

/**
 * @brief Install the global time source used by the logging system.
 *
 * @details
 * Passing nullptr resets the global source back to the fallback DummyTimeSource.
 */
void set_global_time_source(std::shared_ptr<ITimeSource> source);

/**
 * @brief Get the current global time source (never null).
 */
std::shared_ptr<ITimeSource> global_time_source();

/**
 * @brief Get the current global time source as a reference.
 *
 * @note
 * This uses a thread-local snapshot to keep the underlying shared_ptr alive
 * for the caller thread.
 */
ITimeSource& global_time_source_ref();

}  // namespace sim_logger
