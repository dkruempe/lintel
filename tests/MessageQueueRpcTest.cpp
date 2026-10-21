#include <catch2/catch_all.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "helpers/PlatformCompat.h"

#include "lintel/core/models/ProcessName.h"
#include "lintel/features/base/msg/MessageQueueRequestReply.h"

namespace lintel {
namespace {
  struct RpcAddRequest
  {
    int32_t lhs{};
    int32_t rhs{};
  };

  struct RpcAddResponse
  {
    int32_t sum{};
  };

  using RpcEndpoint = MessageQueueRequestReply<RpcAddRequest, RpcAddResponse>;

  std::string rpcUniqueQueueName(const std::string &suffix)
  {
    static int counter = 0;
    // ctest gives every test case its own process, so the shared memory name needs the PID:
    // without it the cases attach to each other's queue as soon as ctest runs them in parallel.
    return "opencode_rpc_test_" + std::to_string(::getpid()) + "_" + std::to_string(++counter) + "_" + suffix;
  }

  /** Drops both shared memory queues on every exit path, also when a failing REQUIRE aborts the case */
  class RpcQueueCleanup
  {
  public:
    RpcQueueCleanup(std::string requestQueueName, std::string replyQueueName)
      : m_requestQueueName(std::move(requestQueueName)), m_replyQueueName(std::move(replyQueueName))
    {
      RpcEndpoint::removeOf(RequestReplyQueues{ m_requestQueueName, m_replyQueueName });
    }

    ~RpcQueueCleanup() { RpcEndpoint::removeOf(RequestReplyQueues{ m_requestQueueName, m_replyQueueName }); }

    RpcQueueCleanup(const RpcQueueCleanup &) = delete;
    RpcQueueCleanup &operator=(const RpcQueueCleanup &) = delete;

  private:
    std::string m_requestQueueName;
    std::string m_replyQueueName;
  };

  /** Opens the endpoint under test on uniquely named queues. */
  std::unique_ptr<RpcEndpoint> rpcEndpointOf(const std::string &requestQueueName, const std::string &replyQueueName)
  {
    auto processName = std::make_shared<ProcessName>("rpcTest");
    return std::make_unique<RpcEndpoint>(
      "rpcTest", RequestReplyQueues{ requestQueueName, replyQueueName }, 8, processName);
  }
}// namespace

TEST_CASE("CorrelationId: generate yields unique non empty ids")
{
  const std::string first = CorrelationId::generate();
  const std::string second = CorrelationId::generate();

  CHECK_FALSE(first.empty());
  CHECK_FALSE(second.empty());
  CHECK(first != second);
  // has to fit the fixed size envelope buffer including its terminating NUL
  CHECK(first.size() < CorrelationId::SIZE);
  CHECK(second.size() < CorrelationId::SIZE);
}

TEST_CASE("MessageQueueRequestReply: call returns the response of the registered handler")
{
  const std::string requestQueueName = rpcUniqueQueueName("requests");
  const std::string replyQueueName = rpcUniqueQueueName("replies");
  RpcQueueCleanup cleanup(requestQueueName, replyQueueName);
  const std::unique_ptr<RpcEndpoint> rpc = rpcEndpointOf(requestQueueName, replyQueueName);

  std::string seenCorrelationId;
  rpc->registerHandler([&seenCorrelationId](const RpcAddRequest &request, const std::string &correlationId) {
    seenCorrelationId = correlationId;
    return RpcAddResponse{ request.lhs + request.rhs };
  });

  std::thread server([&] { rpc->serve(std::chrono::seconds(10)); });

  const std::optional<RpcAddResponse> response = rpc->call(RpcAddRequest{ 20, 22 }, std::chrono::seconds(5));
  rpc->stopServing();
  server.join();

  REQUIRE(response.has_value());
  CHECK(response->sum == 42);
  CHECK_FALSE(seenCorrelationId.empty());
  CHECK(seenCorrelationId.size() < CorrelationId::SIZE);
  CHECK(rpc->pendingRepliesOf() == 0);
  CHECK(rpc->droppedRequestsOf() == 0);
}

TEST_CASE("MessageQueueRequestReply: responses are routed to the matching correlation id")
{
  const std::string requestQueueName = rpcUniqueQueueName("requests");
  const std::string replyQueueName = rpcUniqueQueueName("replies");
  RpcQueueCleanup cleanup(requestQueueName, replyQueueName);
  const std::unique_ptr<RpcEndpoint> rpc = rpcEndpointOf(requestQueueName, replyQueueName);

  rpc->registerHandler([](const RpcAddRequest &request, const std::string & /*correlationId*/) {
    return RpcAddResponse{ request.lhs + request.rhs };
  });

  std::thread server([&] { rpc->serve(std::chrono::seconds(10)); });

  std::optional<RpcAddResponse> first;
  std::optional<RpcAddResponse> second;
  // both calls are in flight at the same time: each reply has to reach the call
  // whose correlation id it carries, not just the next waiter
  std::thread firstCaller([&] { first = rpc->call(RpcAddRequest{ 10, 1 }, std::chrono::seconds(5)); });
  std::thread secondCaller([&] { second = rpc->call(RpcAddRequest{ 30, 12 }, std::chrono::seconds(5)); });
  firstCaller.join();
  secondCaller.join();
  rpc->stopServing();
  server.join();

  REQUIRE(first.has_value());
  REQUIRE(second.has_value());
  CHECK(first->sum == 11);
  CHECK(second->sum == 42);
  CHECK(rpc->pendingRepliesOf() == 0);
}

TEST_CASE("MessageQueueRequestReply: call times out when no endpoint serves the request")
{
  const std::string requestQueueName = rpcUniqueQueueName("requests");
  const std::string replyQueueName = rpcUniqueQueueName("replies");
  RpcQueueCleanup cleanup(requestQueueName, replyQueueName);
  const std::unique_ptr<RpcEndpoint> rpc = rpcEndpointOf(requestQueueName, replyQueueName);

  const std::optional<RpcAddResponse> response = rpc->call(RpcAddRequest{ 1, 1 }, std::chrono::milliseconds(100));

  REQUIRE_FALSE(response.has_value());
  CHECK(rpc->pendingRepliesOf() == 0);
  CHECK(rpc->droppedRequestsOf() == 0);
}

TEST_CASE("MessageQueueRequestReply: request without registered handler is dropped and counted")
{
  const std::string requestQueueName = rpcUniqueQueueName("requests");
  const std::string replyQueueName = rpcUniqueQueueName("replies");
  RpcQueueCleanup cleanup(requestQueueName, replyQueueName);
  const std::unique_ptr<RpcEndpoint> rpc = rpcEndpointOf(requestQueueName, replyQueueName);

  std::optional<RpcAddResponse> response;
  std::thread caller([&] { response = rpc->call(RpcAddRequest{ 1, 2 }, std::chrono::milliseconds(500)); });

  // no handler registered: the request is received (and dropped), never answered
  REQUIRE(rpc->serveOnce(std::chrono::seconds(5)));
  CHECK(rpc->droppedRequestsOf() == 1);
  caller.join();

  REQUIRE_FALSE(response.has_value());
  CHECK(rpc->pendingRepliesOf() == 0);
}
}// namespace lintel
