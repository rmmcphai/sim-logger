#include "logger/async_sink.hpp"
#include "logger/console_sink.hpp"
#include "logger/log_macros.hpp"
#include "logger/logger_registry.hpp"
#include "logger/pattern_formatter.hpp"
#include "logger/rotating_file_sink.hpp"

#include <memory>
#include <string>
#include <vector>

int main() {
  using namespace sim_logger;

  // Root logger that everything inherits from.
  auto root = LoggerRegistry::instance().get_logger("sim");
  root->set_level(Level::Debug);

  // Pattern includes MET and source location.
  PatternFormatter fmt("{met} {level} [{logger}] {msg} ({file}:{line})");

  // Console for interactive runs.
  auto console = std::make_shared<ConsoleSink>(fmt, ConsoleSink::ColorMode::Auto);

  // Rotating file sink for long runs.
  const std::uint64_t max_bytes = 64 * 1024;  // small for demonstration
  const std::size_t max_rotated_files = 5;
  auto rotating = std::make_shared<RotatingFileSink>(
      "sim.log", fmt, max_bytes, /*durable_flush=*/false, max_rotated_files);

  // Async wrapper to keep hot-path overhead low.
  AsyncOptions aopt;
  aopt.capacity = 1024;
  aopt.overflow_policy = OverflowPolicy::Block;
  aopt.max_batch = 256;
  auto async_file = std::make_shared<AsyncSink>(rotating, aopt);

  root->set_sinks(std::vector<std::shared_ptr<ISink>>{console, async_file});
  root->set_immediate_flush(false);

  LOG_INFO(root, std::string("async logging demo"));
  for (int i = 0; i < 1000; ++i) {
    LOG_DEBUGF(root, "i=%d", i);
  }

  // Demonstrate hierarchy: child inherits sinks and level.
  auto gnc = LoggerRegistry::instance().get_logger("sim.gnc");
  LOG_WARN(gnc, std::string("child logger inherits async file sink"));

  // Ensure everything is written before exit.
  for (const auto& sink : root->effective_sinks()) {
    sink->flush();
  }
  return 0;
}
