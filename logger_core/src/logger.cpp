#include "logger/logger.hpp"

namespace sim_logger {

Logger::Logger(std::string name)
  : name_(std::move(name)) {}

const std::string& Logger::name() const noexcept {
  return name_;
}

void Logger::set_level(Level level) noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  level_ = level;
  level_overridden_ = true;
}

void Logger::clear_level_override() noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  level_overridden_ = false;
}

Level Logger::effective_level() const noexcept {
  // Copy shared state under lock, then do any recursion without holding the lock.
  bool overridden = false;
  Level local_level = Level::Info;
  std::shared_ptr<Logger> parent_shared;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    overridden = level_overridden_;
    local_level = level_;
    parent_shared = parent_.lock();
  }

  if (overridden || !parent_shared) {
    return local_level;
  }

  return parent_shared->effective_level();
}

void Logger::add_sink(std::shared_ptr<ISink> sink) {
  if (!sink) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  sinks_overridden_ = true;
  sinks_.push_back(std::move(sink));
}

void Logger::set_sinks(std::vector<std::shared_ptr<ISink>> sinks) {
  std::lock_guard<std::mutex> lock(mutex_);
  sinks_overridden_ = true;
  sinks_ = std::move(sinks);
}

void Logger::clear_sink_override() noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  sinks_overridden_ = false;
  sinks_.clear();
}

std::vector<std::shared_ptr<ISink>> Logger::effective_sinks() const {
  bool overridden = false;
  std::vector<std::shared_ptr<ISink>> local_sinks;
  std::shared_ptr<Logger> parent_shared;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    overridden = sinks_overridden_;
    local_sinks = sinks_;
    parent_shared = parent_.lock();
  }

  if (overridden || !parent_shared) {
    return local_sinks;
  }
  return parent_shared->effective_sinks();
}

void Logger::set_parent(std::shared_ptr<Logger> parent) noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  parent_ = parent;
}

std::shared_ptr<Logger> Logger::parent() const noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  return parent_.lock();
}

void Logger::log(const LogRecord& record) noexcept {
  const Level threshold = effective_level();
  if (!is_at_least(record.level(), threshold)) {
    return;
  }

  const auto sinks = effective_sinks();
  for (const auto& sink : sinks) {
    if (sink) {
      sink->write(record);
    }
  }
}

} // namespace sim_logger
