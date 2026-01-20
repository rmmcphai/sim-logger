#include "logger/console_sink.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>

#if !defined(_WIN32)
#include <unistd.h>  // isatty, fileno
#endif

namespace sim_logger {
namespace {

constexpr const char* kAnsiReset = "\x1b[0m";

bool is_tty(std::FILE* stream) {
#if defined(_WIN32)
  (void)stream;
  return false;
#else
  if (stream == nullptr) {
    return false;
  }
  const int fd = ::fileno(stream);
  if (fd < 0) {
    return false;
  }
  return ::isatty(fd) != 0;
#endif
}

void write_all_or_throw(std::FILE* stream, const char* data, size_t size) {
  if (stream == nullptr) {
    throw std::invalid_argument("ConsoleSink stream is null");
  }

  const size_t written = std::fwrite(data, 1U, size, stream);
  if (written != size) {
    const int err = errno;
    throw std::runtime_error(std::string("ConsoleSink write failed: ") + std::strerror(err));
  }
}

}  // namespace

ConsoleSink::ConsoleSink(PatternFormatter formatter, ColorMode color_mode, std::FILE* stream)
    : formatter_(std::move(formatter)), color_mode_(color_mode), stream_(stream) {
  if (stream_ == nullptr) {
    throw std::invalid_argument("ConsoleSink stream must not be null");
  }
}

void ConsoleSink::write(const LogRecord& record) {
  const std::string line = formatter_.format(record);

  std::lock_guard<std::mutex> lock(mu_);

  const bool colorize = should_colorize_locked();
  if (colorize) {
    if (const char* prefix = ansi_prefix_for(record.level()); prefix != nullptr) {
      write_all_or_throw(stream_, prefix, std::strlen(prefix));
    }
  }

  write_all_or_throw(stream_, line.c_str(), line.size());

  // Always terminate with a newline (pattern strings typically omit it).
  if (line.empty() || line.back() != '\n') {
    write_all_or_throw(stream_, "\n", 1U);
  }

  if (colorize) {
    write_all_or_throw(stream_, kAnsiReset, std::strlen(kAnsiReset));
  }
}

void ConsoleSink::flush() {
  std::lock_guard<std::mutex> lock(mu_);
  if (stream_ == nullptr) {
    return;
  }

  if (std::fflush(stream_) != 0) {
    const int err = errno;
    throw std::runtime_error(std::string("ConsoleSink flush failed: ") + std::strerror(err));
  }
}

bool ConsoleSink::should_colorize_locked() const {
  switch (color_mode_) {
    case ColorMode::Always:
      return true;
    case ColorMode::Never:
      return false;
    case ColorMode::Auto:
    default:
      return is_tty(stream_);
  }
}

const char* ConsoleSink::ansi_prefix_for(Level level) noexcept {
  // Basic, conventional severity coloring.
  // - Warn: yellow
  // - Error/Fatal: red
  // - Debug: dim gray
  switch (level) {
    case Level::Warn:
      return "\x1b[33m";
    case Level::Error:
    case Level::Fatal:
      return "\x1b[31m";
    case Level::Debug:
      return "\x1b[90m";
    case Level::Info:
    default:
      return nullptr;
  }
}

}  // namespace sim_logger
