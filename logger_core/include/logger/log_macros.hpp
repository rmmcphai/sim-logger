// logger_core/include/logger/log_macros.hpp
#pragma once

#include "logger/global_time.hpp"
#include "logger/log_record.hpp"
#include "logger/logger.hpp"

#include <cstdarg>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace sim_logger::detail {

// Accept Logger& or std::shared_ptr<Logger>.
inline Logger& as_logger(Logger& logger) noexcept { return logger; }

inline Logger& as_logger(const std::shared_ptr<Logger>& logger) noexcept {
  // Hard failure on null is intentional; macro call sites stay simple.
  return *logger;
}

inline std::string vformat_printf(const char* fmt, std::va_list ap) {
  if (fmt == nullptr) {
    return {};
  }

  std::va_list ap_copy;
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

template <typename LoggerLike>
inline void log_string(LoggerLike&& logger_like,
                       Level level,
                       const char* file,
                       unsigned line,
                       const char* function,
                       std::string message) {
  Logger& logger = as_logger(std::forward<LoggerLike>(logger_like));
  ITimeSource& ts = global_time_source_ref();

  LogRecord record(level,
                   ts.sim_time(),
                   ts.mission_elapsed(),
                   ts.wall_time_ns(),
                   std::this_thread::get_id(),
                   file ? file : "",
                   line,
                   function ? function : "",
                   logger.name(),
                   std::vector<Tag>{},
                   std::move(message));

  logger.log(record);
}

template <typename LoggerLike>
inline void log_printf(LoggerLike&& logger_like,
                       Level level,
                       const char* file,
                       unsigned line,
                       const char* function,
                       const char* fmt,
                       ...) {
  std::va_list ap;
  va_start(ap, fmt);
  std::string msg = vformat_printf(fmt, ap);
  va_end(ap);

  log_string(std::forward<LoggerLike>(logger_like),
             level,
             file,
             line,
             function,
             std::move(msg));
}

}  // namespace sim_logger::detail

// -----------------------------------------------------------------------------
// Public macros (message-only)
// -----------------------------------------------------------------------------
#define LOG_DEBUG(logger, msg)                                                     \
  ::sim_logger::detail::log_string((logger), ::sim_logger::Level::Debug, __FILE__, \
                                  static_cast<unsigned>(__LINE__), __func__, (msg))

#define LOG_INFO(logger, msg)                                                      \
  ::sim_logger::detail::log_string((logger), ::sim_logger::Level::Info, __FILE__,  \
                                  static_cast<unsigned>(__LINE__), __func__, (msg))

#define LOG_WARN(logger, msg)                                                      \
  ::sim_logger::detail::log_string((logger), ::sim_logger::Level::Warn, __FILE__,  \
                                  static_cast<unsigned>(__LINE__), __func__, (msg))

#define LOG_ERROR(logger, msg)                                                     \
  ::sim_logger::detail::log_string((logger), ::sim_logger::Level::Error, __FILE__, \
                                  static_cast<unsigned>(__LINE__), __func__, (msg))

#define LOG_FATAL(logger, msg)                                                     \
  ::sim_logger::detail::log_string((logger), ::sim_logger::Level::Fatal, __FILE__, \
                                  static_cast<unsigned>(__LINE__), __func__, (msg))

// -----------------------------------------------------------------------------
// C++17-safe printf-style formatting macros that allow zero varargs:
//   LOG_INFOF(logger, "no args");
//   LOG_INFOF(logger, "x=%d", 7);
// -----------------------------------------------------------------------------

// ---- Preprocessor utilities ----
#define SIM_LOGGER_PP_CAT_(a, b) a##b
#define SIM_LOGGER_PP_CAT(a, b) SIM_LOGGER_PP_CAT_(a, b)

// Count args up to 10.
#define SIM_LOGGER_PP_NARGS_(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define SIM_LOGGER_PP_NARGS(...) \
  SIM_LOGGER_PP_NARGS_(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

// Dispatch: NAME is a macro prefix, expands to NAME_IMPL<N>(...)
#define SIM_LOGGER_PP_DISPATCH_BY_NARGS(name, ...) \
  SIM_LOGGER_PP_CAT(name##_IMPL, SIM_LOGGER_PP_NARGS(__VA_ARGS__))(__VA_ARGS__)

// ---- Implementations: 2-arg and 3+-arg ----
// (logger, fmt)
#define SIM_LOGGER_DEBUGF_2(logger, fmt)                                            \
  ::sim_logger::detail::log_printf((logger), ::sim_logger::Level::Debug, __FILE__,  \
                                  static_cast<unsigned>(__LINE__), __func__, (fmt))
#define SIM_LOGGER_INFOF_2(logger, fmt)                                             \
  ::sim_logger::detail::log_printf((logger), ::sim_logger::Level::Info, __FILE__,   \
                                  static_cast<unsigned>(__LINE__), __func__, (fmt))
#define SIM_LOGGER_WARNF_2(logger, fmt)                                             \
  ::sim_logger::detail::log_printf((logger), ::sim_logger::Level::Warn, __FILE__,   \
                                  static_cast<unsigned>(__LINE__), __func__, (fmt))
#define SIM_LOGGER_ERRORF_2(logger, fmt)                                            \
  ::sim_logger::detail::log_printf((logger), ::sim_logger::Level::Error, __FILE__,  \
                                  static_cast<unsigned>(__LINE__), __func__, (fmt))
#define SIM_LOGGER_FATALF_2(logger, fmt)                                            \
  ::sim_logger::detail::log_printf((logger), ::sim_logger::Level::Fatal, __FILE__,  \
                                  static_cast<unsigned>(__LINE__), __func__, (fmt))

// (logger, fmt, ...)
#define SIM_LOGGER_DEBUGF_3(logger, fmt, ...)                                       \
  ::sim_logger::detail::log_printf((logger), ::sim_logger::Level::Debug, __FILE__,  \
                                  static_cast<unsigned>(__LINE__), __func__, (fmt), \
                                  __VA_ARGS__)
#define SIM_LOGGER_INFOF_3(logger, fmt, ...)                                        \
  ::sim_logger::detail::log_printf((logger), ::sim_logger::Level::Info, __FILE__,   \
                                  static_cast<unsigned>(__LINE__), __func__, (fmt), \
                                  __VA_ARGS__)
#define SIM_LOGGER_WARNF_3(logger, fmt, ...)                                        \
  ::sim_logger::detail::log_printf((logger), ::sim_logger::Level::Warn, __FILE__,   \
                                  static_cast<unsigned>(__LINE__), __func__, (fmt), \
                                  __VA_ARGS__)
#define SIM_LOGGER_ERRORF_3(logger, fmt, ...)                                       \
  ::sim_logger::detail::log_printf((logger), ::sim_logger::Level::Error, __FILE__,  \
                                  static_cast<unsigned>(__LINE__), __func__, (fmt), \
                                  __VA_ARGS__)
#define SIM_LOGGER_FATALF_3(logger, fmt, ...)                                       \
  ::sim_logger::detail::log_printf((logger), ::sim_logger::Level::Fatal, __FILE__,  \
                                  static_cast<unsigned>(__LINE__), __func__, (fmt), \
                                  __VA_ARGS__)

// ---- Dispatch tables per level ----
// 2 args -> _2, 3..10 args -> _3
#define SIM_LOGGER_DEBUGF_IMPL2(...) SIM_LOGGER_DEBUGF_2(__VA_ARGS__)
#define SIM_LOGGER_DEBUGF_IMPL3(...) SIM_LOGGER_DEBUGF_3(__VA_ARGS__)
#define SIM_LOGGER_DEBUGF_IMPL4(...) SIM_LOGGER_DEBUGF_3(__VA_ARGS__)
#define SIM_LOGGER_DEBUGF_IMPL5(...) SIM_LOGGER_DEBUGF_3(__VA_ARGS__)
#define SIM_LOGGER_DEBUGF_IMPL6(...) SIM_LOGGER_DEBUGF_3(__VA_ARGS__)
#define SIM_LOGGER_DEBUGF_IMPL7(...) SIM_LOGGER_DEBUGF_3(__VA_ARGS__)
#define SIM_LOGGER_DEBUGF_IMPL8(...) SIM_LOGGER_DEBUGF_3(__VA_ARGS__)
#define SIM_LOGGER_DEBUGF_IMPL9(...) SIM_LOGGER_DEBUGF_3(__VA_ARGS__)
#define SIM_LOGGER_DEBUGF_IMPL10(...) SIM_LOGGER_DEBUGF_3(__VA_ARGS__)

#define SIM_LOGGER_INFOF_IMPL2(...) SIM_LOGGER_INFOF_2(__VA_ARGS__)
#define SIM_LOGGER_INFOF_IMPL3(...) SIM_LOGGER_INFOF_3(__VA_ARGS__)
#define SIM_LOGGER_INFOF_IMPL4(...) SIM_LOGGER_INFOF_3(__VA_ARGS__)
#define SIM_LOGGER_INFOF_IMPL5(...) SIM_LOGGER_INFOF_3(__VA_ARGS__)
#define SIM_LOGGER_INFOF_IMPL6(...) SIM_LOGGER_INFOF_3(__VA_ARGS__)
#define SIM_LOGGER_INFOF_IMPL7(...) SIM_LOGGER_INFOF_3(__VA_ARGS__)
#define SIM_LOGGER_INFOF_IMPL8(...) SIM_LOGGER_INFOF_3(__VA_ARGS__)
#define SIM_LOGGER_INFOF_IMPL9(...) SIM_LOGGER_INFOF_3(__VA_ARGS__)
#define SIM_LOGGER_INFOF_IMPL10(...) SIM_LOGGER_INFOF_3(__VA_ARGS__)

#define SIM_LOGGER_WARNF_IMPL2(...) SIM_LOGGER_WARNF_2(__VA_ARGS__)
#define SIM_LOGGER_WARNF_IMPL3(...) SIM_LOGGER_WARNF_3(__VA_ARGS__)
#define SIM_LOGGER_WARNF_IMPL4(...) SIM_LOGGER_WARNF_3(__VA_ARGS__)
#define SIM_LOGGER_WARNF_IMPL5(...) SIM_LOGGER_WARNF_3(__VA_ARGS__)
#define SIM_LOGGER_WARNF_IMPL6(...) SIM_LOGGER_WARNF_3(__VA_ARGS__)
#define SIM_LOGGER_WARNF_IMPL7(...) SIM_LOGGER_WARNF_3(__VA_ARGS__)
#define SIM_LOGGER_WARNF_IMPL8(...) SIM_LOGGER_WARNF_3(__VA_ARGS__)
#define SIM_LOGGER_WARNF_IMPL9(...) SIM_LOGGER_WARNF_3(__VA_ARGS__)
#define SIM_LOGGER_WARNF_IMPL10(...) SIM_LOGGER_WARNF_3(__VA_ARGS__)

#define SIM_LOGGER_ERRORF_IMPL2(...) SIM_LOGGER_ERRORF_2(__VA_ARGS__)
#define SIM_LOGGER_ERRORF_IMPL3(...) SIM_LOGGER_ERRORF_3(__VA_ARGS__)
#define SIM_LOGGER_ERRORF_IMPL4(...) SIM_LOGGER_ERRORF_3(__VA_ARGS__)
#define SIM_LOGGER_ERRORF_IMPL5(...) SIM_LOGGER_ERRORF_3(__VA_ARGS__)
#define SIM_LOGGER_ERRORF_IMPL6(...) SIM_LOGGER_ERRORF_3(__VA_ARGS__)
#define SIM_LOGGER_ERRORF_IMPL7(...) SIM_LOGGER_ERRORF_3(__VA_ARGS__)
#define SIM_LOGGER_ERRORF_IMPL8(...) SIM_LOGGER_ERRORF_3(__VA_ARGS__)
#define SIM_LOGGER_ERRORF_IMPL9(...) SIM_LOGGER_ERRORF_3(__VA_ARGS__)
#define SIM_LOGGER_ERRORF_IMPL10(...) SIM_LOGGER_ERRORF_3(__VA_ARGS__)

#define SIM_LOGGER_FATALF_IMPL2(...) SIM_LOGGER_FATALF_2(__VA_ARGS__)
#define SIM_LOGGER_FATALF_IMPL3(...) SIM_LOGGER_FATALF_3(__VA_ARGS__)
#define SIM_LOGGER_FATALF_IMPL4(...) SIM_LOGGER_FATALF_3(__VA_ARGS__)
#define SIM_LOGGER_FATALF_IMPL5(...) SIM_LOGGER_FATALF_3(__VA_ARGS__)
#define SIM_LOGGER_FATALF_IMPL6(...) SIM_LOGGER_FATALF_3(__VA_ARGS__)
#define SIM_LOGGER_FATALF_IMPL7(...) SIM_LOGGER_FATALF_3(__VA_ARGS__)
#define SIM_LOGGER_FATALF_IMPL8(...) SIM_LOGGER_FATALF_3(__VA_ARGS__)
#define SIM_LOGGER_FATALF_IMPL9(...) SIM_LOGGER_FATALF_3(__VA_ARGS__)
#define SIM_LOGGER_FATALF_IMPL10(...) SIM_LOGGER_FATALF_3(__VA_ARGS__)

// ---- Public entry points ----
#define LOG_DEBUGF(...) SIM_LOGGER_PP_DISPATCH_BY_NARGS(SIM_LOGGER_DEBUGF, __VA_ARGS__)
#define LOG_INFOF(...)  SIM_LOGGER_PP_DISPATCH_BY_NARGS(SIM_LOGGER_INFOF, __VA_ARGS__)
#define LOG_WARNF(...)  SIM_LOGGER_PP_DISPATCH_BY_NARGS(SIM_LOGGER_WARNF, __VA_ARGS__)
#define LOG_ERRORF(...) SIM_LOGGER_PP_DISPATCH_BY_NARGS(SIM_LOGGER_ERRORF, __VA_ARGS__)
#define LOG_FATALF(...) SIM_LOGGER_PP_DISPATCH_BY_NARGS(SIM_LOGGER_FATALF, __VA_ARGS__)
