#include "base_library/persistence/postgresql/Notify.h"
#include <iostream>

namespace postgresql {
Notify::Notify(Connection &connection, std::string tableName,
               std::function<void()> &callBack)
    : connection(connection), thread([&]() { run(); }),
      tableName(std::move(tableName)), callBack(callBack) {}

void Notify::listen() {
  if (shutdown) {
    return;
  }

  auto result = connection.execute("LISTEN " + tableName);
  if (!result->isState(ExecStatusType::PGRES_COMMAND_OK)) {
    throw db::SQLException("LISTEN command failed: " +
                           connection.getErrorMessage());
  }
  int sock = PQsocket(connection.conn);

  if (sock < 0) {
    throw db::SQLException("LISTEN sock connection failed");
  }
  fd_set input_mask;
  FD_ZERO(&input_mask);
  FD_SET(sock, &input_mask);
  const int rc = select(sock + 1, &input_mask, nullptr, nullptr, &timeout);
  switch (rc) {
    // error happen
  case -1:
    throw db::SQLException("LISTEN select() failed: " +
                           std::string(strerror(errno)));
    break;
  case 0:
    // timeout
    break;
  default:
    PQconsumeInput(connection.conn);
    PGnotify *notify = nullptr;
    do {
      notify = PQnotifies(connection.conn);
      // clean received messages
      if (notify != nullptr) {
        PQfreemem(notify);
      }
    } while (notify != nullptr);
    callBack();
    // worked
    break;
  }
}
void Notify::run() {
  while (!shutdown) {
    listen();
  }
}

Notify::~Notify() {
  shutdown.store(true);
  thread.join();
}
} // namespace postgresql