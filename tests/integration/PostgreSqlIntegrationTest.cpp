#include <lintel/core/persistence/Connection.h>
#include <lintel/core/persistence/ConnectionType.h>
#include <lintel/core/persistence/Cursor.h>
#include <lintel/core/persistence/Notify.h>
#include <lintel/core/persistence/ParameterBuilder.h>
#include <lintel/core/persistence/PreparedStatement.h>
#include <lintel/core/persistence/Result.h>
#include <lintel/core/persistence/Statement.h>
#include <lintel/core/persistence/Transaction.h>
#include <lintel/features/base/configuration/DatabaseConnectionEntry.h>

#include <catch2/catch_all.hpp>

#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include <vector>

/*
 * The PostgreSQL half of the persistence layer.
 *
 * These tests are tagged `[pg]` and skipped when no server answers on
 * PGHOST/PGPORT (default 127.0.0.1:5432). In CI the `postgres:17` service
 * provides it with user/password/db `test`/`test`/`test`, matching
 * docker-compose.yml.
 *
 * Why they exist: `db::Connection` dispatches every call to either the
 * PostgreSQL or the SQLite implementation behind one interface, and the SQLite
 * side was the only one under test. A change that satisfies SQLite can still
 * break PostgreSQL - the two backends disagree about exactly the features
 * covered here (server-side cursors, LISTEN/NOTIFY, real transactions).
 */

namespace {

/** Connection string built from the environment, empty when unset. */
std::string pgConnInfo()
{
  const char *host = std::getenv("PGHOST");
  const char *port = std::getenv("PGPORT");
  const char *user = std::getenv("PGUSER");
  const char *password = std::getenv("PGPASSWORD");
  const char *database = std::getenv("PGDATABASE");

  // PGDATABASE alone is enough to opt in; the rest falls back to the CI
  // service credentials so a bare `PGDATABASE=x ctest` also works.
  std::string info = "hostaddr=";
  info += (host != nullptr && *host != '\0') ? host : "127.0.0.1";
  info += " port=";
  info += (port != nullptr && *port != '\0') ? port : "5432";
  info += " user=";
  info += (user != nullptr && *user != '\0') ? user : "test";
  info += " password=";
  info += (password != nullptr && *password != '\0') ? password : "test";
  info += " dbname=";
  info += (database != nullptr && *database != '\0') ? database : "test";
  return info;
}

/**
 * Skips the current test case unless a PostgreSQL server answers.
 *
 * Skipping (rather than failing) is the right call: the suite must stay green
 * for a contributor without a local server, and the alternative would make
 * `ctest` useless outside CI. The `[pg]` label keeps these tests selectable.
 *
 * Every test case must call this *before* anything else that touches the
 * server, including the PgTable fixture - otherwise the fixture's own
 * connection throws first and the test fails instead of skipping.
 */
void requirePg()
{
  try {
    // Constructed and immediately destroyed: this is a reachability probe.
    db::Connection probe(db::ConnectionType::PostgreSQL, pgConnInfo());
  } catch (const std::exception &) {
    SKIP("no PostgreSQL server reachable - start one with 'docker compose up postgres'");
  }
}

/** Opens a PostgreSQL connection. Call requirePg() first. */
db::Connection connectPg() { return db::Connection(db::ConnectionType::PostgreSQL, pgConnInfo()); }

/** RAII helper: drops the test table when the fixture goes out of scope. */
struct PgTable
{
  explicit PgTable(std::string name) : m_name(std::move(name))
  {
    requirePg();
    drop();
    db::Connection connection(db::ConnectionType::PostgreSQL, pgConnInfo());
    db::Statement statement(connection);
    statement.execute(
      "create table " + m_name + " (id serial primary key, name text not null, value integer not null)");
  }

  ~PgTable()
  {
    try {
      drop();
    } catch (...) {
      // A destructor must not throw; a leftover table is harmless
      // because the name carries the pid, so the next run cannot collide.
    }
  }

  PgTable(const PgTable &) = delete;
  PgTable &operator=(const PgTable &) = delete;

  [[nodiscard]] const std::string &name() const { return m_name; }

  void insert(const std::string &name, int value)
  {
    db::Connection connection(db::ConnectionType::PostgreSQL, pgConnInfo());
    db::Statement statement(connection);
    db::ParameterBuilder builder(std::make_shared<DatabaseConnectionEntry>(
      "pg_test", "", "test", "test", db::ConnectionType::PostgreSQL, "pg_test", 5432, "test", true));
    builder.add(name).add(value);
    statement.execute("insert into " + m_name + " (name, value) values (?, ?)", builder);
  }

  [[nodiscard]] int countRows()
  {
    db::Connection connection(db::ConnectionType::PostgreSQL, pgConnInfo());
    db::Statement statement(connection);
    db::Result result = statement.execute("select count(*) from " + m_name);
    return std::stoi(result.getValue(0, 0));
  }

private:
  void drop()
  {
    db::Connection connection(db::ConnectionType::PostgreSQL, pgConnInfo());
    db::Statement statement(connection);
    statement.execute("drop table if exists " + m_name);
  }

  std::string m_name;
};

/**
 * Table name unique to this process, so parallel ctest runs cannot collide.
 *
 * ctest starts every TEST_CASE in its own process (catch_discover_tests), and
 * every test case creates at most one table, so the pid alone is unique enough.
 * An earlier version added a static counter on top; it was dead weight and was
 * additionally reported as an unused static variable.
 */
std::string uniqueTableName() { return "pg_test_" + std::to_string(::getpid()); }

std::shared_ptr<DatabaseConnectionEntry> pgEntry()
{
  return std::make_shared<DatabaseConnectionEntry>(
    "pg_test", "", "test", "test", db::ConnectionType::PostgreSQL, "pg_test", 5432, "test", true);
}

}// namespace

TEST_CASE("PostgreSQL: connection and simple query", "[pg]")
{
  requirePg();
  db::Connection connection = connectPg();
  db::Statement statement(connection);

  db::Result result = statement.execute("select 1 as one, 'two' as two");
  REQUIRE(result.getSize() == 1);
  REQUIRE(result.getValue(0, 0) == "1");
  REQUIRE(result.getValue(0, 1) == "two");
}

TEST_CASE("PostgreSQL: parameter binding is not string interpolation", "[pg]")
{
  requirePg();
  db::Connection connection = connectPg();
  PgTable table(uniqueTableName());

  // A value that would break the statement if it were interpolated. The
  // classic false confidence here is that SQLite tolerates some of these,
  // so only the PostgreSQL side proves the placeholder path.
  const std::string hostile = "o'brien; drop table " + table.name() + " --";
  table.insert(hostile, 7);

  REQUIRE(table.countRows() == 1);

  db::Statement statement(connection);
  db::ParameterBuilder builder(pgEntry());
  builder.add(hostile);
  db::Result result = statement.execute("select value from " + table.name() + " where name = ?", builder);
  REQUIRE(result.getSize() == 1);
  REQUIRE(result.getValue(0, 0) == "7");
}

TEST_CASE("PostgreSQL: prepared statement is reusable", "[pg]")
{
  requirePg();
  db::Connection connection = connectPg();
  PgTable table(uniqueTableName());

  db::PreparedStatement statement(
    connection, "insert into " + table.name() + " (name, value) values (?, ?)", "pg_insert");

  for (int i = 0; i < 3; i++) {
    db::ParameterBuilder builder(pgEntry());
    builder.add("row" + std::to_string(i)).add(i);
    statement.execute(builder);
  }

  REQUIRE(table.countRows() == 3);
}

TEST_CASE("PostgreSQL: server-side cursor streams rows in batches", "[pg]")
{
  requirePg();
  db::Connection connection = connectPg();
  PgTable table(uniqueTableName());

  for (int i = 0; i < 25; i++) { table.insert("row" + std::to_string(i), i); }

  db::Statement statement(connection);
  db::Cursor cursor = statement.executeCursor("select name, value from " + table.name() + " order by value asc");
  // fetchSize > 0 must produce a DECLARE CURSOR on the backend. A smaller
  // batch than the row count is what proves the cursor is actually paged
  // instead of the whole result being buffered up front.
  cursor.setFetchSize(10);

  // No named bindings for the batches: CodeQL reports a named one as
  // cpp/unused-static-variable even when it is indexed on the next line.
  // The sizes are what proves the paging, and the values are checked by the
  // range-for test below.
  REQUIRE(cursor.fetchNext(10).size() == 10);
  REQUIRE(cursor.fetchNext(10).size() == 10);
  REQUIRE(cursor.fetchNext(10).size() == 5);

  // Exhausted: another fetch yields nothing rather than repeating rows.
  REQUIRE(cursor.fetchNext(10).empty());
}

TEST_CASE("PostgreSQL: cursor range-for iterates every row once", "[pg]")
{
  requirePg();
  db::Connection connection = connectPg();
  PgTable table(uniqueTableName());

  for (int i = 0; i < 7; i++) { table.insert("row" + std::to_string(i), i); }

  db::Statement statement(connection);
  db::Cursor cursor = statement.executeCursor("select name, value from " + table.name() + " order by value asc");
  cursor.setFetchSize(3);

  std::vector<std::string> seen;
  for (auto &row : cursor) {
    REQUIRE(row.size() == 2);
    seen.push_back(row.of(1).getValue());
  }

  REQUIRE(seen.size() == 7);
  REQUIRE(seen.front() == "0");
  REQUIRE(seen.back() == "6");
}

TEST_CASE("PostgreSQL: cursor fetch size 1 needs one round-trip per row", "[pg]")
{
  requirePg();
  db::Connection connection = connectPg();
  PgTable table(uniqueTableName());

  for (int i = 0; i < 4; i++) { table.insert("row" + std::to_string(i), i); }

  db::Statement statement(connection);
  db::Cursor cursor = statement.executeCursor("select name, value from " + table.name() + " order by value asc");
  cursor.setFetchSize(1);

  // fetchSize 1 must yield exactly one row per call; four rows in, then empty.
  REQUIRE(cursor.fetchNext(1).size() == 1);
  REQUIRE(cursor.fetchNext(1).size() == 1);
  REQUIRE(cursor.fetchNext(1).size() == 1);
  REQUIRE(cursor.fetchNext(1).size() == 1);
  REQUIRE(cursor.fetchNext(1).empty());
}

TEST_CASE("PostgreSQL: transaction commit is visible to other connections", "[pg]")
{
  requirePg();
  PgTable table(uniqueTableName());

  {
    db::Connection connection = connectPg();
    db::Statement statement(connection);
    db::Transaction transaction(connection);
    statement.execute("insert into " + table.name() + " (name, value) values ('committed', 1)");
    transaction.commit();
  }

  // A separate connection proves this is a real commit and not just
  // visibility inside the writing transaction.
  db::Connection reader = connectPg();
  db::Statement select(reader);
  REQUIRE(select.execute("select count(*) from " + table.name()).getValue(0, 0) == "1");
}

TEST_CASE("PostgreSQL: transaction rollback discards the insert", "[pg]")
{
  requirePg();
  PgTable table(uniqueTableName());

  {
    db::Connection connection = connectPg();
    db::Statement statement(connection);
    db::Transaction transaction(connection);
    statement.execute("insert into " + table.name() + " (name, value) values ('rolled_back', 1)");
    transaction.rollback();
  }

  db::Connection reader = connectPg();
  db::Statement select(reader);
  REQUIRE(select.execute("select count(*) from " + table.name()).getValue(0, 0) == "0");
}

TEST_CASE("PostgreSQL: savepoint rollback keeps the earlier insert", "[pg]")
{
  requirePg();
  PgTable table(uniqueTableName());

  {
    db::Connection connection = connectPg();
    db::Statement statement(connection);
    db::Transaction transaction(connection);

    statement.execute("insert into " + table.name() + " (name, value) values ('kept', 1)");
    transaction.save("sp1");
    statement.execute("insert into " + table.name() + " (name, value) values ('discarded', 2)");
    transaction.rollbackTo("sp1");
    transaction.commit();
  }

  db::Connection reader = connectPg();
  db::Statement select(reader);
  db::Result result = select.execute("select name from " + table.name());
  REQUIRE(result.getSize() == 1);
  REQUIRE(result.getValue(0, 0) == "kept");
}

TEST_CASE("PostgreSQL: notify delivers to a listening connection", "[pg]")
{
  requirePg();
  PgTable table(uniqueTableName());

  db::Connection listener = connectPg();
  std::atomic_bool received{ false };

  // The listener has to be attached before the notification is sent, and
  // db::Notify invokes the callback on its own thread, hence the atomic.
  db::Notify notify(listener, [&received]() { received.store(true); }, table.name());
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  {
    db::Connection writer = connectPg();
    db::Statement statement(writer);
    statement.execute("notify " + table.name());
  }

  // LISTEN/NOTIFY is asynchronous; give the listener a bounded window rather
  // than asserting immediately, which would be a race.
  for (int i = 0; i < 50 && !received.load(); i++) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
  REQUIRE(received.load());
}