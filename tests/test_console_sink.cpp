#include <catch2/catch_test_macros.hpp>

#include "logger/console_sink.hpp"

#include "logger/level.hpp"
#include "logger/log_record.hpp"
#include "logger/pattern_formatter.hpp"

#include <cstdio>
#include <string>
#include <thread>
#include <vector>

namespace sim_logger {
namespace {

LogRecord make_record(Level level, std::string message) {
  std::vector<Tag> tags;
  tags.push_back(Tag{"k", "v"});

  return LogRecord(
      level,
      /*sim_time=*/1.0,
      /*mission_elapsed=*/2.0,
      /*wall_time_ns=*/3,
      /*thread_id=*/std::this_thread::get_id(),
      /*file=*/"f.cpp",
      /*line=*/7U,
      /*function=*/"func",
      /*logger_name=*/"a.b",
      /*tags=*/tags,
      /*message=*/std::move(message));
}

std::string read_all(std::FILE* f) {
  REQUIRE(f != nullptr);

  std::fflush(f);
  std::fseek(f, 0, SEEK_SET);

  std::string out;
  char buf[256];
  while (true) {
    const size_t n = std::fread(buf, 1U, sizeof(buf), f);
    if (n > 0) {
      out.append(buf, n);
    }
    if (n < sizeof(buf)) {
      break;
    }
  }
  return out;
}

}  // namespace

TEST_CASE("ConsoleSink appends a newline", "[console_sink]") {
  std::FILE* f = std::tmpfile();
  REQUIRE(f != nullptr);

  PatternFormatter fmt("{level} {msg}");
  ConsoleSink sink(std::move(fmt), ConsoleSink::ColorMode::Never, f);

  sink.write(make_record(Level::Info, "hello"));
  sink.flush();

  const std::string out = read_all(f);
  REQUIRE(out == "INFO hello\n");

  std::fclose(f);
}

TEST_CASE("ConsoleSink Auto mode does not emit color when not a TTY", "[console_sink]") {
  std::FILE* f = std::tmpfile();
  REQUIRE(f != nullptr);

  PatternFormatter fmt("{level} {msg}");
  ConsoleSink sink(std::move(fmt), ConsoleSink::ColorMode::Auto, f);

  sink.write(make_record(Level::Warn, "w"));
  sink.flush();

  const std::string out = read_all(f);
  REQUIRE(out == "WARN w\n");

  std::fclose(f);
}

TEST_CASE("ConsoleSink Always mode emits expected ANSI prefix and reset", "[console_sink]") {
  std::FILE* f = std::tmpfile();
  REQUIRE(f != nullptr);

  PatternFormatter fmt("{level} {msg}");
  ConsoleSink sink(std::move(fmt), ConsoleSink::ColorMode::Always, f);

  sink.write(make_record(Level::Warn, "w"));
  sink.flush();

  const std::string out = read_all(f);
  REQUIRE(out == "\x1b[33mWARN w\n\x1b[0m");

  std::fclose(f);
}

}  // namespace sim_logger
