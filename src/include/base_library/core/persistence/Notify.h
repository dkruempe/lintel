#ifndef CPP_BASE_LIBRARY_NOTIFY_H
#define CPP_BASE_LIBRARY_NOTIFY_H

#include <memory>

#include "Connection.h"
#include "base_library/core/persistence/postgresql/Notify.h"
#include "base_library/core/persistence/sqlite3/Notify.h"

namespace db {
class Notify {
 private:
  Connection &connection;
  std::unique_ptr<sqlite::Notify> notifySqlite = nullptr;
  std::unique_ptr<postgresql::Notify> notifyPostgresql = nullptr;

 public:
  explicit Notify(Connection &connection,
                  std::function<void()> functionCallBack,
                  std::string tableName);
};
}  // namespace db

#endif  // CPP_BASE_LIBRARY_NOTIFY_H
