#include "logger/logger_registry.hpp"

namespace sim_logger {

LoggerRegistry& LoggerRegistry::instance() noexcept {
  static LoggerRegistry registry;
  return registry;
}

std::shared_ptr<Logger> LoggerRegistry::get_logger(const std::string& name) {
  if (name.empty()) {
    return nullptr;
  }

  std::lock_guard<std::mutex> lock(mutex_);

  // Create each prefix of the hierarchical name and link parent pointers.
  // Example: "a.b.c" -> create/link "a" -> "a.b" -> "a.b.c".
  std::shared_ptr<Logger> prev;
  std::size_t start = 0;
  while (start < name.size()) {
    const auto dot = name.find('.', start);
    const std::size_t end = (dot == std::string::npos) ? name.size() : dot;
    const std::string segment = name.substr(0, end);

    auto current = get_or_create_locked_(segment);
    if (prev) {
      current->set_parent(prev);
    }
    prev = current;

    if (dot == std::string::npos) {
      break;
    }
    start = dot + 1;
  }

  return prev;
}

void LoggerRegistry::clear() noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  loggers_.clear();
}

std::shared_ptr<Logger> LoggerRegistry::get_or_create_locked_(const std::string& name) {
  auto it = loggers_.find(name);
  if (it != loggers_.end()) {
    return it->second;
  }

  auto created = std::make_shared<Logger>(name);
  loggers_.emplace(name, created);
  return created;
}

std::string LoggerRegistry::parent_name_(const std::string& name) {
  const auto pos = name.rfind('.');
  if (pos == std::string::npos) {
    return {};
  }
  return name.substr(0, pos);
}

} // namespace sim_logger
