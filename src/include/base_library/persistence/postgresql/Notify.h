#ifndef CPP_BASE_LIBRARY_POSTGRESQL_NOTIFY_H
#define CPP_BASE_LIBRARY_POSTGRESQL_NOTIFY_H

#include "base_library/persistence/postgresql/Connection.h"

#include <atomic>
#include <functional>
#include <string>
#include <sys/select.h>
#include <thread>

namespace postgresql {
class Notify {
private:
  const int32_t timeoutSeconds = 1;
  std::atomic<bool> shutdown = false;
  Connection &connection;
  std::thread thread;

  std::string tableName;
  std::function<void()> callBack;
  struct timeval timeout {
    .tv_sec = timeoutSeconds, .tv_usec = 0
  };

  void run();
  void listen();

public:
  explicit Notify(Connection &connection, std::string tableName,
                  std::function<void()> &callBack);

  ~Notify();
};
} // namespace postgresql

#endif // CPP_BASE_LIBRARY_POSTGRESQL_NOTIFY_H
