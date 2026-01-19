#include "logger/global_time.hpp"

#include "logger/dummy_time_source.hpp"

#include <mutex>
#include <utility>

namespace sim_logger {

namespace {

struct GlobalTimeState {
  std::mutex mutex;
  std::shared_ptr<ITimeSource> installed;
  std::shared_ptr<DummyTimeSource> fallback = std::make_shared<DummyTimeSource>();
};

GlobalTimeState& state() {
  static GlobalTimeState s;
  return s;
}

}  // namespace

void set_global_time_source(std::shared_ptr<ITimeSource> source) {
  auto& s = state();
  std::lock_guard<std::mutex> lock(s.mutex);

  if (source) {
    s.installed = std::move(source);
  } else {
    s.installed.reset();
  }
}

std::shared_ptr<ITimeSource> global_time_source() {
  auto& s = state();
  std::lock_guard<std::mutex> lock(s.mutex);

  if (s.installed) {
    return s.installed;
  }
  return s.fallback;
}

ITimeSource& global_time_source_ref() {
  thread_local std::shared_ptr<ITimeSource> tls_snapshot;
  tls_snapshot = global_time_source();
  return *tls_snapshot;
}

}  // namespace sim_logger
