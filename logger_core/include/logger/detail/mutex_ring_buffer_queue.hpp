#pragma once

#include "logger/detail/async_queue.hpp"

#include <condition_variable>
#include <mutex>
#include <optional>
#include <vector>

namespace sim_logger::detail {

/**
 * @brief Mutex + condition_variable bounded ring-buffer queue.
 */
class MutexRingBufferQueue final : public IQueue {
 public:
  MutexRingBufferQueue(std::size_t capacity, OverflowPolicy policy);

  EnqueueResult enqueue(LogRecord&& r) override;
  std::size_t dequeue_batch(std::vector<LogRecord>& out, std::size_t max) override;
  bool empty() const override;
  void request_stop() override;
  void notify_consumer() override;

  /**
   * @brief Wait until work is available, a flush kick is requested, or stop is requested.
   *
   * @note Exposed for AsyncSink's worker loop.
   */
  void wait_for_work(std::unique_lock<std::mutex>& lk);

  /**
   * @brief Mark that a flush kick is requested and wake the consumer.
   */
  void kick_for_flush();

  // Exposed for AsyncSink worker loop (v1 backend).
  std::mutex& mutex() { return m_; }
  bool stop_requested_unlocked() const { return stop_requested_; }
  bool has_items_unlocked() const { return count_ > 0; }

 private:
  void push_unlocked_(LogRecord&& r);
  void pop_oldest_unlocked_();

  mutable std::mutex m_;
  std::condition_variable cv_not_empty_;
  std::condition_variable cv_not_full_;

  std::size_t capacity_;
  OverflowPolicy policy_;

  std::vector<std::optional<LogRecord>> buffer_;
  std::size_t head_ = 0;
  std::size_t tail_ = 0;
  std::size_t count_ = 0;
  bool stop_requested_ = false;
  bool flush_kick_ = false;
};

inline MutexRingBufferQueue::MutexRingBufferQueue(std::size_t capacity, OverflowPolicy policy)
    : capacity_(capacity), policy_(policy), buffer_(capacity) {
  if (capacity_ == 0) {
    capacity_ = 1;
    buffer_.resize(1);
  }
}

inline EnqueueResult MutexRingBufferQueue::enqueue(LogRecord&& r) {
  std::unique_lock<std::mutex> lk(m_);

  if (stop_requested_) {
    return EnqueueResult{false, 0};
  }

  std::uint32_t dropped = 0;

  if (policy_ == OverflowPolicy::Block) {
    cv_not_full_.wait(lk, [&] { return stop_requested_ || count_ < capacity_; });
    if (stop_requested_) {
      return EnqueueResult{false, 0};
    }
  } else if (count_ >= capacity_) {
    if (policy_ == OverflowPolicy::DropNewest) {
      return EnqueueResult{false, 1};
    }
    // DropOldest: evict one then enqueue.
    pop_oldest_unlocked_();
    dropped = 1;
  }

  push_unlocked_(std::move(r));
  cv_not_empty_.notify_one();
  return EnqueueResult{true, dropped};
}

inline std::size_t MutexRingBufferQueue::dequeue_batch(std::vector<LogRecord>& out, std::size_t max) {
  std::unique_lock<std::mutex> lk(m_);
  const std::size_t n = (count_ < max) ? count_ : max;
  for (std::size_t i = 0; i < n; ++i) {
    // Slots are engaged while counted.
    out.emplace_back(std::move(*buffer_[head_]));
    buffer_[head_].reset();
    head_ = (head_ + 1) % capacity_;
    --count_;
  }
  if (n > 0) {
    cv_not_full_.notify_all();
  }
  return n;
}

inline bool MutexRingBufferQueue::empty() const {
  std::lock_guard<std::mutex> lk(m_);
  return count_ == 0;
}

inline void MutexRingBufferQueue::request_stop() {
  {
    std::lock_guard<std::mutex> lk(m_);
    stop_requested_ = true;
  }
  cv_not_empty_.notify_all();
  cv_not_full_.notify_all();
}

inline void MutexRingBufferQueue::notify_consumer() {
  cv_not_empty_.notify_all();
}

inline void MutexRingBufferQueue::wait_for_work(std::unique_lock<std::mutex>& lk) {
  cv_not_empty_.wait(lk, [&] { return stop_requested_ || count_ > 0 || flush_kick_; });
  flush_kick_ = false;
}

inline void MutexRingBufferQueue::kick_for_flush() {
  {
    std::lock_guard<std::mutex> lk(m_);
    flush_kick_ = true;
  }
  cv_not_empty_.notify_all();
}

inline void MutexRingBufferQueue::push_unlocked_(LogRecord&& r) {
  buffer_[tail_] = std::move(r);
  tail_ = (tail_ + 1) % capacity_;
  ++count_;
}

inline void MutexRingBufferQueue::pop_oldest_unlocked_() {
  buffer_[head_].reset();
  head_ = (head_ + 1) % capacity_;
  --count_;
}

}  // namespace sim_logger::detail
