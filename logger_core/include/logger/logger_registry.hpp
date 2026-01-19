#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace sim_logger {

class Logger;

/**
 * @brief Global registry for named loggers.
 *
 * The registry creates and caches loggers by name. It also establishes the
 * parent-child hierarchy based on dot-separated names:
 * - "root" has no parent.
 * - "a.b.c" has parent "a.b".
 *
 * Thread-safety:
 * - get_logger() is safe to call concurrently.
 * - Returned shared_ptr instances are stable (cached).
 */
class LoggerRegistry final {
 public:
  /**
   * @brief Returns the singleton registry instance.
   */
  static LoggerRegistry& instance();

  /**
   * @brief Get (or create) a logger with the specified name.
   * @param name Logger name. "root" is treated as the root logger.
   * @return Shared pointer to the logger.
   *
   * If the logger does not exist, it is created and inserted into the registry.
   * Parent loggers are created as needed.
   */
  std::shared_ptr<Logger> get_logger(const std::string& name);

  /**
   * @brief Remove all loggers from the registry.
   *
   * Useful for tests to ensure clean state between cases.
   */
  void clear();

 private:
  LoggerRegistry() = default;

  /**
   * @brief Compute the parent name for a dot-separated logger name.
   * @param name Child logger name.
   * @return Parent name, or empty string if none.
   */
  static std::string parent_name_for(const std::string& name);

  /// Protects the map of cached loggers.
  std::mutex mutex_;

  /// Cached loggers keyed by name.
  std::unordered_map<std::string, std::shared_ptr<Logger>> loggers_;
};

}  // namespace sim_logger
