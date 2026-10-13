#include <benchmark/benchmark.h>

#include <chrono>
#include <memory>
#include <string>

#include <unistd.h>

#include "lintel/features/base/models/ProcessName.h"
#include "lintel/features/base/msg/Message.h"
#include "lintel/features/base/msg/MessageQueue.h"

namespace {

// The system POSIX mqueue limit (fs.mqueue.msg_max) is 10 on the CI runners, so
// the queue is deliberately small and stays below it.
constexpr int32_t kQueueCount = 8;

std::string uniqueQueueName(const std::string &suffix)
{
  static int counter = 0;
  return "lintel_bench_mq_" + std::to_string(::getpid()) + "_" + std::to_string(++counter) + "_" + suffix;
}

/** Owns the underlying shared memory queue and removes it on every exit path. */
struct QueueFixture
{
  std::string name;
  std::shared_ptr<ProcessName> processName;
  std::unique_ptr<MessageQueue<Message>> queue;

  explicit QueueFixture(const std::string &suffix)
    : name(uniqueQueueName(suffix)), processName(std::make_shared<ProcessName>("benchmark"))
  {
    MessageQueue<Message>::removeOf(name);
    queue = std::make_unique<MessageQueue<Message>>("benchmark", name, kQueueCount, processName);
  }

  ~QueueFixture()
  {
    queue.reset();
    MessageQueue<Message>::removeOf(name);
  }

  QueueFixture(const QueueFixture &) = delete;
  QueueFixture &operator=(const QueueFixture &) = delete;
};

}// namespace

static void BM_MessageQueue_TrySendReceive(benchmark::State &state)
{
  QueueFixture fixture("try");
  Message message;
  for (auto _ : state) {
    fixture.queue->trySendOf(message);
    benchmark::DoNotOptimize(fixture.queue->tryReceiveOf());
  }
}
BENCHMARK(BM_MessageQueue_TrySendReceive);

static void BM_MessageQueue_SendReceive(benchmark::State &state)
{
  QueueFixture fixture("blocking");
  Message message;
  for (auto _ : state) {
    fixture.queue->sendOf(message);
    benchmark::DoNotOptimize(fixture.queue->receiveOf());
  }
}
BENCHMARK(BM_MessageQueue_SendReceive);

static void BM_MessageQueue_SendOfWithTimeout(benchmark::State &state)
{
  QueueFixture fixture("timeout");
  Message message;
  for (auto _ : state) {
    fixture.queue->sendOfWithTimeout(message, std::chrono::milliseconds(0));
    benchmark::DoNotOptimize(fixture.queue->tryReceiveOf());
  }
}
BENCHMARK(BM_MessageQueue_SendOfWithTimeout);
