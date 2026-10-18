/**
 * SQLite connection setup.
 *
 * Both constructors apply a small set of PRAGMAs after a successful `sqlite3_open`:
 * - `PRAGMA busy_timeout = 5000`: a second writer waits up to 5 seconds for the lock
 *   instead of failing immediately with SQLITE_BUSY.
 * - `PRAGMA journal_mode = WAL` (file databases only, not `:memory:`): readers do not
 *   block while a writer holds the write lock and vice versa. WAL is persistent in the
 *   database file, so existing databases keep the mode after reopening.
 * - `PRAGMA foreign_keys = ON`: FK constraints are enforced (SQLite defaults to off).
 */

#include "lintel/core/persistence/sqlite3/Connection.h"

#include <functional>
#include <regex>

#include "lintel/core/configuration/DatabaseConnectionEntry.h"
#include "lintel/core/exceptions/SQLException.h"
#include "lintel/core/utils/RegexUtils.h"

namespace lintel {
namespace {

  /// Wait time in ms before SQLite reports SQLITE_BUSY on a locked database.
  constexpr int kBusyTimeoutMs = 5000;

  /// In-memory databases (and empty paths) must not switch to WAL.
  constexpr const char *kInMemoryDatabase = ":memory:";

  void execPragma(sqlite3 *db, const std::string &pragma)
  {
    char *errorMessage = nullptr;
    const int rc = sqlite3_exec(db, pragma.c_str(), nullptr, nullptr, &errorMessage);
    if (rc != SQLITE_OK) {
      std::string const msg = (errorMessage != nullptr) ? errorMessage : sqlite3_errmsg(db);
      sqlite3_free(errorMessage);
      throw db::SQLException("SQLite PRAGMA exception (" + pragma + "): " + msg);
    }
  }

  [[nodiscard]] bool isFileDatabase(const std::string &connectionInfo)
  {
    return !connectionInfo.empty() && connectionInfo != kInMemoryDatabase;
  }

  void regexpFunc(sqlite3_context *ctx, int argc, sqlite3_value **argv)
  {
    if (argc != 2) {
      sqlite3_result_error(ctx, "REGEXP requires exactly 2 arguments", -1);
      return;
    }
    const char *pattern = reinterpret_cast<const char *>(sqlite3_value_text(argv[0]));
    const char *text = reinterpret_cast<const char *>(sqlite3_value_text(argv[1]));
    if (pattern == nullptr || text == nullptr) {
      sqlite3_result_int(ctx, 0);
      return;
    }
    try {
      std::string errorMessage;
      if (!RegexUtils::validatePattern(pattern, errorMessage)) {
        sqlite3_result_error(ctx, errorMessage.c_str(), -1);
        return;
      }
      std::regex re(pattern, std::regex::ECMAScript | std::regex::icase);
      sqlite3_result_int(ctx, std::regex_match(text, re) ? 1 : 0);
    } catch (const std::regex_error &) {
      sqlite3_result_error(ctx, "invalid REGEXP pattern", -1);
    }
  }

  void registerRegexp(sqlite3 *db)
  {
    sqlite3_create_function(db, "REGEXP", 2, SQLITE_UTF8, nullptr, regexpFunc, nullptr, nullptr);
  }

}// namespace

namespace sqlite {
  Connection::Connection(const std::string &connectionInfo) : m_db(nullptr)
  {
    int const rc = sqlite3_open(connectionInfo.c_str(), &m_db);
    if (rc != SQLITE_OK) {
      std::string const msg = getErrorMessage();
      sqlite3_close(m_db);
      m_db = nullptr;
      throw db::SQLException("Can't open database: " + msg);
    }
    registerRegexp(m_db);
    applyPragmas(connectionInfo);
  }

  Connection::Connection(const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry) : m_db(nullptr)
  {
    int const rc = sqlite3_open(connectionEntry->getConnection().c_str(), &m_db);
    if (rc != SQLITE_OK) {
      std::string const msg = getErrorMessage();
      sqlite3_close(m_db);
      m_db = nullptr;
      throw db::SQLException("Can't open database: " + msg);
    }
    registerRegexp(m_db);
    applyPragmas(connectionEntry->getConnection());
  }

  void Connection::applyPragmas(const std::string &connectionInfo)
  {
    // wait for the lock instead of failing immediately with SQLITE_BUSY
    execPragma(m_db, "PRAGMA busy_timeout = " + std::to_string(kBusyTimeoutMs));
    if (isFileDatabase(connectionInfo)) {
      // readers do not block during a write and vice versa; persisted in the DB file
      execPragma(m_db, "PRAGMA journal_mode = WAL");
    }
    // FK constraints are off by default in SQLite
    execPragma(m_db, "PRAGMA foreign_keys = ON");
  }

  std::string Connection::getErrorMessage() const { return sqlite3_errmsg(m_db); }

  bool Connection::isHealthy() const
  {
    if (m_db == nullptr) { return false; }
    return sqlite3_exec(m_db, "SELECT 1", nullptr, nullptr, nullptr) == SQLITE_OK;
  }

  Connection::~Connection()
  {
    if (!m_preparedStatements.empty()) {
      for (auto &[queryName, stmt] : m_preparedStatements) { sqlite3_finalize(stmt); }
      m_preparedStatements.clear();
    }

    if (m_db != nullptr) {
      // finalize first, otherwise sqlite3_close fails with SQLITE_BUSY
      static_cast<void>(sqlite3_close(m_db));
      m_db = nullptr;
    }
  }

  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) — distinct names (queryName, query); signature is public API.
  std::shared_ptr<Result> Connection::prepareStatement(const std::string &queryName, const std::string &query)
  {
    auto found = m_preparedStatements.find(queryName);
    if (found != m_preparedStatements.end()) { throw db::SQLException("SQLite sqlite3_stmt is initialized => abort"); }

    sqlite3_stmt *stmt = nullptr;
    const int rc = sqlite3_prepare_v2(m_db, query.c_str(), static_cast<int>(query.size() + 1), &stmt, nullptr);

    if (rc != SQLITE_OK) { throw db::SQLException("SQLite prepare exception: " + getErrorMessage()); }

    m_preparedStatements.insert({ queryName, stmt });

    return std::make_shared<Result>();
  }

  void Connection::finalizePreparedStatement(const std::string &queryName)
  {
    auto found = m_preparedStatements.find(queryName);
    if (found == m_preparedStatements.end()) {
      // no need to finalize
      return;
    }

    sqlite3_finalize(found->second);
    m_preparedStatements.erase(found);
  }

  std::shared_ptr<Result> Connection::executePreparedStatement(const std::string &queryName,
    const db::Parameters &parameters)
  {
    auto found = m_preparedStatements.find(queryName);
    if (found == m_preparedStatements.end()) { throw db::SQLException("SQLite not prepared statement available"); }

    sqlite3_reset(found->second);
    sqlite3_clear_bindings(found->second);
    int counter = 0;
    for (const auto &param : parameters.getParameters()) {
      int const bindRc = sqlite3_bind_text(found->second, ++counter, param, -1, SQLITE_TRANSIENT);
      if (bindRc != SQLITE_OK) { throw db::SQLException("SQLite bind exception: " + getErrorMessage()); }
    }
    std::shared_ptr<Result> result = std::make_shared<Result>();
    int step = -1;
    do {
      step = sqlite3_step(found->second);
      if (step != SQLITE_ROW) {
        if (step == SQLITE_ERROR || step == SQLITE_MISUSE) {
          throw db::SQLException("SQLite step exception: " + getErrorMessage());
        }
        continue;
      }
      const unsigned char *text = nullptr;
      const int count = sqlite3_column_count(found->second);
      db::Arguments arguments(db::ConnectionType::SQLite);
      for (int i = 0; i < count; i++) {
        text = sqlite3_column_text(found->second, i);
        const char *name = sqlite3_column_name(found->second, i);
        std::string value;
        if (text != nullptr) { value = std::string(reinterpret_cast<const char *>(text)); }
        if (name == nullptr) { continue; }
        db::Argument argument(value, name);
        arguments.add(argument);
      }
      result->add(arguments);
    } while (step == SQLITE_ROW);
    sqlite3_reset(found->second);
    return result;
  }

  std::shared_ptr<Result> Connection::executeParameters(const std::string &statement, const db::Parameters &parameters)
  {
    std::shared_ptr<Result> result = std::make_shared<Result>();
    sqlite3_stmt *stmt = nullptr;
    const int rc = sqlite3_prepare_v2(m_db, statement.c_str(), static_cast<int>(statement.size() + 1), &stmt, nullptr);
    if (rc != SQLITE_OK) { throw db::SQLException("SQLite prepare exception: " + getErrorMessage()); }

    int i = 0;
    for (auto param : parameters.getParameters()) {
      int const bindRc = sqlite3_bind_text(stmt, ++i, param, -1, SQLITE_TRANSIENT);
      if (bindRc != SQLITE_OK) {
        sqlite3_finalize(stmt);
        throw db::SQLException("SQLite bind exception: " + getErrorMessage());
      }
    }

    int step = -1;
    do {
      step = sqlite3_step(stmt);
      if (step != SQLITE_ROW) {
        if (step == SQLITE_ERROR || step == SQLITE_MISUSE) {
          sqlite3_finalize(stmt);
          throw db::SQLException("SQLite step exception: " + getErrorMessage());
        }
        continue;
      }
      const unsigned char *text = nullptr;
      const int count = sqlite3_column_count(stmt);
      db::Arguments arguments(db::ConnectionType::SQLite);
      for (int j = 0; j < count; j++) {
        text = sqlite3_column_text(stmt, j);
        const char *name = sqlite3_column_name(stmt, j);
        std::string value;
        if (text != nullptr) { value = std::string(reinterpret_cast<const char *>(text)); }
        if (name == nullptr) { continue; }
        db::Argument argument(value, name);
        arguments.add(argument);
      }
      result->add(arguments);
    } while (step == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return result;
  }

  std::shared_ptr<Result> Connection::execute(const std::string &statement) const
  {
    char *errorMessage = nullptr;
    std::shared_ptr<Result> result = std::make_shared<Result>();
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) — sqlite3_exec callback signature is fixed by the C API.
    std::function<void(int argc, char **argv, char **column)> func = [&](int argc, char **argv, char **column) {
      db::Arguments arguments(db::ConnectionType::SQLite);
      for (int i = 0; i < argc; i++) {
        std::string value;
        if (argv[i] != nullptr) { value = argv[i]; }
        db::Argument argument(value, column[i]);
        arguments.add(argument);
      }
      result->add(arguments);
    };
    void *funcPtr = static_cast<void *>(&func);
    const int rc = sqlite3_exec(m_db, statement.c_str(), callBack, funcPtr, &errorMessage);

    if (rc != SQLITE_OK) {
      std::string const errMsg = (errorMessage);
      sqlite3_free(errorMessage);
      throw db::SQLException("SQLite execution error: " + errMsg);
    }
    return result;
  }

  int Connection::callBack(void *funcPtr, int argc, char **argv, char **column)
  {
    auto *func = static_cast<std::function<void(int argc, char **argv, char **column)> *>(funcPtr);
    func->operator()(argc, argv, column);
    return 0;
  }
}// namespace sqlite
}// namespace lintel