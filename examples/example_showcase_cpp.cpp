#include "logger/async_sink.hpp"
#include "logger/console_sink.hpp"
#include "logger/dummy_time_source.hpp"
#include "logger/global_time.hpp"
#include "logger/level.hpp"
#include "logger/log_macros.hpp"
#include "logger/logger_registry.hpp"
#include "logger/pattern_formatter.hpp"
#include "logger/rotating_file_sink.hpp"

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

int main() {
  using namespace sim_logger;

  // ---------------------------------------------------------------------------
  // 1) Time: install a deterministic time source.
  //
  // In a real non-Trick simulation you would install your own ITimeSource that
  // returns your model's simulation time and MET.
  //
  // For this example, we use DummyTimeSource so {sim} and {met} change in a
  // predictable way that you can see in the log output.
  // ---------------------------------------------------------------------------
  auto time = std::make_shared<DummyTimeSource>(
      /*sim_time*/ 1000.0,
      /*met*/ 500.0,
      /*wall_time_ns*/ 0);
  set_global_time_source(time);

  // ---------------------------------------------------------------------------
  // 2) Formatting: pattern tokens. MET is required and first-class.
  //
  // Supported tokens:
  //   {level} {sim} {met} {wall_ns} {thread} {file} {line} {function} {logger} {msg}
  //
  // The newline is included here so each record is one line in text sinks.
  // ---------------------------------------------------------------------------
  PatternFormatter fmt(
      "{met} {sim} {level} [{logger}] {msg} (tid={thread} {file}:{line})\n",
      /*require_met_token=*/true);

  // ---------------------------------------------------------------------------
  // 3) Sinks:
  //    - ConsoleSink: readable during interactive runs
  //    - RotatingFileSink: safe for long runs (size-based rotation + retention)
  //    - AsyncSink: wraps the file sink to reduce hot-path overhead
  // ---------------------------------------------------------------------------
  auto console = std::make_shared<ConsoleSink>(fmt, ConsoleSink::ColorMode::Auto);

  // Small max_bytes so rotation triggers quickly when you run this example.
  const std::uint64_t max_bytes = 32 * 1024;
  const std::size_t max_rotated_files = 3;
  auto rotating = std::make_shared<RotatingFileSink>(
      "sim.log", fmt, max_bytes, /*durable_flush=*/false, max_rotated_files);

  AsyncOptions aopt;
  aopt.capacity = 256;
  aopt.overflow_policy = OverflowPolicy::DropNewest;  // deterministic drops
  aopt.max_batch = 64;
  auto async_file = std::make_shared<AsyncSink>(rotating, aopt);

  // ---------------------------------------------------------------------------
  // 4) Loggers: hierarchical names + thresholds.
  //    - root logger "sim": INFO threshold
  //    - "sim.gnc": DEBUG threshold (overrides parent)
  //    - "sim.dyn": inherits INFO from parent
  // ---------------------------------------------------------------------------
  auto root = LoggerRegistry::instance().get_logger("sim");
  root->set_level(Level::Info);
  root->set_sinks(std::vector<std::shared_ptr<ISink>>{console, async_file});

  auto gnc = LoggerRegistry::instance().get_logger("sim.gnc");
  gnc->set_level(Level::Debug);

  auto dyn = LoggerRegistry::instance().get_logger("sim.dyn");
  // dyn inherits INFO from root (no override)

  // ---------------------------------------------------------------------------
  // 5) Demonstrate filtering by threshold.
  // ---------------------------------------------------------------------------
  LOG_DEBUG(dyn, std::string("This DEBUG will be filtered out (dyn inherits INFO)."));
  LOG_INFO(dyn, std::string("This INFO is emitted (dyn inherits INFO)."));

  LOG_DEBUG(gnc, std::string("This DEBUG is emitted (gnc overrides to DEBUG)."));
  LOG_INFO(gnc, std::string("This INFO is emitted."));

  // ---------------------------------------------------------------------------
  // 6) Demonstrate advancing time (MET + sim time show in formatted output).
  // ---------------------------------------------------------------------------
  for (int step = 0; step < 5; ++step) {
    time->advance(/*sim_delta*/ 0.5, /*met_delta*/ 0.5, /*wall_delta_ns*/ 500000000);
    LOG_INFOF(dyn, "Step=%d dt=%.1f", step, 0.5);
  }

  // ---------------------------------------------------------------------------
  // 7) Demonstrate async queue overflow behavior (DropNewest).
  //
  // We intentionally burst logs to overflow the small queue.
  // In a real simulation you'd typically use Block for "never lose logs" behavior.
  // ---------------------------------------------------------------------------
  for (int i = 0; i < 2000; ++i) {
    LOG_DEBUGF(gnc, "burst i=%d", i);
  }

  // ---------------------------------------------------------------------------
  // 8) Flush: in stand-alone mode, flush the effective sinks before exit.
  //
  // AsyncSink::flush() deterministically drains the queue and flushes the wrapped sink.
  // ---------------------------------------------------------------------------
  for (const auto& sink : root->effective_sinks()) {
    sink->flush();
  }

  // ---------------------------------------------------------------------------
  // 9) Report basic stats.
  // ---------------------------------------------------------------------------
  std::cout << "\n--- Stats ---\n";
  std::cout << "root dropped_records_count (filtered): " << root->dropped_records_count() << "\n";
  std::cout << "gnc dropped_records_count (filtered): " << gnc->dropped_records_count() << "\n";
  std::cout << "AsyncSink dropped due to overflow: " << async_file->dropped_records_count() << "\n";
  std::cout << "AsyncSink sink failures: " << async_file->sink_failures_count() << "\n";

  std::cout << "\nWrote to: sim.log (rotates quickly; retention keeps last 3 rotated files)\n";
  return 0;
}

