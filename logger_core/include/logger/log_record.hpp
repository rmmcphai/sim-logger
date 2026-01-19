#pragma once

#include "logger/level.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace sim_logger {

/**
 * @brief Key/value tag associated with a log record.
 */
struct Tag {
  std::string key;
  std::string value;
};

/**
 * @file log_record.hpp
 * @brief Immutable, fully materialized log record passed to sinks/formatters.
 *
 * @details
 * Canonical test-suite contract includes:
 * - sim_time (seconds), mission_elapsed/MET (seconds)
 * - wall_time_ns
 * - thread_id
 * - source location (file/line/function)
 * - logger_name
 * - tags
 * - message
 */
class LogRecord {
 public:
  LogRecord(Level level,
            double sim_time,
            double met,
            int64_t wall_time_ns,
            std::thread::id thread_id,
            std::string file,
            uint32_t line,
            std::string function,
            std::string logger_name,
            std::vector<Tag> tags,
            std::string message)
      : level_(level),
        sim_time_(sim_time),
        met_(met),
        wall_time_ns_(wall_time_ns),
        thread_id_(thread_id),
        file_(std::move(file)),
        line_(line),
        function_(std::move(function)),
        logger_name_(std::move(logger_name)),
        tags_(std::move(tags)),
        message_(std::move(message)) {}

  Level level() const noexcept { return level_; }

  double sim_time() const noexcept { return sim_time_; }

  double mission_elapsed() const noexcept { return met_; }

  int64_t wall_time_ns() const noexcept { return wall_time_ns_; }

  std::thread::id thread_id() const noexcept { return thread_id_; }

  std::string_view file() const noexcept { return file_; }

  uint32_t line() const noexcept { return line_; }

  std::string_view function() const noexcept { return function_; }

  std::string_view logger_name() const noexcept { return logger_name_; }

  const std::vector<Tag>& tags() const noexcept { return tags_; }

  std::string_view message() const noexcept { return message_; }

 private:
  Level level_;
  double sim_time_;
  double met_;
  int64_t wall_time_ns_;
  std::thread::id thread_id_;

  std::string file_;
  uint32_t line_;
  std::string function_;
  std::string logger_name_;
  std::vector<Tag> tags_;
  std::string message_;
};

}  // namespace sim_logger
