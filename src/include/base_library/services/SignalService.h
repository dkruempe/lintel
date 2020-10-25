#ifndef CPP_BASE_LIBRARY_SIGNALSERVICE_H
#define CPP_BASE_LIBRARY_SIGNALSERVICE_H

#include <condition_variable>
#include <csignal>
#include <mutex>
#include <vector>

static std::condition_variable condition;
static std::mutex mutex;

class SignalService {
public:
  static void registerHooks(const std::vector<int32_t> &signals) {
    for (const auto &sig : signals) {
      signal(sig, handleSignal);
    }
  }

  static void handleSignal(int signal) { condition.notify_all(); }

  static void waitForUserInterrupt() {
    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock);
  }
};

#endif // CPP_BASE_LIBRARY_SIGNALSERVICE_H
