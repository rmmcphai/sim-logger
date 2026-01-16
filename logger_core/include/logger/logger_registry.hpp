#pragma once

#include "logger/logger.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace sim_logger {

/**
 * @file logger_registry.hpp
 * @brief Provides a central registry for hierarchical loggers.
 *
 * @details
 * The registry ensures that:
 *  - Each hierarchical name refers to a single Logger instance.
 *  - Parent loggers exist and are linked automatically.
 *  - Children can inherit configuration from parents.
 *
 * Example:
 *  - get_logger("vehicle1.propulsion") creates (if needed) both
 *    "vehicle1" and "vehicle1.propulsion" and links the child to the parent.
 */
class LoggerRegistry {
 public:
  /**
   * @brief Access the process-wide registry instance.
   */
  static LoggerRegistry& instance() noexcept;

  /**
   * @brief Get or create a logger for the given hierarchical name.
   */
  std::shared_ptr<Logger> get_logger(const std::string& name);

  /**
   * @brief Remove all registered loggers (primarily for unit tests).
   */
  void clear() noexcept;

 private:
  LoggerRegistry() = default;

  std::shared_ptr<Logger> get_or_create_locked_(const std::string& name);
  static std::string parent_name_(const std::string& name);

  std::mutex mutex_;
  std::unordered_map<std::string, std::shared_ptr<Logger>> loggers_;
};

} // namespace sim_logger
