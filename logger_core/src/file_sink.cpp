#include "logger/file_sink.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

#if !defined(_WIN32)
#include <unistd.h>  // fsync, fileno
#endif

namespace sim_logger {
namespace {

void write_all_or_throw(std::FILE* f, const char* data, size_t size) {
  if (f == nullptr) {
    throw std::runtime_error("FileSink file handle is null");
  }
  const size_t written = std::fwrite(data, 1U, size, f);
  if (written != size) {
    const int err = errno;
    throw std::runtime_error(std::string("FileSink write failed: ") + std::strerror(err));
  }
}

void fflush_or_throw(std::FILE* f) {
  if (f == nullptr) {
    throw std::runtime_error("FileSink file handle is null");
  }
  if (std::fflush(f) != 0) {
    const int err = errno;
    throw std::runtime_error(std::string("FileSink fflush failed: ") + std::strerror(err));
  }
}

#if !defined(_WIN32)
void fsync_or_throw(std::FILE* f) {
  const int fd = ::fileno(f);
  if (fd < 0) {
    const int err = errno;
    throw std::runtime_error(std::string("FileSink fileno failed: ") + std::strerror(err));
  }
  if (::fsync(fd) != 0) {
    const int err = errno;
    throw std::runtime_error(std::string("FileSink fsync failed: ") + std::strerror(err));
  }
}
#endif

}  // namespace

FileSink::FileSink(std::string path, PatternFormatter formatter, bool durable_flush)
    : path_(std::move(path)),
      formatter_(std::move(formatter)),
      durable_flush_(durable_flush) {
  if (path_.empty()) {
    throw std::invalid_argument("FileSink path must not be empty");
  }
  open_or_throw();
}

FileSink::~FileSink() { close_noexcept(); }

void FileSink::open_or_throw() {
  // Append mode; create if missing.
  file_ = std::fopen(path_.c_str(), "a");
  if (file_ == nullptr) {
    const int err = errno;
    throw std::runtime_error(std::string("FileSink fopen failed for '") + path_ +
                             "': " + std::strerror(err));
  }
}

void FileSink::close_noexcept() noexcept {
  if (file_ != nullptr) {
    std::fclose(file_);
    file_ = nullptr;
  }
}

void FileSink::write(const LogRecord& record) {
  const std::string line = formatter_.format(record);

  std::lock_guard<std::mutex> lock(mu_);

  write_all_or_throw(file_, line.c_str(), line.size());
  if (line.empty() || line.back() != '\n') {
    write_all_or_throw(file_, "\n", 1U);
  }
}

void FileSink::flush() {
  std::lock_guard<std::mutex> lock(mu_);

  fflush_or_throw(file_);

#if !defined(_WIN32)
  if (durable_flush_) {
    fsync_or_throw(file_);
  }
#else
  (void)durable_flush_;
#endif
}

}  // namespace sim_logger
