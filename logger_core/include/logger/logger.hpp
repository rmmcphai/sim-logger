#pragma once

#include "logger/level.hpp"
#include "logger/log_record.hpp"
#include "logger/sink.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace sim_logger {

/**
 * @file logger.hpp
 * @brief Defines the Logger type responsible for routing LogRecord objects to sinks.
 *
 * @details
 * In Sprint 2, a Logger provides:
 *  - a name (hierarchical string such as "vehicle1.propulsion"),
 *  - a severity threshold (Level),
 *  - a list of sinks, and
 *  - optional inheritance from a parent logger.
 *
 * The logger routes pre-built LogRecord objects to sinks. Construction of log
 * records (macros capturing file/line/function, time sources, formatting, etc.)
 * is introduced in later sprints.
 */
class Logger : public std::enable_shared_from_this<Logger> {
 public:
  /**
   * @brief Create a logger with the provided name.
   *
   * @note
   * The default effective level is Level::Info unless inherited from a parent.
   */
  explicit Logger(std::string name);

  /**
   * @brief Get the logger name.
   */
  const std::string& name() const noexcept;

  /**
   * @brief Set the logger's level threshold and mark it as an override.
   *
   * @note
   * Overrides affect only this logger. They do not modify the parent.
   */
  void set_level(Level level) noexcept;

  /**
   * @brief Clear the logger's level override, restoring inheritance.
   */
  void clear_level_override() noexcept;

  /**
   * @brief Return the logger's effective threshold.
   */
  Level effective_level() const noexcept;

  /**
   * @brief Add a sink and mark sinks as overridden for this logger.
   *
   * @details
   * This method transitions the logger into "sinks overridden" mode.
   * In that state, the logger will no longer inherit sinks from its parent.
   */
  void add_sink(std::shared_ptr<ISink> sink);

  /**
   * @brief Replace sinks and mark sinks as overridden for this logger.
   */
  void set_sinks(std::vector<std::shared_ptr<ISink>> sinks);

  /**
   * @brief Clear the sink override, restoring inheritance.
   */
  void clear_sink_override() noexcept;

  /**
   * @brief Get the effective sinks (inherited or overridden).
   */
  std::vector<std::shared_ptr<ISink>> effective_sinks() const;

  /**
   * @brief Assign a parent logger.
   *
   * @note
   * The registry sets parents at creation time.
   */
  void set_parent(std::shared_ptr<Logger> parent) noexcept;

  /**
   * @brief Get the parent logger if present.
   */
  std::shared_ptr<Logger> parent() const noexcept;

  /**
   * @brief Route a fully materialized log record to sinks.
   *
   * @details
   * Filtering in Sprint 2:
   *  - If record.level() is below effective_level(), the record is suppressed.
   */
  void log(const LogRecord& record) noexcept;

 private:
  std::string name_;

  // NOTE: Using a single mutex keeps implementation straightforward.
  // Later sprints may introduce finer-grained locking if needed.
  mutable std::mutex mutex_;

  bool level_overridden_{false};
  Level level_{Level::Info};

  bool sinks_overridden_{false};
  std::vector<std::shared_ptr<ISink>> sinks_;

  std::weak_ptr<Logger> parent_;
};

} // namespace sim_logger
