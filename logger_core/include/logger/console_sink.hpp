#pragma once

#include "logger/pattern_formatter.hpp"
#include "logger/sink.hpp"

#include <cstdio>
#include <mutex>

namespace sim_logger {

/**
 * @file console_sink.hpp
 * @brief Synchronous console sink (stdout/stderr) with optional ANSI coloring.
 *
 * @details
 * Sprint 3 scope:
 * - Synchronous writes to a FILE* (stdout by default).
 * - Optional ANSI colors based on record severity.
 * - Color can be forced on/off or auto-detected (disabled if output is not a TTY).
 *
 * Thread-safety:
 * - This sink serializes output via an internal mutex to prevent interleaving
 *   across threads.
 */
class ConsoleSink final : public ISink {
 public:
  enum class ColorMode {
    /** Auto-enable colors only when output is a TTY. */
    Auto,
    /** Always emit ANSI escape sequences. */
    Always,
    /** Never emit ANSI escape sequences. */
    Never,
  };

  /**
   * @brief Construct a ConsoleSink.
   *
   * @param formatter Formatter used to render records.
   * @param color_mode Color behavior.
   * @param stream Output FILE*. Defaults to stdout.
   */
  explicit ConsoleSink(PatternFormatter formatter,
                       ColorMode color_mode = ColorMode::Auto,
                       std::FILE* stream = stdout);

  void write(const LogRecord& record) override;
  void flush() override;

 private:
  PatternFormatter formatter_;
  ColorMode color_mode_;
  std::FILE* stream_;
  std::mutex mu_;

  bool should_colorize_locked() const;
  static const char* ansi_prefix_for(Level level) noexcept;
};

}  // namespace sim_logger
