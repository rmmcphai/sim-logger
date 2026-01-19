#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace sim_logger {

/**
 * @file level.hpp
 * @brief Defines log severity levels and parsing helpers.
 *
 * @details
 * Canonical test-suite contract:
 * - to_string(Level) returns uppercase names for DEBUG/INFO/WARN/ERROR/FATAL.
 * - level_from_string(...) parses those names case-insensitively and accepts "warning".
 * - level_from_int(int) accepts Trick-style numeric compatibility values.
 * - "TRACE" is intentionally not supported by parsing in this build.
 */
enum class Level : uint8_t {
  Debug = 0,
  Info  = 1,
  Warn  = 2,
  Error = 3,
  Fatal = 4
};

/**
 * @brief Return canonical uppercase name.
 */
std::string_view to_string(Level level) noexcept;

/**
 * @brief Parse a level name (case-insensitive). Accepts "warning" -> Warn.
 *
 * @return Parsed level or std::nullopt.
 */
std::optional<Level> level_from_string(std::string_view s) noexcept;

/**
 * @brief Parse Trick-style numeric levels for compatibility.
 *
 * Mapping (as required by tests):
 * - 0,1 -> Info
 * - 2   -> Warn
 * - 3   -> Error
 * - 10  -> Debug
 */
std::optional<Level> level_from_int(int value) noexcept;

/**
 * @brief Inclusive threshold semantics.
 */
bool is_at_least(Level msg, Level threshold) noexcept;

}  // namespace sim_logger
