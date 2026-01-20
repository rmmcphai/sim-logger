#include "logger/async_sink.hpp"

#include "logger/detail/async_queue.hpp"
#include "logger/detail/mutex_ring_buffer_queue.hpp"

#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace sim_logger {

using detail::IQueue;
using detail::MutexRingBufferQueue;

struct AsyncSink::Impl {
  std::thread worker;
  std::mutex flush_m;
  std::condition_variable flush_cv;
};

AsyncSink::AsyncSink(std::shared_ptr<ISink> wrapped, AsyncOptions options)
    : wrapped_(std::move(wrapped)), options_(options), impl_(std::make_unique<Impl>()) {
  if (!wrapped_) {
    throw std::invalid_argument("AsyncSink requires a wrapped sink");
  }
  if (options_.capacity == 0) {
    options_.capacity = 1;
  }
  if (options_.max_batch == 0) {
    options_.max_batch = 1;
  }

  queue_ = std::make_unique<MutexRingBufferQueue>(options_.capacity, options_.overflow_policy);
  impl_->worker = std::thread([this] { worker_loop_(); });
}

AsyncSink::~AsyncSink() {
  request_stop_();
  if (impl_ && impl_->worker.joinable()) {
    impl_->worker.join();
  }
}

void AsyncSink::request_stop_() noexcept {
  if (stop_requested_.exchange(true, std::memory_order_relaxed)) {
    return;
  }
  if (queue_) {
    queue_->request_stop();
  }
  // Wake worker to observe stop.
  if (auto* q = dynamic_cast<MutexRingBufferQueue*>(queue_.get())) {
    q->kick_for_flush();
  } else if (queue_) {
    queue_->notify_consumer();
  }
}

void AsyncSink::write(const LogRecord& record) {
  // LogRecord is immutable; copy then move into queue.
  LogRecord copy = record;
  const auto res = queue_->enqueue(std::move(copy));
  if (res.dropped > 0) {
    dropped_records_count_.fetch_add(res.dropped, std::memory_order_relaxed);
  }
  if (!res.enqueued && res.dropped == 0) {
    // Stop requested (or other non-overflow rejection) counts as a drop.
    dropped_records_count_.fetch_add(1, std::memory_order_relaxed);
  }
}

void AsyncSink::flush() {
  const std::uint64_t gen = flush_request_gen_.fetch_add(1, std::memory_order_acq_rel) + 1;

  // Kick worker even if queue is empty (to observe flush request).
  if (auto* q = dynamic_cast<MutexRingBufferQueue*>(queue_.get())) {
    q->kick_for_flush();
  } else {
    queue_->notify_consumer();
  }

  std::unique_lock<std::mutex> lk(impl_->flush_m);
  impl_->flush_cv.wait(lk, [&] { return flush_done_gen_.load(std::memory_order_acquire) >= gen; });
}

void AsyncSink::worker_loop_() noexcept {
  std::vector<LogRecord> batch;
  batch.reserve(options_.max_batch);

  // Specialize waiting logic for MutexRingBufferQueue (v1 backend).
  auto* q = dynamic_cast<MutexRingBufferQueue*>(queue_.get());
  if (!q) {
    // Should not happen in v1.
    return;
  }

  std::uint64_t last_seen_flush_gen = 0;

  for (;;) {
    // Wait for work, flush request, or stop.
    {
      std::unique_lock<std::mutex> lk(q->mutex());
      q->wait_for_work(lk);
      if (q->stop_requested_unlocked() && !q->has_items_unlocked()) {
        break;
      }
    }

    // Drain batches.
    while (queue_->dequeue_batch(batch, options_.max_batch) > 0) {
      for (const auto& r : batch) {
        try {
          wrapped_->write(r);
        } catch (...) {
          sink_failures_count_.fetch_add(1, std::memory_order_relaxed);
        }
      }
      batch.clear();
    }

    // Handle flush requests.
    const std::uint64_t want = flush_request_gen_.load(std::memory_order_acquire);
    if (want != last_seen_flush_gen) {
      // Ensure queue is drained before flushing wrapped sink.
      try {
        wrapped_->flush();
      } catch (...) {
        sink_failures_count_.fetch_add(1, std::memory_order_relaxed);
      }

      last_seen_flush_gen = want;
      flush_done_gen_.store(last_seen_flush_gen, std::memory_order_release);
      {
        std::lock_guard<std::mutex> lk(impl_->flush_m);
      }
      impl_->flush_cv.notify_all();
    }
  }

  // Final drain on shutdown (best-effort).
  while (queue_->dequeue_batch(batch, options_.max_batch) > 0) {
    for (const auto& r : batch) {
      try {
        wrapped_->write(r);
      } catch (...) {
        sink_failures_count_.fetch_add(1, std::memory_order_relaxed);
      }
    }
    batch.clear();
  }
  try {
    wrapped_->flush();
  } catch (...) {
    sink_failures_count_.fetch_add(1, std::memory_order_relaxed);
  }

  // Unblock any waiting flush.
  const std::uint64_t want = flush_request_gen_.load(std::memory_order_acquire);
  flush_done_gen_.store(want, std::memory_order_release);
  impl_->flush_cv.notify_all();
}

}  // namespace sim_logger
