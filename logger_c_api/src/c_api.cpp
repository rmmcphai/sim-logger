#include "sim_logger/c_api.h"

#include "logger/global_time.hpp"
#include "logger/level.hpp"
#include "logger/log_record.hpp"
#include "logger/logger.hpp"
#include "logger/logger_registry.hpp"

#include <cstdarg>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using sim_logger::ITimeSource;
using sim_logger::Level;
using sim_logger::LogRecord;
using sim_logger::Logger;
using sim_logger::LoggerRegistry;

struct sim_logger_logger {
  std::shared_ptr<Logger> impl;
};

static Level to_cpp_level(sim_logger_level_t lvl) noexcept {
  switch (lvl) {
    case SIM_LOGGER_LEVEL_DEBUG: return Level::Debug;
    case SIM_LOGGER_LEVEL_INFO:  return Level::Info;
    case SIM_LOGGER_LEVEL_WARN:  return Level::Warn;
    case SIM_LOGGER_LEVEL_ERROR: return Level::Error;
    case SIM_LOGGER_LEVEL_FATAL: return Level::Fatal;
    default:                     return Level::Info;
  }
}

static std::string vformat_printf(const char* fmt, va_list ap) {
  if (fmt == nullptr) {
    return {};
  }

  va_list ap_copy;
  va_copy(ap_copy, ap);
  const int needed = std::vsnprintf(nullptr, 0, fmt, ap_copy);
  va_end(ap_copy);

  if (needed <= 0) {
    return {};
  }

  std::string out;
  out.resize(static_cast<size_t>(needed));
  std::vsnprintf(out.data(), out.size() + 1U, fmt, ap);
  return out;
}

extern "C" {

sim_logger_logger_t* sim_logger_get(const char* name) {
  const char* safe_name = (name != nullptr) ? name : "";
  auto logger = LoggerRegistry::instance().get_logger(safe_name);

  auto* h = new sim_logger_logger();
  h->impl = std::move(logger);
  return h;
}

void sim_logger_release(sim_logger_logger_t* logger) {
  delete logger;
}

void sim_logger_log(sim_logger_logger_t* logger,
                    sim_logger_level_t level,
                    const char* file,
                    uint32_t line,
                    const char* func,
                    const char* msg) {
  if (logger == nullptr || !logger->impl) {
    return;
  }

  ITimeSource& ts = sim_logger::global_time_source_ref();

  LogRecord record(to_cpp_level(level),
                   ts.sim_time(),
                   ts.mission_elapsed(),
                   ts.wall_time_ns(),
                   std::this_thread::get_id(),
                   (file != nullptr) ? file : "",
                   line,
                   (func != nullptr) ? func : "",
                   logger->impl->name(),
                   std::vector<sim_logger::Tag>{},
                   (msg != nullptr) ? std::string(msg) : std::string{});

  logger->impl->log(record);
}

void sim_logger_vlogf(sim_logger_logger_t* logger,
                      sim_logger_level_t level,
                      const char* file,
                      uint32_t line,
                      const char* func,
                      const char* fmt,
                      va_list ap) {
  const std::string formatted = vformat_printf(fmt, ap);
  sim_logger_log(logger, level, file, line, func, formatted.c_str());
}

void sim_logger_logf(sim_logger_logger_t* logger,
                     sim_logger_level_t level,
                     const char* file,
                     uint32_t line,
                     const char* func,
                     const char* fmt,
                     ...) {
  va_list ap;
  va_start(ap, fmt);
  sim_logger_vlogf(logger, level, file, line, func, fmt, ap);
  va_end(ap);
}

void sim_logger_flush(sim_logger_logger_t* logger) {
  if (logger == nullptr || !logger->impl) {
    return;
  }

  // Best-effort flush: use effective sinks and call flush() on each.
  // Any sink exceptions are contained by the logger's policies elsewhere,
  // but flush() here is outside Logger::log, so contain locally.
  try {
    const auto sinks = logger->impl->effective_sinks();
    for (const auto& s : sinks) {
      try {
        s->flush();
      } catch (...) {
        // swallow
      }
    }
  } catch (...) {
    // swallow
  }
}

}  // extern "C"
