#pragma once

#include "logger/level.hpp"
#include "logger/log_record.hpp"
#include "logger/sink.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace sim_logger {

class LoggerRegistry;

/**
 * @brief A hierarchical logger that emits LogRecord instances to one or more sinks.
 *
 * Loggers form a tree based on dot-separated names (e.g. "vehicle1.propulsion").
 * A logger may inherit its effective level and sinks from its parent unless
 * explicitly overridden.
 *
 * Thread-safety:
 * - All configuration access/mutations are internally synchronized.
 * - log() is safe to call concurrently from multiple threads.
 *
 * Failure behavior:
 * - Sink exceptions are swallowed; failures are counted and logging continues.
 * - If a record is filtered out by level, it is not emitted to sinks.
 */
class Logger final {
 public:
  /**
   * @brief Construct a logger with a stable name.
   * @param name Logger name (e.g. "root", "vehicle1.propulsion").
   */
  explicit Logger(std::string name);

  /**
   * @brief Returns the logger name.
   * @return Reference to the stored name string (stable for object lifetime).
   */
  const std::string& name() const noexcept;

  // --------------------------------------------------------------------------
  // Level override
  // --------------------------------------------------------------------------

  /**
   * @brief Override the logger's level explicitly.
   * @param level New level for this logger.
   *
   * Once set, effective_level() returns this level regardless of parent.
   */
  void set_level(Level level) noexcept;

  /**
   * @brief Clears the level override so the logger may inherit from its parent.
   *
   * After clearing, effective_level() returns:
   * - parent's effective level if a parent exists, otherwise the local default.
   */
  void clear_level_override() noexcept;

  /**
   * @brief Returns the level used for filtering records on this logger.
   * @return The effective (inherited or overridden) Level.
   */
  Level effective_level() const noexcept;

  // --------------------------------------------------------------------------
  // Sink override
  // --------------------------------------------------------------------------

  /**
   * @brief Append a sink to this logger's sink override list.
   * @param sink Sink to append (must be non-null).
   *
   * This enables sink override mode for this logger.
   */
  void add_sink(std::shared_ptr<ISink> sink);

  /**
   * @brief Replace this logger's sink override list.
   * @param sinks New sink list (each must be non-null).
   *
   * This enables sink override mode for this logger.
   */
  void set_sinks(std::vector<std::shared_ptr<ISink>> sinks);

  /**
   * @brief Clear sink override and revert to inheriting sinks from parent.
   *
   * After clearing, effective_sinks() returns parent's sinks if a parent exists,
   * otherwise returns an empty vector.
   */
  void clear_sink_override() noexcept;

  /**
   * @brief Returns the sinks that will be used when logging.
   * @return Effective sinks (inherited or overridden).
   */
  std::vector<std::shared_ptr<ISink>> effective_sinks() const;

  /**
   * @brief Returns the parent logger if present.
   * @return Shared pointer to parent logger, or nullptr if none.
   */
  std::shared_ptr<Logger> parent() const noexcept;

  // --------------------------------------------------------------------------
  // Logging and stats
  // --------------------------------------------------------------------------

  /**
   * @brief Emit a log record to the effective sinks if it passes level filtering.
   * @param record Record to emit.
   *
   * Exceptions thrown by sinks are swallowed and counted in sink_failures_count().
   */
  void log(const LogRecord& record) noexcept;

  /**
   * @brief Returns number of records dropped (typically due to filtering).
   * @return Count of dropped records.
   */
  std::uint64_t dropped_records_count() const noexcept;

  /**
   * @brief Returns number of sink write failures encountered.
   * @return Count of sink failures.
   */
  std::uint64_t sink_failures_count() const noexcept;

 private:
  /// LoggerRegistry needs internal access to set hierarchical parent relationships.
  friend class LoggerRegistry;

  /**
   * @brief Set the parent logger (used by LoggerRegistry).
   * @param parent Parent logger (may be nullptr).
   */
  void set_parent(std::shared_ptr<Logger> parent) noexcept;

  /// Logger name.
  std::string name_;

  /// Protects configuration and parent pointer.
  mutable std::mutex mutex_;

  /// Explicit level if overridden; otherwise default local level.
  Level level_{Level::Info};

  /// True if this logger's level is explicitly overridden.
  bool level_overridden_{false};

  /// Sink list used when sinks are overridden.
  std::vector<std::shared_ptr<ISink>> sinks_;

  /// True if this logger's sinks are explicitly overridden.
  bool sinks_overridden_{false};

  /// Weak parent pointer to avoid ownership cycles in the registry.
  std::weak_ptr<Logger> parent_;

  /// Number of records dropped (e.g., filtered).
  std::atomic<std::uint64_t> dropped_records_count_{0};

  /// Number of sink failures (exceptions) swallowed during log().
  std::atomic<std::uint64_t> sink_failures_count_{0};
};

}  // namespace sim_logger
