#include "logger/logger.hpp"

#include <exception>

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
  std::lock_guard<std::mutex> lock(mutex_);

  if (level_overridden_) {
    return level_;
  }

  auto parent = parent_.lock();
  if (parent) {
    return parent->effective_level();
  }

  return level_;
}

void Logger::add_sink(std::shared_ptr<ISink> sink) {
  std::lock_guard<std::mutex> lock(mutex_);
  sinks_.push_back(std::move(sink));
  sinks_overridden_ = true;
}

void Logger::set_sinks(std::vector<std::shared_ptr<ISink>> sinks) {
  std::lock_guard<std::mutex> lock(mutex_);
  sinks_ = std::move(sinks);
  sinks_overridden_ = true;
}

void Logger::clear_sink_override() noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  sinks_overridden_ = false;
  sinks_.clear();
}

std::vector<std::shared_ptr<ISink>> Logger::effective_sinks() const {
  std::lock_guard<std::mutex> lock(mutex_);

  if (sinks_overridden_) {
    return sinks_;
  }

  auto parent = parent_.lock();
  if (parent) {
    return parent->effective_sinks();
  }

  return {};
}

std::shared_ptr<Logger> Logger::parent() const noexcept {
  return parent_.lock();
}

void Logger::log(const LogRecord& record) noexcept {
  try {
    if (record.level() < effective_level()) {
      return;
    }

    const auto sinks = effective_sinks();
    const bool do_flush = effective_immediate_flush();

    for (const auto& sink : sinks) {
      try {
        sink->write(record);
        if (do_flush) {
          sink->flush();
        }
      } catch (...) {
        sink_failures_count_.fetch_add(1, std::memory_order_relaxed);
      }
    }
  } catch (...) {
    dropped_records_count_.fetch_add(1, std::memory_order_relaxed);
  }
}


void Logger::set_parent(std::shared_ptr<Logger> parent) noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  parent_ = parent;
}

std::uint64_t Logger::sink_failures_count() const noexcept {
  return sink_failures_count_.load(std::memory_order_relaxed);
}

std::uint64_t Logger::dropped_records_count() const noexcept {
  return dropped_records_count_.load(std::memory_order_relaxed);
}

void Logger::set_immediate_flush(bool enabled) noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  immediate_flush_ = enabled;
  immediate_flush_overridden_ = true;
}

void Logger::clear_immediate_flush_override() noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  immediate_flush_overridden_ = false;
}

bool Logger::effective_immediate_flush() const noexcept {
  std::lock_guard<std::mutex> lock(mutex_);

  if (immediate_flush_overridden_) {
    return immediate_flush_;
  }

  auto parent = parent_.lock();
  if (parent) {
    return parent->effective_immediate_flush();
  }

  return immediate_flush_;
}


}  // namespace sim_logger
