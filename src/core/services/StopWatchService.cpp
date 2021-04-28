#include "base_library/core/services/StopWatchService.h"

StopWatchService::StopWatchService(bool run) : run(run) {
  if (run) {
    reset();
  }
}

void StopWatchService::reset() {
  run = true;
  startTime = std::chrono::steady_clock::now();
  stopVar = false;
}

void StopWatchService::stop() {
  if (stopVar) {
    return;
  }
  endTime = std::chrono::steady_clock::now();
  stopVar = true;
  run = false;
}

std::chrono::nanoseconds StopWatchService::elapsed() const {
  if (stopVar) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(endTime -
                                                                startTime);
  }
  if (!run) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(startTime -
                                                                startTime);
  }
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now() - startTime);
}
std::ostream &operator<<(std::ostream &os, const StopWatchService &service) {
  return os << static_cast<double>(service.elapsed().count()) / 1000000.0
            << " ms";
}
