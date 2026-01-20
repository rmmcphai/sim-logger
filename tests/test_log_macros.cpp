#include <catch2/catch_test_macros.hpp>

#include "logger/dummy_time_source.hpp"
#include "logger/global_time.hpp"
#include "logger/log_macros.hpp"
#include "logger/logger_registry.hpp"
#include "logger/test_sink.hpp"

#include <memory>
#include <string>

namespace sim_logger {

TEST_CASE("LOG_INFO captures source location and time", "[log_macros]") {
  LoggerRegistry::instance().clear();

  auto ts = std::make_shared<DummyTimeSource>(123.0, 456.0, 789);
  set_global_time_source(ts);

  auto logger = LoggerRegistry::instance().get_logger("vehicle1");
  auto sink = std::make_shared<TestSink>();
  logger->set_sinks({sink});

  const int expected_line = __LINE__ + 1;
  LOG_INFO(logger, std::string("hello"));

  const auto records = sink->snapshot();
  REQUIRE(records.size() == 1);

  const auto& r = records[0];
  REQUIRE(r.level() == Level::Info);
  REQUIRE(r.sim_time() == 123.0);
  REQUIRE(r.mission_elapsed() == 456.0);
  REQUIRE(r.wall_time_ns() == 789);
  REQUIRE(r.logger_name() == "vehicle1");
  REQUIRE(r.message() == "hello");

  REQUIRE_FALSE(r.function().empty());
  REQUIRE(r.line() == static_cast<std::uint32_t>(expected_line));
}

TEST_CASE("LOG_WARNF formats printf-style messages", "[log_macros]") {
  LoggerRegistry::instance().clear();

  auto ts = std::make_shared<DummyTimeSource>(1.0, 2.0, 3);
  set_global_time_source(ts);

  auto logger = LoggerRegistry::instance().get_logger("root");
  auto sink = std::make_shared<TestSink>();
  logger->set_sinks({sink});

  LOG_WARNF(logger, "x=%d y=%s", 7, "ok");

  const auto records = sink->snapshot();
  REQUIRE(records.size() == 1);
  REQUIRE(records[0].level() == Level::Warn);
  REQUIRE(records[0].message() == "x=7 y=ok");
}

}  // namespace sim_logger
