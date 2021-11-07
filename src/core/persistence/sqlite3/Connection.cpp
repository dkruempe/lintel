#include "base_library/core/persistence/sqlite3/Connection.h"

#include <cstring>
#include <functional>

#include "base_library/core/exceptions/SQLException.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"

namespace sqlite {
Connection::Connection(const std::string &connectionInfo) : m_db(nullptr) {
  int rc = sqlite3_open(connectionInfo.c_str(), &m_db);
  if (rc != SQLITE_OK) {
    throw db::SQLException("Can't open database: " + getErrorMessage());
  }
}
Connection::Connection(
    const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry)
    : m_db(nullptr) {
  int rc = sqlite3_open(connectionEntry->getConnection().c_str(), &m_db);
  if (rc != SQLITE_OK) {
    throw db::SQLException("Can't open database: " + getErrorMessage());
  }
}
std::string Connection::getErrorMessage() const { return sqlite3_errmsg(m_db); }
Connection::~Connection() {
  if (m_db != nullptr) {
    sqlite3_close(m_db);
    m_db = nullptr;
  }

  if (!m_preparedStatements.empty()) {
    for (auto &[queryName, stmt] : m_preparedStatements) {
      sqlite3_finalize(stmt);
    }
  }
}

std::shared_ptr<Result> Connection::prepareStatement(
    const std::string &queryName, const std::string &query) {
  auto found = m_preparedStatements.find(queryName);
  if (found != m_preparedStatements.end()) {
    throw db::SQLException("SQLite sqlite3_stmt is initialized => abort");
  }

  sqlite3_stmt *stmt;
  const int rc = sqlite3_prepare_v2(
      m_db, query.c_str(), static_cast<int>(query.size() + 1), &stmt, nullptr);

  if (rc != SQLITE_OK) {
    throw db::SQLException("SQLite prepare exception: " + getErrorMessage());
  }

  m_preparedStatements.insert({queryName, stmt});

  return std::make_shared<Result>();
}
void Connection::finalizePreparedStatement(const std::string &queryName) {
  auto found = m_preparedStatements.find(queryName);
  if (found == m_preparedStatements.end()) {
    // no need to finalize
    return;
  }

  sqlite3_finalize(found->second);
  m_preparedStatements.erase(found);
}
std::shared_ptr<Result> Connection::executePreparedStatement(
    const std::string &queryName, const db::Parameters &parameters) {
  auto found = m_preparedStatements.find(queryName);
  if (found == m_preparedStatements.end()) {
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
    const unsigned char *text;
    const char *name;
    const int count = sqlite3_column_count(found->second);
    db::Arguments arguments(db::ConnectionType::SQLite);
    for (int i = 0; i < count; i++) {
      text = sqlite3_column_text(found->second, i);
      name = sqlite3_column_name(found->second, i);
      std::basic_string<unsigned char> temp = text;
      std::string value(temp.begin(), temp.end());
      db::Argument argument(value, name);
      arguments.add(argument);
    }
    result->add(arguments);
    row++;
  } while (step == SQLITE_ROW);
  sqlite3_reset(found->second);
  return result;
}

std::shared_ptr<Result> Connection::executeParameters(
    const std::string &statement, const db::Parameters &parameters) {
  std::shared_ptr<Result> result = std::make_shared<Result>();
  sqlite3_stmt *stmt;
  const int rc = sqlite3_prepare_v2(m_db, statement.c_str(),
                                    static_cast<int>(statement.size() + 1),
                                    &stmt, nullptr);
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
    const unsigned char *text;
    const char *name;
    const int count = sqlite3_column_count(stmt);
    db::Arguments arguments(db::ConnectionType::SQLite);
    for (int j = 0; j < count; j++) {
      text = sqlite3_column_text(stmt, j);
      name = sqlite3_column_name(stmt, j);
      std::string value;
      if (text != nullptr) {
        std::basic_string<unsigned char> temp = text;
        value = std::string(temp.begin(), temp.end());
      }
      if (name == nullptr) {
        continue;
      }
      db::Argument argument(value, name);
      arguments.add(argument);
    }
    result->add(arguments);
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
        db::Arguments arguments(db::ConnectionType::SQLite);
        for (int i = 0; i < argc; i++) {
          std::string value;
          if (argv[i] != nullptr) {
            value = argv[i];
          }
          db::Argument argument(value, column[i]);
          arguments.add(argument);
        }
        result->add(arguments);
      };
  void *funcPtr = static_cast<void *>(&func);
  const int rc =
      sqlite3_exec(m_db, statement.c_str(), callBack, funcPtr, &errorMessage);

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