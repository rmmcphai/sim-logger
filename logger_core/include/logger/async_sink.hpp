#pragma once

#include "logger/sink.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace sim_logger {

namespace detail {
class IQueue;
}

/**
 * @brief Overflow behavior when the async queue reaches capacity.
 */
enum class OverflowPolicy {
  /**
   * @brief Block the calling thread until space is available.
   */
  Block,

  /**
   * @brief Drop the newest record when the queue is full.
   */
  DropNewest,

  /**
   * @brief Drop the oldest queued record to make room for the new one.
   */
  DropOldest,
};

/**
 * @brief Options for AsyncSink.
 */
struct AsyncOptions {
  /**
   * @brief Maximum number of queued records.
   */
  std::size_t capacity = 1024;

  /**
   * @brief Queue overflow behavior.
   */
  OverflowPolicy overflow_policy = OverflowPolicy::Block;

  /**
   * @brief Maximum number of records to drain per worker iteration.
   */
  std::size_t max_batch = 256;
};

/**
 * @brief Async wrapper around a sink.
 *
 * @details
 * The frontend (simulation threads) pushes fully materialized LogRecord copies
 * into a bounded queue. A dedicated worker thread drains the queue and forwards
 * records to the wrapped sink.
 *
 * This class is intended as a low-risk v1 async backend:
 *  - bounded queue with deterministic overflow policy
 *  - deterministic flush/shutdown
 *
 * A future v2 backend may replace the queue implementation with per-producer
 * SPSC queues and a backend merge, without changing Logger or sink APIs.
 */
class AsyncSink final : public ISink {
 public:
  AsyncSink(std::shared_ptr<ISink> wrapped, AsyncOptions options);
  ~AsyncSink() override;

  AsyncSink(const AsyncSink&) = delete;
  AsyncSink& operator=(const AsyncSink&) = delete;

  void write(const LogRecord& record) override;
  void flush() override;

  /**
   * @brief Total number of records dropped due to queue overflow.
   */
  std::uint64_t dropped_records_count() const noexcept {
    return dropped_records_count_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Total number of times the wrapped sink threw during write/flush.
   */
  std::uint64_t sink_failures_count() const noexcept {
    return sink_failures_count_.load(std::memory_order_relaxed);
  }

 private:
  void worker_loop_() noexcept;
  void request_stop_() noexcept;

  std::shared_ptr<ISink> wrapped_;
  AsyncOptions options_;

  std::unique_ptr<detail::IQueue> queue_;

  std::atomic<bool> stop_requested_{false};

  // Flush coordination.
  std::atomic<std::uint64_t> flush_request_gen_{0};
  std::atomic<std::uint64_t> flush_done_gen_{0};

  std::atomic<std::uint64_t> dropped_records_count_{0};
  std::atomic<std::uint64_t> sink_failures_count_{0};

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace sim_logger
