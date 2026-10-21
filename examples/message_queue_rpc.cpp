#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>

#include "lintel/core/models/ProcessName.h"
#include "lintel/features/base/msg/MessageQueueRequestReply.h"

namespace {
/** Typed request payload: the two operands to add. */
struct AddRequest
{
  int32_t lhs{};
  int32_t rhs{};
};

/** Typed response payload: the sum of both operands. */
struct AddResponse
{
  int32_t sum{};
};
}// namespace

/**
 * Request/Reply (RPC) example over the lintel message queues:
 * - one endpoint configured with a request queue and a reply queue
 * - a handler registered on the server side, running in a serve() loop
 * - typed call() invocations from the client side, each carrying a correlation
 *   id that routes the response back to the matching call
 */
int main(int /*argc*/, char ** /*argv[]*/)
{
  try {
    using Rpc = lintel::MessageQueueRequestReply<AddRequest, AddResponse>;

    constexpr const char *requestQueueName = "rpc_example_requests";
    constexpr const char *replyQueueName = "rpc_example_replies";

    // start clean: queues of an earlier run would still carry their messages
    Rpc::removeOf(requestQueueName, replyQueueName);

    auto processName = std::make_shared<lintel::ProcessName>("rpc_example");
    Rpc rpc("rpc_example", requestQueueName, replyQueueName, 8, processName);

    rpc.registerHandler([](const AddRequest &request, const std::string &correlationId) {
      std::cout << "server: handling request correlation_id=" << correlationId << "\n";
      return AddResponse{ request.lhs + request.rhs };
    });

    // the serve loop runs in the background until stopServing() is called
    std::thread server([&rpc] { rpc.serve(std::chrono::seconds(30)); });

    for (const AddRequest &request : { AddRequest{ 2, 3 }, AddRequest{ 40, 2 } }) {
      const std::optional<AddResponse> response = rpc.call(request, std::chrono::milliseconds(2000));
      if (!response.has_value()) {
        std::cerr << "request timed out\n";
        rpc.stopServing();
        server.join();
        Rpc::removeOf(requestQueueName, replyQueueName);
        return 1;
      }
      std::cout << request.lhs << " + " << request.rhs << " = " << response->sum << "\n";
    }

    rpc.stopServing();
    server.join();
    std::cout << "requests dropped by the server: " << rpc.droppedRequestsOf() << "\n";

    Rpc::removeOf(requestQueueName, replyQueueName);
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << "\n";
    return 1;
  }
  return 0;
}
