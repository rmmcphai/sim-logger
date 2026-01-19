#include <catch2/catch_test_macros.hpp>

#include "logger/pattern_formatter.hpp"

#include "logger/level.hpp"
#include "logger/log_record.hpp"

#include <thread>
#include <vector>

namespace sim_logger {

namespace {

static LogRecord make_record() {
  std::vector<Tag> tags;
  tags.push_back(Tag{"k1", "v1"});
  tags.push_back(Tag{"k2", "v2"});

  // Use a fixed thread id so "{thread}" is deterministic.
  // (std::thread::id has no portable default constructor semantics across libs,
  // so we use the current thread id but do not assert on the numeric value.)
  const std::thread::id tid = std::this_thread::get_id();

  return LogRecord(
      Level::Info,
      /*sim_time=*/123.5,
      /*mission_elapsed=*/42.25,
      /*wall_time_ns=*/999ULL,
      /*thread_id=*/tid,
      /*file=*/"file.cpp",
      /*line=*/321U,
      /*function=*/"func()",
      /*logger_name=*/"a.b.c",
      /*tags=*/tags,
      /*message=*/"hello");
}

}  // namespace

TEST_CASE("PatternFormatter renders known tokens", "[formatter][pattern]") {
  const auto rec = make_record();

  PatternFormatter fmt("{level} {sim} {met} {wall_ns} {file}:{line} {function} {logger} {msg}");

  const std::string out = fmt.format(rec);

  // Fixed-point rule in formatter: 6 fractional digits.
  // sim_time 123.5 -> "123.500000"
  // met      42.25 -> "42.250000"
  REQUIRE(out ==
          "INFO 123.500000 42.250000 999 file.cpp:321 func() a.b.c hello");
}

TEST_CASE("PatternFormatter preserves unknown tokens verbatim", "[formatter][pattern]") {
  const auto rec = make_record();

  PatternFormatter fmt("X{unknown}Y {msg}");

  const std::string out = fmt.format(rec);

  REQUIRE(out == "X{unknown}Y hello");
}

TEST_CASE("PatternFormatter leaves unmatched '{' as literal remainder", "[formatter][pattern]") {
  const auto rec = make_record();

  PatternFormatter fmt("abc {msg} {broken");

  const std::string out = fmt.format(rec);

  // The unmatched "{broken" is appended as-is.
  REQUIRE(out == "abc hello {broken");
}

TEST_CASE("PatternFormatter token extraction identifies tokens", "[formatter][pattern]") {
  PatternFormatter fmt("{level} {sim} {met} {logger} {msg} {unknown}");

  const auto& toks = fmt.tokens();

  REQUIRE(toks.find("level") != toks.end());
  REQUIRE(toks.find("sim") != toks.end());
  REQUIRE(toks.find("met") != toks.end());
  REQUIRE(toks.find("logger") != toks.end());
  REQUIRE(toks.find("msg") != toks.end());
  REQUIRE(toks.find("unknown") != toks.end());
}

TEST_CASE("PatternFormatter can enforce presence of {met}", "[formatter][pattern]") {
  REQUIRE_NOTHROW(PatternFormatter("{met} {msg}", /*require_met_token=*/true));
  REQUIRE_THROWS_AS(PatternFormatter("{sim} {msg}", /*require_met_token=*/true),
                    std::invalid_argument);
}

}  // namespace sim_logger
