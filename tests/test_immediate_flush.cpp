#include <catch2/catch_test_macros.hpp>

#include "logger/logger.hpp"
#include "logger/logger_registry.hpp"

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace sim_logger {
namespace {

class CountingSink final : public ISink {
 public:
  void write(const LogRecord&) override { writes_.fetch_add(1, std::memory_order_relaxed); }
  void flush() override { flushes_.fetch_add(1, std::memory_order_relaxed); }

  std::uint64_t writes() const noexcept { return writes_.load(std::memory_order_relaxed); }
  std::uint64_t flushes() const noexcept { return flushes_.load(std::memory_order_relaxed); }

 private:
  std::atomic<std::uint64_t> writes_{0};
  std::atomic<std::uint64_t> flushes_{0};
};

LogRecord make_record(Level level) {
  return LogRecord(level,
                   /*sim_time=*/1.0,
                   /*mission_elapsed=*/2.0,
                   /*wall_time_ns=*/3,
                   /*thread_id=*/std::this_thread::get_id(),
                   /*file=*/"f.cpp",
                   /*line=*/7U,
                   /*function=*/"func",
                   /*logger_name=*/"a.b",
                   /*tags=*/std::vector<Tag>{},
                   /*message=*/"m");
}

}  // namespace

TEST_CASE("Immediate flush default is false", "[immediate_flush]") {
  LoggerRegistry::instance().clear();

  auto logger = LoggerRegistry::instance().get_logger("root");
  auto sink = std::make_shared<CountingSink>();
  logger->set_sinks({sink});

  logger->log(make_record(Level::Info));

  REQUIRE(sink->writes() == 1);
  REQUIRE(sink->flushes() == 0);
}

TEST_CASE("Immediate flush can be enabled per logger", "[immediate_flush]") {
  LoggerRegistry::instance().clear();

  auto logger = LoggerRegistry::instance().get_logger("root");
  auto sink = std::make_shared<CountingSink>();
  logger->set_sinks({sink});

  logger->set_immediate_flush(true);
  logger->log(make_record(Level::Info));

  REQUIRE(sink->writes() == 1);
  REQUIRE(sink->flushes() == 1);
}

TEST_CASE("Immediate flush inherits from parent and can be overridden", "[immediate_flush]") {
  LoggerRegistry::instance().clear();

  auto parent = LoggerRegistry::instance().get_logger("vehicle1");
  auto child = LoggerRegistry::instance().get_logger("vehicle1.propulsion");

  auto sink = std::make_shared<CountingSink>();
  parent->set_sinks({sink});

  // Parent enables immediate flush; child should inherit.
  parent->set_immediate_flush(true);
  child->log(make_record(Level::Info));
  REQUIRE(sink->writes() == 1);
  REQUIRE(sink->flushes() == 1);

  // Child overrides to disable.
  child->set_immediate_flush(false);
  child->log(make_record(Level::Info));
  REQUIRE(sink->writes() == 2);
  REQUIRE(sink->flushes() == 1);

  // Clear override -> inherit again.
  child->clear_immediate_flush_override();
  child->log(make_record(Level::Info));
  REQUIRE(sink->writes() == 3);
  REQUIRE(sink->flushes() == 2);
}

}  // namespace sim_logger
