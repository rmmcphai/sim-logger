#include "logger/pattern_formatter.hpp"

#include "logger/level.hpp"

#include <charconv>
#include <cctype>
#include <stdexcept>
#include <thread>
#include <cstdio>

namespace sim_logger {

namespace {

/**
 * @brief Append an unsigned integer to a string using to_chars.
 */
void append_u64(std::string& out, uint64_t value) {
  char buf[32];
  auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), value);
  if (ec != std::errc{}) {
    throw std::runtime_error("append_u64 failed");
  }
  out.append(buf, static_cast<size_t>(ptr - buf));
}

/**
 * @brief Append a uint32_t using to_chars.
 */
void append_u32(std::string& out, uint32_t value) {
  char buf[16];
  auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), value);
  if (ec != std::errc{}) {
    throw std::runtime_error("append_u32 failed");
  }
  out.append(buf, static_cast<size_t>(ptr - buf));
}

/**
 * @brief Append a double with fixed formatting.
 *
 * @details
 * Used for sim time and MET (seconds).
 * v1 rule: fixed-point with 6 fractional digits.
 */
void append_double(std::string& out, double value) {
  char buf[64];
  const int n = std::snprintf(buf, sizeof(buf), "%.6f", value);
  if (n < 0) {
    throw std::runtime_error("append_double failed");
  }
  out.append(buf, static_cast<size_t>(n));
}

/**
 * @brief Append a stable textual representation of a thread id.
 *
 * @details
 * Uses a hash to avoid iostream formatting.
 */
void append_thread_id(std::string& out, std::thread::id tid) {
  const auto hashed =
      static_cast<uint64_t>(std::hash<std::thread::id>{}(tid));
  append_u64(out, hashed);
}

}  // namespace

PatternFormatter::PatternFormatter(std::string pattern, bool require_met_token)
    : pattern_(std::move(pattern)),
      tokens_(extract_tokens(pattern_)) {
  if (require_met_token && tokens_.find("met") == tokens_.end()) {
    throw std::invalid_argument("PatternFormatter: required token '{met}' missing");
  }
}

std::unordered_set<std::string>
PatternFormatter::extract_tokens(std::string_view pattern) {
  std::unordered_set<std::string> out;

  size_t i = 0;
  while (i < pattern.size()) {
    if (pattern[i] != '{') {
      ++i;
      continue;
    }

    const size_t start = i + 1;
    size_t j = start;
    while (j < pattern.size() && pattern[j] != '}') {
      ++j;
    }
    if (j >= pattern.size()) {
      break;  // unmatched '{'
    }

    bool ok = (j > start);
    for (size_t k = start; k < j; ++k) {
      const unsigned char c = static_cast<unsigned char>(pattern[k]);
      if (!(std::isalnum(c) || c == '_')) {
        ok = false;
        break;
      }
    }

    if (ok) {
      out.emplace(pattern.substr(start, j - start));
    }

    i = j + 1;
  }

  return out;
}

std::string PatternFormatter::format(const LogRecord& record) const {
  std::string out;
  out.reserve(pattern_.size() + record.message().size() + 32);

  std::string_view pat = pattern_;
  size_t i = 0;

  while (i < pat.size()) {
    if (pat[i] != '{') {
      out.push_back(pat[i]);
      ++i;
      continue;
    }

    const size_t start = i + 1;
    size_t j = start;
    while (j < pat.size() && pat[j] != '}') {
      ++j;
    }
    if (j >= pat.size()) {
      out.append(pat.substr(i));
      break;
    }

    const std::string_view token = pat.substr(start, j - start);

    if (token == "level") {
      out.append(to_string(record.level()));
    } else if (token == "sim") {
      append_double(out, record.sim_time());
    } else if (token == "met") {
      append_double(out, record.mission_elapsed());
    } else if (token == "wall_ns") {
      append_u64(out, record.wall_time_ns());
    } else if (token == "thread") {
      append_thread_id(out, record.thread_id());
    } else if (token == "file") {
      out.append(record.file());
    } else if (token == "line") {
      append_u32(out, record.line());
    } else if (token == "function") {
      out.append(record.function());
    } else if (token == "logger") {
      out.append(record.logger_name());
    } else if (token == "msg") {
      out.append(record.message());
    } else {
      // Unknown token: preserve verbatim
      out.push_back('{');
      out.append(token);
      out.push_back('}');
    }

    i = j + 1;
  }

  return out;
}

}  // namespace sim_logger
