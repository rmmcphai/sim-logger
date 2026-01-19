#include "logger/logger_registry.hpp"

#include "logger/logger.hpp"

#include <stdexcept>

namespace sim_logger {

LoggerRegistry& LoggerRegistry::instance() {
  static LoggerRegistry inst;
  return inst;
}

std::shared_ptr<Logger> LoggerRegistry::get_logger(const std::string& name) {
  // Fast path: lookup under lock
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loggers_.find(name);
    if (it != loggers_.end()) return it->second;
  }

  // Compute and ensure parent exists WITHOUT holding mutex_
  std::shared_ptr<Logger> parent;
  const auto parent_name = parent_name_for(name);
  if (!parent_name.empty()) {
    parent = get_logger(parent_name);
  }

  // Create candidate
  auto created = std::make_shared<Logger>(name);
  created->set_parent(parent);

  // Insert (double-check)
  std::lock_guard<std::mutex> lock(mutex_);
  auto [it, inserted] = loggers_.emplace(name, created);
  return it->second;
}


void LoggerRegistry::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  loggers_.clear();
}

std::string LoggerRegistry::parent_name_for(const std::string& name) {
  if (name.empty() || name == "root") return {};

  const auto pos = name.find_last_of('.');
  if (pos == std::string::npos) return "root";
  if (pos == 0) return "root";          // ".x" -> root (defensive)
  if (pos == name.size() - 1) {         // "a." -> treat as "a"
    return parent_name_for(name.substr(0, name.size() - 1));
  }
  return name.substr(0, pos);
}


}  // namespace sim_logger
