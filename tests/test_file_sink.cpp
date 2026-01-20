#include <catch2/catch_test_macros.hpp>

#include "logger/file_sink.hpp"

#include "logger/log_record.hpp"
#include "logger/pattern_formatter.hpp"

#include <filesystem>
#include <fstream>
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

std::string read_all_text(const std::filesystem::path& p) {
  std::ifstream ifs(p, std::ios::in | std::ios::binary);
  REQUIRE(ifs.good());
  std::string s((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  return s;
}

std::filesystem::path unique_temp_path(const char* stem) {
  const auto dir = std::filesystem::temp_directory_path();
  // Use time-based uniqueness; sufficient for unit tests.
  const auto name =
      std::string(stem) + "-" + std::to_string(static_cast<long long>(std::time(nullptr))) + ".log";
  return dir / name;
}

}  // namespace

TEST_CASE("FileSink appends formatted lines", "[file_sink]") {
  const auto path = unique_temp_path("sim_logger_file_sink");
  std::filesystem::remove(path);

  PatternFormatter fmt("{level} {msg}");
  FileSink sink(path.string(), std::move(fmt), /*durable_flush=*/false);

  sink.write(make_record(Level::Info, "one"));
  sink.write(make_record(Level::Warn, "two"));
  sink.flush();

  const std::string out = read_all_text(path);
  REQUIRE(out == "INFO one\nWARN two\n");

  std::filesystem::remove(path);
}

TEST_CASE("FileSink durable flush does not throw", "[file_sink]") {
  const auto path = unique_temp_path("sim_logger_file_sink_durable");
  std::filesystem::remove(path);

  PatternFormatter fmt("{msg}");
  FileSink sink(path.string(), std::move(fmt), /*durable_flush=*/true);

  sink.write(make_record(Level::Info, "x"));
  REQUIRE_NOTHROW(sink.flush());

  std::filesystem::remove(path);
}

}  // namespace sim_logger
