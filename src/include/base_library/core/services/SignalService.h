#ifndef CPP_BASE_LIBRARY_SIGNALSERVICE_H
#define CPP_BASE_LIBRARY_SIGNALSERVICE_H

#include <condition_variable>
#include <csignal>
#include <mutex>
#include <vector>

#include "base_library/core/services/LoggerService.h"

static std::condition_variable m_condition;
static std::mutex m_mutex;

class SignalService {
 public:
  static void registerHooks(const std::vector<int32_t> &signals) {
    for (const auto &sig : signals) {
      signal(sig, handleSignal);
      LOG_TRACE("received {} signal", sig);
    }
  }

  static void handleSignal(int signal) { m_condition.notify_all(); }

  static void waitForUserInterrupt() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock);
    LOG_TRACE("triggered condition");
  }

  static void raiseSignal(int32_t signal) {
    raise(signal);
    LOG_TRACE("raise {} signal", signal);
  }

  static void kill(pid_t pid, int32_t signal) {
    ::kill(pid, signal);
  }
};

#endif  // CPP_BASE_LIBRARY_SIGNALSERVICE_H
