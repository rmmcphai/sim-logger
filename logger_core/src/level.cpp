#include "logger/level.hpp"

#include <optional>
#include <string_view>

namespace sim_logger {

/**
 * @file level.cpp
 * @brief Implements level parsing utilities.
 *
 * @details
 * These utilities are used primarily during configuration parsing.
 * They are intentionally:
 *  - allocation-free,
 *  - ASCII-only (no locale surprises),
 *  - noexcept (logging infrastructure should not throw during normal operation).
 */

namespace {

constexpr char to_upper_ascii(char c) noexcept {
  return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
}

bool iequals_ascii(std::string_view a, std::string_view b) noexcept {
  if (a.size() != b.size()) {
    return false;
  }
  for (size_t i = 0; i < a.size(); ++i) {
    if (to_upper_ascii(a[i]) != to_upper_ascii(b[i])) {
      return false;
    }
  }
  return true;
}

}  // namespace

std::string_view to_string(Level level) noexcept {
  switch (level) {
    case Level::Debug: return "DEBUG";
    case Level::Info:  return "INFO";
    case Level::Warn:  return "WARN";
    case Level::Error: return "ERROR";
    case Level::Fatal: return "FATAL";
    default:           return "UNKNOWN";
  }
}

std::optional<Level> level_from_string(std::string_view s) noexcept {
  if (iequals_ascii(s, "DEBUG")) return Level::Debug;
  if (iequals_ascii(s, "INFO"))  return Level::Info;
  if (iequals_ascii(s, "WARN") || iequals_ascii(s, "WARNING")) return Level::Warn;
  if (iequals_ascii(s, "ERROR")) return Level::Error;
  if (iequals_ascii(s, "FATAL")) return Level::Fatal;

  // Canonical suite expects TRACE to be rejected in this build.
  return std::nullopt;
}

std::optional<Level> level_from_int(int value) noexcept {
  switch (value) {
    case 0:
    case 1:
      return Level::Info;
    case 2:
      return Level::Warn;
    case 3:
      return Level::Error;
    case 10:
      return Level::Debug;
    default:
      return std::nullopt;
  }
}

bool is_at_least(Level msg, Level threshold) noexcept {
  return static_cast<uint8_t>(msg) >= static_cast<uint8_t>(threshold);
}

}  // namespace sim_logger
