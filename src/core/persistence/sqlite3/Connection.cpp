#include "base_library/core/persistence/sqlite3/Connection.h"

#include <cstring>
#include <functional>

#include "base_library/core/exceptions/SQLException.h"
#include "base_library/features/base/configuration/ConnectionEntry.h"

namespace sqlite {
Connection::Connection(const std::string &connectionInfo) : db(nullptr) {
  int rc = sqlite3_open(connectionInfo.c_str(), &db);
  if (rc != SQLITE_OK) {
    throw db::SQLException("Can't open database: " + getErrorMessage());
  }
}
Connection::Connection(const std::shared_ptr<ConnectionEntry> &connectionEntry)
    : db(nullptr) {
  int rc = sqlite3_open(connectionEntry->getConnection().c_str(), &db);
  if (rc != SQLITE_OK) {
    throw db::SQLException("Can't open database: " + getErrorMessage());
  }
}
std::string Connection::getErrorMessage() const { return sqlite3_errmsg(db); }
Connection::~Connection() {
  if (db != nullptr) {
    sqlite3_close(db);
    db = nullptr;
  }

  if (!preparedStatements.empty()) {
    for (auto &[queryName, stmt] : preparedStatements) {
      sqlite3_finalize(stmt);
    }
  }
}

std::shared_ptr<Result> Connection::prepareStatement(
    const std::string &queryName, const std::string &query) {
  auto found = preparedStatements.find(queryName);
  if (found != preparedStatements.end()) {
    throw db::SQLException("SQLite sqlite3_stmt is initialized => abort");
  }

  sqlite3_stmt *stmt;
  const int rc = sqlite3_prepare_v2(db, query.c_str(), (int)query.size() + 1,
                                    &stmt, nullptr);

  if (rc != SQLITE_OK) {
    throw db::SQLException("SQLite prepare exception: " + getErrorMessage());
  }

  preparedStatements.insert({queryName, stmt});

  return std::make_shared<Result>();
}
void Connection::finalizePreparedStatement(const std::string &queryName) {
  auto found = preparedStatements.find(queryName);
  if (found == preparedStatements.end()) {
    // no need to finalize
    return;
  }

  sqlite3_finalize(found->second);
  preparedStatements.erase(found);
}
std::shared_ptr<Result> Connection::executePreparedStatement(
    const std::string &queryName, const db::Parameters &parameters) {
  auto found = preparedStatements.find(queryName);
  if (found == preparedStatements.end()) {
    throw db::SQLException("SQLite not prepared statement available");
  }

  int counter = 0;
  for (auto param : parameters.getParameters()) {
    if (std::strlen(param) > 0) {
      sqlite3_bind_text(found->second, ++counter, param, -1, SQLITE_TRANSIENT);
    } else {
      sqlite3_bind_null(found->second, ++counter);
    }
  }
  std::shared_ptr<Result> result = std::make_shared<Result>();
  int step = -1;
  int row = 0;
  do {
    step = sqlite3_step(found->second);
    if (step != SQLITE_ROW) {
      continue;
    }
    int bytes;
    const unsigned char *text;
    const int count = sqlite3_column_count(found->second);
    std::vector<std::string> entry;
    for (int i = 0; i < count; i++) {
      bytes = sqlite3_column_bytes(found->second, i);
      text = sqlite3_column_text(found->second, i);
      std::basic_string<unsigned char> temp = text;
      entry.emplace_back(temp.begin(), temp.end());
    }
    result->add(entry);
    row++;
  } while (step == SQLITE_ROW);
  sqlite3_reset(found->second);
  return result;
}

std::shared_ptr<Result> Connection::executeParameters(
    const std::string &statement, const db::Parameters &parameters) {
  std::shared_ptr<Result> result = std::make_shared<Result>();
  sqlite3_stmt *stmt;
  const int rc = sqlite3_prepare_v2(db, statement.c_str(),
                                    (int)statement.size() + 1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    throw db::SQLException("SQLite prepare exception: " + getErrorMessage());
  }

  int i = 0;
  for (auto param : parameters.getParameters()) {
    if (std::strlen(param) > 0) {
      sqlite3_bind_text(stmt, ++i, param, -1, SQLITE_TRANSIENT);
    } else {
      sqlite3_bind_null(stmt, ++i);
    }
  }

  int step = -1;
  int row = 0;
  do {
    step = sqlite3_step(stmt);
    if (step != SQLITE_ROW) {
      continue;
    }
    int bytes;
    const unsigned char *text;
    const int count = sqlite3_column_count(stmt);
    std::vector<std::string> entry;
    for (int i = 0; i < count; i++) {
      bytes = sqlite3_column_bytes(stmt, i);
      text = sqlite3_column_text(stmt, i);
      std::basic_string<unsigned char> temp = text;
      entry.emplace_back(temp.begin(), temp.end());
    }
    result->add(entry);
    row++;
  } while (step == SQLITE_ROW);
  sqlite3_finalize(stmt);
  return result;
}

std::shared_ptr<Result> Connection::execute(
    const std::string &statement) const {
  char *errorMessage;
  std::shared_ptr<Result> result = std::make_shared<Result>();
  std::function<void(int argc, char **argv, char **column)> func =
      [&](int argc, char **argv, char **column) {
        std::vector<std::string> entry;
        entry.reserve(argc);
        for (int i = 0; i < argc; i++) {
          entry.emplace_back(argv[i]);
        }
        result->add(entry);
      };
  void *funcPtr = static_cast<void *>(&func);
  const int rc =
      sqlite3_exec(db, statement.c_str(), callBack, funcPtr, &errorMessage);

  if (rc != SQLITE_OK) {
    std::string errMsg = (errorMessage);
    sqlite3_free(errorMessage);
    throw db::SQLException("SQLite execution error: " + errMsg);
  }
  return result;
}

int Connection::callBack(void *funcPtr, int argc, char **argv, char **column) {
  auto *func =
      static_cast<std::function<void(int argc, char **argv, char **column)> *>(
          funcPtr);
  func->operator()(argc, argv, column);
  return 0;
}
}  // namespace sqlite