#include "base_library/core/persistence/sqlite3/Notify.h"

#include <utility>

namespace sqlite {
Notify::Notify(Connection &connection, std::function<void()> functionCallBack,
               std::string tableName)
    : tableName(std::move(tableName)),
      functionCallBack(std::move(functionCallBack)), connection(connection) {
  void *thisPtr = static_cast<void *>(this);
  sqlite3_update_hook(this->connection.db, callBack, thisPtr);
}

void Notify::notify() { functionCallBack(); }

void Notify::callBack(void *arg, int operation, const char *thread,
                      const char *tableName, sqlite3_int64 changes) {
  auto *notify = static_cast<Notify *>(arg);
  notify->notify();
}
} // namespace sqlite