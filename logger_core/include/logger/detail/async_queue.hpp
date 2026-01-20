#pragma once

#include "logger/async_sink.hpp"
#include "logger/log_record.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sim_logger::detail {

/**
 * @brief Result of an enqueue attempt.
 */
struct EnqueueResult {
  bool enqueued = false;
  std::uint32_t dropped = 0;  // number of records dropped to satisfy the enqueue
};

/**
 * @brief Internal queue abstraction used by AsyncSink.
 *
 * @details
 * This abstraction exists so the v1 backend can be a mutex-based ring buffer
 * while a future v2 backend can be swapped in (e.g., per-producer SPSC queues).
 */
class IQueue {
 public:
  virtual ~IQueue() = default;

  /**
   * @brief Enqueue a record.
   *
   * @returns
   * - enqueued=false when the queue is stopping (or DropNewest overflow).
   * - dropped>0 indicates records were dropped due to overflow policy.
   */
  virtual EnqueueResult enqueue(LogRecord&& r) = 0;

  /**
   * @brief Dequeue up to max records, appending them to out.
   * @return number of records appended.
   */
  virtual std::size_t dequeue_batch(std::vector<LogRecord>& out, std::size_t max) = 0;

  /**
   * @brief Whether the queue is empty.
   */
  virtual bool empty() const = 0;

  /**
   * @brief Request stop and wake any blocked threads.
   */
  virtual void request_stop() = 0;

  /**
   * @brief Wake the consumer thread (used for flush kicks).
   */
  virtual void notify_consumer() = 0;
};

}  // namespace sim_logger::detail
