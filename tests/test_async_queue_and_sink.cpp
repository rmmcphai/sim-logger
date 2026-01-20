#include "logger/async_sink.hpp"
#include "logger/detail/mutex_ring_buffer_queue.hpp"
#include "logger/test_sink.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <future>
#include <stdexcept>
#include <thread>

using namespace sim_logger;

namespace {

LogRecord make_record(Level lvl, std::string msg = "m") {
  return LogRecord(lvl,
                   /*sim_time*/ 1.0,
                   /*met*/ 2.0,
                   /*wall_time_ns*/ 3,
                   std::this_thread::get_id(),
                   "f.cpp",
                   10,
                   "fn",
                   "root",
                   {},
                   std::move(msg));
}

struct ThrowingSink final : ISink {
  void write(const LogRecord&) override { throw std::runtime_error("boom"); }
  void flush() override { throw std::runtime_error("boom"); }
};

}  // namespace

TEST_CASE("MutexRingBufferQueue DropNewest drops deterministically", "[async][queue]") {
  detail::MutexRingBufferQueue q(/*capacity*/ 1, OverflowPolicy::DropNewest);

  auto r1 = make_record(Level::Info, "a");
  auto r2 = make_record(Level::Info, "b");

  auto res1 = q.enqueue(std::move(r1));
  REQUIRE(res1.enqueued);
  REQUIRE(res1.dropped == 0);

  auto res2 = q.enqueue(std::move(r2));
  REQUIRE_FALSE(res2.enqueued);
  REQUIRE(res2.dropped == 1);

  std::vector<LogRecord> out;
  REQUIRE(q.dequeue_batch(out, 10) == 1);
  REQUIRE(out.size() == 1);
  REQUIRE(out[0].message() == "a");
}

TEST_CASE("MutexRingBufferQueue DropOldest drops oldest deterministically", "[async][queue]") {
  detail::MutexRingBufferQueue q(/*capacity*/ 1, OverflowPolicy::DropOldest);

  auto r1 = make_record(Level::Info, "a");
  auto r2 = make_record(Level::Info, "b");

  auto res1 = q.enqueue(std::move(r1));
  REQUIRE(res1.enqueued);
  REQUIRE(res1.dropped == 0);

  auto res2 = q.enqueue(std::move(r2));
  REQUIRE(res2.enqueued);
  REQUIRE(res2.dropped == 1);

  std::vector<LogRecord> out;
  REQUIRE(q.dequeue_batch(out, 10) == 1);
  REQUIRE(out.size() == 1);
  REQUIRE(out[0].message() == "b");
}

TEST_CASE("MutexRingBufferQueue Block blocks until space is available", "[async][queue]") {
  detail::MutexRingBufferQueue q(/*capacity*/ 1, OverflowPolicy::Block);

  auto r1 = make_record(Level::Info, "a");
  REQUIRE(q.enqueue(std::move(r1)).enqueued);

  std::promise<void> started_wait;
  std::shared_future<void> started_wait_f = started_wait.get_future().share();

  auto fut = std::async(std::launch::async, [&] {
    started_wait.set_value();
    auto r2 = make_record(Level::Info, "b");
    return q.enqueue(std::move(r2)).enqueued;
  });

  started_wait_f.wait();

  REQUIRE(fut.wait_for(std::chrono::milliseconds(10)) == std::future_status::timeout);

  std::vector<LogRecord> out;
  REQUIRE(q.dequeue_batch(out, 1) == 1);

  REQUIRE(fut.get() == true);
}

TEST_CASE("AsyncSink flush drains and delivers to wrapped sink", "[async][sink]") {
  auto wrapped = std::make_shared<TestSink>();
  AsyncOptions opt;
  opt.capacity = 16;
  opt.overflow_policy = OverflowPolicy::DropNewest;
  opt.max_batch = 8;

  AsyncSink async(wrapped, opt);

  for (int i = 0; i < 20; ++i) {
    async.write(make_record(Level::Info, "m" + std::to_string(i)));
  }

  async.flush();

  REQUIRE(wrapped->size() > 0);
  REQUIRE(async.dropped_records_count() <= 20);
}

TEST_CASE("AsyncSink contains wrapped sink exceptions and increments failure count", "[async][sink]") {
  auto bad = std::make_shared<ThrowingSink>();
  AsyncOptions opt;
  opt.capacity = 4;
  opt.overflow_policy = OverflowPolicy::Block;
  opt.max_batch = 4;

  AsyncSink async(bad, opt);

  REQUIRE_NOTHROW(async.write(make_record(Level::Info, "x")));
  REQUIRE_NOTHROW(async.flush());

  REQUIRE(async.sink_failures_count() > 0);
}
