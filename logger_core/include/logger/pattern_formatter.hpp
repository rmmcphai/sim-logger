#pragma once

#include "logger/log_record.hpp"
#include "logger/level.hpp"

#include <string>
#include <string_view>
#include <unordered_set>

namespace sim_logger {

/**
 * @file pattern_formatter.hpp
 * @brief Pattern-based formatting for LogRecord instances.
 *
 * @details
 * This component is the single owner of pattern token rules (e.g., "{met}").
 *
 * Locked design intent:
 * - Formatting is independent of Logger routing/filtering and independent of sinks.
 * - A LogRecord is already fully materialized (time, source location, logger name,
 *   thread id, tags, message).
 * - PatternFormatter renders LogRecord -> std::string for text-oriented sinks
 *   or diagnostics.
 *
 * Token grammar (v1):
 * - Tokens are delimited by '{' and '}' (e.g., "{met}").
 * - Token characters: [A-Za-z0-9_]
 * - Unknown tokens are left unchanged (including braces) for forward compatibility.
 *
 * Locked requirement:
 * - "{met}" MUST be supported and rendered (Mission Elapsed Time, seconds).
 */
class PatternFormatter {
 public:
  /**
   * @brief Construct a formatter with a pattern string.
   *
   * @details
   * If require_met_token is true, the constructor validates that "{met}" exists
   * in the provided pattern. This supports the requirement that default patterns
   * include MET and makes misconfiguration explicit at construction time.
   *
   * @param pattern Pattern string (e.g., "{met} {level} {logger}: {msg}").
   * @param require_met_token If true, throws if "{met}" is missing.
   *
   * @throws std::invalid_argument if require_met_token is true and "{met}" is missing.
   */
  explicit PatternFormatter(std::string pattern, bool require_met_token = false);

  /**
   * @brief Format a LogRecord according to the configured pattern.
   *
   * @details
   * Supported tokens (v1, canonical with current LogRecord):
   * - {level}    -> to_string(record.level())
   * - {sim}      -> record.sim_time()            (seconds, double)
   * - {met}      -> record.mission_elapsed()     (seconds, double)
   * - {wall_ns} -> record.wall_time_ns()         (uint64)
   * - {thread}  -> record.thread_id()            (implementation-defined string)
   * - {file}    -> record.file()
   * - {line}    -> record.line()
   * - {function}-> record.function()
   * - {logger}  -> record.logger_name()
   * - {msg}     -> record.message()
   *
   * Unknown tokens are preserved verbatim (including braces).
   *
   * @param record Fully materialized log record.
   * @return Formatted string.
   *
   * @note
   * This function may throw (e.g., std::bad_alloc). Exceptions must be contained
   * by the caller per the project’s exception-containment rules.
   */
  std::string format(const LogRecord& record) const;

  /**
   * @brief Return the raw pattern string.
   */
  const std::string& pattern() const noexcept { return pattern_; }

  /**
   * @brief Return the set of detected tokens (without braces).
   */
  const std::unordered_set<std::string>& tokens() const noexcept { return tokens_; }

 private:
  std::string pattern_;
  std::unordered_set<std::string> tokens_;

  static std::unordered_set<std::string> extract_tokens(std::string_view pattern);
};

}  // namespace sim_logger
