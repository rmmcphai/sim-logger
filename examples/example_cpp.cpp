#include "logger/console_sink.hpp"
#include "logger/file_sink.hpp"
#include "logger/log_macros.hpp"
#include "logger/logger_registry.hpp"
#include "logger/pattern_formatter.hpp"

#include <memory>
#include <string>

int main() {
  using namespace sim_logger;

  auto root = LoggerRegistry::instance().get_logger("sim");
  root->set_level(Level::Debug);

  // Simple, readable pattern; adjust as desired.
  PatternFormatter fmt("{met} {level} {logger} {msg}");

  auto console = std::make_shared<ConsoleSink>(fmt, ConsoleSink::ColorMode::Auto);
  auto file = std::make_shared<FileSink>("sim.log", fmt, /*durable_flush=*/false);

  root->set_sinks({console, file});

  // Debug mode convenience.
  root->set_immediate_flush(true);

  LOG_INFO(root, std::string("startup"));
  LOG_DEBUGF(root, "dt=%.3f", 0.100);
  LOG_WARNF(root, "step=%d status=%s", 1, "ok");

  // Demonstrate hierarchy.
  auto child = LoggerRegistry::instance().get_logger("sim.propulsion");
  LOG_INFO(child, std::string("child logger inherits sinks"));

  return 0;
}