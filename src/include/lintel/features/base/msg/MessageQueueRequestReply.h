#ifndef LINTEL_MESSAGEQUEUEREQUESTREPLY_H
#define LINTEL_MESSAGEQUEUEREQUESTREPLY_H

#include "lintel/core/models/ProcessName.h"
#include "lintel/features/base/msg/CorrelationId.h"
#include "lintel/features/base/msg/MessageQueue.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace lintel {
/**
 * Header that precedes every request or response payload. Its fixed size keeps
 * the envelope trivially copyable, which MessageQueue<T> needs to transfer the
 * raw object representation.
 */
struct RequestReplyHeader
{
  /** Correlation id of the request/response pair, NUL terminated within the buffer. */
  char correlationId[CorrelationId::SIZE]{};
};

/**
 * Envelope of a request or response: correlation id header plus typed payload.
 * @tparam TPayload request or response type, has to be trivially copyable and default constructible
 */
template<typename TPayload> struct RequestReplyEnvelope
{
  static_assert(std::is_trivially_copyable_v<TPayload>, "payload has to be trivially copyable");
  static_assert(std::is_default_constructible_v<TPayload>, "payload has to be default constructible");

  /** Header carrying the correlation id. */
  RequestReplyHeader header{};

  /** Typed payload transferred with the envelope. */
  TPayload payload{};
};

/**
 * Higher level request/reply API on top of two MessageQueue instances:
 * - the request queue carries RequestReplyEnvelope<TReq> from caller to handler
 * - the reply queue carries ResponseEnvelope<TResp> back, matched by correlation id
 *
 * call() is synchronous: it registers a freshly generated correlation id in the
 * pending map, sends the request and pumps the reply queue until its response
 * is routed or the timeout expires. Responses of other in-flight calls that are
 * received while pumping are parked in the pending map, so several threads may
 * call() on the same endpoint concurrently without losing a response.
 *
 * serveOnce()/serve() run the handler side: the loop receives a request,
 * invokes the registered handler and sends the response carrying the same
 * correlation id back.
 *
 * Both payload types travel as raw bytes (MessageQueue transfers the object
 * representation), hence the trivially copyable constraint above.
 *
 * @tparam TReq request payload type
 * @tparam TResp response payload type
 */
template<typename TReq, typename TResp> class MessageQueueRequestReply
{
public:
  /** Handler signature: the request plus its correlation id in, the response out. */
  using RequestHandler = std::function<TResp(const TReq &, const std::string &)>;

  /** Envelope travelling from caller to handler. */
  using RequestEnvelope = RequestReplyEnvelope<TReq>;

  /** Envelope travelling from handler back to caller. */
  using ResponseEnvelope = RequestReplyEnvelope<TResp>;

  /**
   * remove of both message queues (also drops messages still in flight)
   *
   * @param requestQueueName queue carrying requests
   * @param replyQueueName queue carrying responses
   */
  static void removeOf(const std::string &requestQueueName, const std::string &replyQueueName)
  {
    MessageQueue<RequestEnvelope>::removeOf(requestQueueName);
    MessageQueue<ResponseEnvelope>::removeOf(replyQueueName);
  }

  /**
   * open or create both queues of this endpoint
   *
   * @param processName name of the owning process
   * @param requestQueueName queue carrying requests, caller -> handler
   * @param replyQueueName queue carrying responses, handler -> caller
   * @param msgCount limit of each queue
   * @param realProcessName shared handle used to resolve the process name
   */
  MessageQueueRequestReply(std::string processName,
    std::string requestQueueName,
    std::string replyQueueName,
    int32_t msgCount,
    std::shared_ptr<ProcessName> realProcessName)
    : m_requestQueue(processName, requestQueueName, msgCount, realProcessName),
      m_replyQueue(std::move(processName), std::move(replyQueueName), msgCount, std::move(realProcessName))
  {}

  /**
   * register the handler invoked by serveOnce()/serve(). May be called while a
   * serve loop is running, the next received request picks up the new handler.
   *
   * @param handler takes the request and the correlation id of the request
   */
  void registerHandler(RequestHandler handler)
  {
    std::lock_guard<std::mutex> lock(m_handlerMutex);
    m_handler = std::move(handler);
  }

  /**
   * Send a request and wait synchronously for the response with the same
   * correlation id. The reply queue is pumped while waiting, so responses that
   * belong to concurrent calls are routed to their pending entries instead of
   * being lost.
   *
   * @param request typed request payload
   * @param timeout budget for sending plus waiting for the response
   * @return the matching response, nullopt on timeout or when the request could not be queued
   */
  std::optional<TResp> call(const TReq &request, const std::chrono::milliseconds &timeout)
  {
    const std::string correlationId = CorrelationId::generate();
    RequestEnvelope envelope{};
    writeCorrelationId(envelope.header.correlationId, correlationId);
    envelope.payload = request;

    {
      std::lock_guard<std::mutex> lock(m_pendingMutex);
      m_pending.emplace(correlationId, std::nullopt);
    }

    if (!m_requestQueue.sendOfWithTimeout(envelope, timeout)) {
      forgetPendingOf(correlationId);
      return std::nullopt;
    }

    const auto deadline = std::chrono::steady_clock::now() + timeout;
    for (;;) {
      if (std::optional<TResp> response = takeResponseOf(correlationId)) { return response; }

      const auto remaining =
        std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
      if (remaining.count() <= 0) { break; }

      // The receive slice bounds how long a response routed by another thread stays unnoticed.
      const std::optional<ResponseEnvelope> response =
        m_replyQueue.receiveOfWithTimeout(std::min(remaining, kPollInterval));
      if (response.has_value()) { routeResponseOf(*response); }
    }

    if (std::optional<TResp> response = takeResponseOf(correlationId)) { return response; }
    forgetPendingOf(correlationId);
    return std::nullopt;
  }

  /**
   * Wait up to timeout for the next request and dispatch it to the registered
   * handler, sending the response with the same correlation id back. A request
   * without registered handler (or one whose handler throws) is dropped and
   * counted in droppedRequestsOf(), its caller then sees its own timeout.
   *
   * @param timeout maximum wait for a request, zero polls the queue once without blocking
   * @return true if a request was received
   */
  bool serveOnce(const std::chrono::milliseconds &timeout)
  {
    std::optional<RequestEnvelope> request = timeout > std::chrono::milliseconds::zero()
                                               ? m_requestQueue.receiveOfWithTimeout(timeout)
                                               : m_requestQueue.tryReceiveOf();
    if (!request.has_value()) { return false; }

    const std::string correlationId = readCorrelationId(request->header.correlationId);
    RequestHandler handler;
    {
      std::lock_guard<std::mutex> lock(m_handlerMutex);
      handler = m_handler;
    }

    if (!handler) {
      m_droppedRequests.fetch_add(1u, std::memory_order_relaxed);
      return true;
    }

    try {
      ResponseEnvelope response{};
      writeCorrelationId(response.header.correlationId, correlationId);
      response.payload = handler(request->payload, correlationId);
      // bounded: a reply queue that stays full must not stall the serve loop forever
      if (!m_replyQueue.sendOfWithTimeout(response, kReplySendTimeout)) {
        m_droppedRequests.fetch_add(1u, std::memory_order_relaxed);
      }
    } catch (...) {
      // a failing handler must not take the serve loop down: without a response
      // the caller times out and the request counts as dropped
      m_droppedRequests.fetch_add(1u, std::memory_order_relaxed);
    }
    return true;
  }

  /**
   * Serve until no request arrives for idleTimeout or stopServing() is called.
   * Receive attempts are capped at kPollInterval so the stop flag is noticed
   * quickly even with a long idle timeout.
   *
   * @param idleTimeout time without a request after which the loop returns
   * @return number of requests received, including dropped ones
   */
  std::size_t serve(const std::chrono::milliseconds &idleTimeout)
  {
    m_serving.store(true, std::memory_order_relaxed);
    std::size_t handled = 0;
    auto idleDeadline = std::chrono::steady_clock::now() + idleTimeout;
    while (m_serving.load(std::memory_order_relaxed)) {
      const auto now = std::chrono::steady_clock::now();
      if (now >= idleDeadline) { break; }

      const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(idleDeadline - now);
      if (serveOnce(std::min(remaining, kPollInterval))) {
        ++handled;
        idleDeadline = std::chrono::steady_clock::now() + idleTimeout;
      }
    }
    return handled;
  }

  /**
   * Signal a running serve() loop to return; the loop notices the flag within
   * kPollInterval.
   */
  void stopServing() { m_serving.store(false, std::memory_order_relaxed); }

  /**
   * @return number of requests that were received without a registered handler,
   *         whose handler threw, or whose response could not be queued
   */
  [[nodiscard]] std::uint64_t droppedRequestsOf() const { return m_droppedRequests.load(std::memory_order_relaxed); }

  /** @return number of calls that currently wait for their response */
  [[nodiscard]] std::size_t pendingRepliesOf()
  {
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    return m_pending.size();
  }

private:
  /** receive slice while waiting: bounds the stop latency of serve() and cross-thread routing latency */
  static constexpr std::chrono::milliseconds kPollInterval{ 50 };

  /** bounded wait for the response send: a full reply queue must not stall the serve loop */
  static constexpr std::chrono::milliseconds kReplySendTimeout{ 1000 };

  MessageQueue<RequestEnvelope> m_requestQueue;
  MessageQueue<ResponseEnvelope> m_replyQueue;

  mutable std::mutex m_handlerMutex;
  RequestHandler m_handler;

  std::mutex m_pendingMutex;
  std::map<std::string, std::optional<TResp>> m_pending;

  std::atomic<bool> m_serving{ true };
  std::atomic<std::uint64_t> m_droppedRequests{ 0 };

  /** copy correlationId into the fixed size buffer, NUL terminated and truncated oversized ids */
  static void writeCorrelationId(char (&buffer)[CorrelationId::SIZE], const std::string &correlationId)
  {
    const std::size_t length = std::min(correlationId.size(), CorrelationId::SIZE - 1);
    std::copy_n(correlationId.begin(), length, buffer);
    buffer[length] = '\0';
  }

  /**
   * @param buffer fixed size correlation id buffer of a received envelope
   * @return correlation id up to the first NUL character of the buffer
   */
  static std::string readCorrelationId(const char *buffer)
  {
    const char *end = buffer;
    while (end < buffer + CorrelationId::SIZE && *end != '\0') { ++end; }
    return std::string(buffer, end);
  }

  /**
   * Park a received response in the pending map of its correlation id.
   * Responses without pending call (already timed out or unknown) are late and ignored.
   */
  void routeResponseOf(const ResponseEnvelope &response)
  {
    const std::string correlationId = readCorrelationId(response.header.correlationId);
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    auto pending = m_pending.find(correlationId);
    if (pending == m_pending.end() || pending->second.has_value()) { return; }
    pending->second = response.payload;
  }

  /**
   * @return the response routed for correlationId and forget its pending entry,
   *         nullopt while the response is still outstanding (entry stays)
   */
  std::optional<TResp> takeResponseOf(const std::string &correlationId)
  {
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    auto pending = m_pending.find(correlationId);
    if (pending == m_pending.end() || !pending->second.has_value()) { return std::nullopt; }
    std::optional<TResp> response = std::move(pending->second);
    m_pending.erase(pending);
    return response;
  }

  /** drop the pending entry of a timed out or rejected call */
  void forgetPendingOf(const std::string &correlationId)
  {
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    m_pending.erase(correlationId);
  }
};

}// namespace lintel
#endif
