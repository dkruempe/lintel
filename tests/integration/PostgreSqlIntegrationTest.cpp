#include <lintel/core/configuration/DatabaseConnectionEntry.h>
#include <lintel/core/persistence/Connection.h>
#include <lintel/core/persistence/ConnectionType.h>
#include <lintel/core/persistence/Cursor.h>
#include <lintel/core/persistence/Notify.h>
#include <lintel/core/persistence/ParameterBuilder.h>
#include <lintel/core/persistence/PreparedStatement.h>
#include <lintel/core/persistence/Result.h>
#include <lintel/core/persistence/Statement.h>
#include <lintel/core/persistence/Transaction.h>

#include <catch2/catch_all.hpp>

#include "../helpers/PlatformCompat.h"

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
 * docker-compose.yml, and LINTEL_REQUIRE_PG=1 turns "server absent" from a
 * skip into a failure - see requirePg() below for why.
 *
 * Why they exist: `db::Connection` dispatches every call to either the
 * PostgreSQL or the SQLite implementation behind one interface, and the SQLite
 * side was the only one under test. A change that satisfies SQLite can still
 * break PostgreSQL - the two backends disagree about exactly the features
 * covered here (server-side cursors, LISTEN/NOTIFY, real transactions).
 */

namespace lintel {
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

  /** True when the suite must treat a missing server as a failure instead of a skip. */
  bool pgRequired()
  {
    const char *required = std::getenv("LINTEL_REQUIRE_PG");
    return required != nullptr && std::string(required) == "1";
  }

  /**
   * Skips the current test case unless a PostgreSQL server answers.
   *
   * Skipping (rather than failing) is the right call for a local run: the suite
   * must stay green for a contributor without a local server, and the
   * alternative would make `ctest` useless outside CI. The `[pg]` tag keeps
   * these tests selectable.
   *
   * The unconditional SKIP is a hole in CI, though. `catch_discover_tests`
   * translates a Catch2 SKIP into ctest's exit code 4 and
   * `SKIP_RETURN_CODE 4`, so a broken or missing `postgres` service in the
   * workflow would turn all ten `[pg]` tests into "Skipped" and leave the job
   * green - the PostgreSQL backend would silently go untested again, which is
   * the exact state this file was written to end. LINTEL_REQUIRE_PG=1 (set by
   * the build, sanitizer and release jobs) inverts that: unreachable server
   * fails, so a service that does not come up is reported instead of ignored.
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
    } catch (const std::exception &exception) {
      if (pgRequired()) {
        FAIL("LINTEL_REQUIRE_PG=1 but no PostgreSQL server is reachable - the CI service is broken: "
             + std::string(exception.what()));
      }
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

TEST_CASE("PostgreSQL: a destroyed prepared statement leaves nothing on the connection", "[pg]")
{
  requirePg();
  db::Connection connection = connectPg();
  PgTable table(uniqueTableName());
  const std::string statementName = "pg_leak_" + std::to_string(::getpid());

  {
    db::PreparedStatement statement(
      connection, "insert into " + table.name() + " (name, value) values (?, ?)", statementName);
    db::ParameterBuilder builder(pgEntry());
    builder.add(std::string("row")).add(1);
    statement.execute(builder);
  }

  // The destructor deallocates the named statement: nothing survives on the
  // connection, which is what makes a pooled connection safe to reuse. Before
  // the destructor deallocated, the name stayed in pg_prepared_statements (and
  // a second prepare under the same name failed).
  db::Statement check(connection);
  db::Result result = check.execute("select count(*) from pg_prepared_statements where name = '" + statementName + "'");
  REQUIRE(result.getValue(0, 0) == "0");
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

TEST_CASE("PostgreSQL: a concurrent reader sees the old state mid-transaction and the new one after commit", "[pg]")
{
  requirePg();
  PgTable table(uniqueTableName());

  db::Connection writer = connectPg();
  db::Statement writeStatement(writer);
  db::Transaction transaction(writer);
  writeStatement.execute("insert into " + table.name() + " (name, value) values ('in_flight', 1)");

  // The row exists only inside the writer's transaction; a second connection
  // must keep seeing the old (empty) state until the commit.
  db::Connection reader = connectPg();
  db::Statement readStatement(reader);
  REQUIRE(readStatement.execute("select count(*) from " + table.name()).getValue(0, 0) == "0");

  transaction.commit();

  // After the commit the same reader connection must see the new state - this
  // is what separates a real commit from visibility inside the writer only.
  REQUIRE(readStatement.execute("select count(*) from " + table.name()).getValue(0, 0) == "1");
}

TEST_CASE("PostgreSQL: parallel writers and a rollback race never expose a partial transaction", "[pg]")
{
  requirePg();
  PgTable table(uniqueTableName());

  constexpr int kWriters = 4;
  constexpr int kIterations = 10;

  std::atomic_bool writersDone{ false };
  std::atomic<int> writersFinished{ 0 };
  std::atomic<int> writerErrors{ 0 };
  std::atomic<int> samples{ 0 };
  std::atomic<int> pairImbalance{ 0 };
  std::atomic<int> racerRowsVisible{ 0 };

  // Each committed transaction inserts a pair of rows sharing one value, so a
  // partial commit (the bug this test guards) would momentarily leave one half
  // of a pair visible and unbalance the two counts.
  auto writer = [&table, &writerErrors, &writersFinished, &writersDone](int writerIndex) {
    try {
      db::Connection connection = connectPg();
      db::Statement statement(connection);
      for (int i = 0; i < kIterations; i++) {
        db::Transaction transaction(connection);
        const std::string prefix = "w" + std::to_string(writerIndex) + "_" + std::to_string(i);
        statement.execute(
          "insert into " + table.name() + " (name, value) values ('" + prefix + "_a', " + std::to_string(i) + ")");
        statement.execute(
          "insert into " + table.name() + " (name, value) values ('" + prefix + "_b', " + std::to_string(i) + ")");
        transaction.commit();
        // widen the window in which the readers can interleave
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    } catch (const std::exception &) {
      writerErrors.fetch_add(1);
    }
    // the last writer to finish releases the readers
    if (writersFinished.fetch_add(1) + 1 == kWriters) { writersDone.store(true); }
  };

  // The racer keeps a BEGIN/INSERT/ROLLBACK cycle with a pause between INSERT
  // and ROLLBACK running, so the readers sample exactly inside the window in
  // which a broken implementation would show the uncommitted row.
  auto racer = [&table, &writerErrors, &racerRowsVisible, &writersDone]() {
    try {
      db::Connection connection = connectPg();
      db::Statement statement(connection);
      while (true) {
        db::Transaction transaction(connection);
        statement.execute("insert into " + table.name() + " (name, value) values ('racer', 1)");
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        transaction.rollback();
        if (racerRowsVisible.load() > 0 || writerErrors.load() > 0) { return; }
        if (writersDone.load()) { return; }
      }
    } catch (const std::exception &) {
      writerErrors.fetch_add(1);
    }
  };

  // The readers run on their own connections while the writers and the racer
  // are active. Catch2 assertions are not used on the worker threads; the
  // violations are counted and checked on the main thread after the join.
  auto reader = [&table, &writersDone, &writerErrors, &samples, &pairImbalance, &racerRowsVisible]() {
    try {
      db::Connection connection = connectPg();
      db::Statement statement(connection);
      while (!writersDone.load()) {
        // Both counts in ONE statement and therefore one snapshot: a committed
        // pair is either fully visible or not at all. Reading the counts in two
        // statements would let a commit landing between them look like a
        // partial one.
        db::Result result = statement.execute(
          "select count(*) filter (where right(name, 2) = '_a'), "
          "count(*) filter (where right(name, 2) = '_b') from "
          + table.name());
        if (result.getValue(0, 0) != result.getValue(0, 1)) { pairImbalance.fetch_add(1); }
        const int racerRows =
          std::stoi(statement.execute("select count(*) from " + table.name() + " where name = 'racer'").getValue(0, 0));
        if (racerRows != 0) { racerRowsVisible.fetch_add(1); }
        samples.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    } catch (const std::exception &) {
      writerErrors.fetch_add(1);
    }
  };

  std::vector<std::thread> threads;
  threads.reserve(kWriters + 3);
  for (int i = 0; i < kWriters; i++) { threads.emplace_back(writer, i); }
  threads.emplace_back(racer);
  threads.emplace_back(reader);
  threads.emplace_back(reader);
  for (auto &thread : threads) { thread.join(); }

  REQUIRE(writerErrors.load() == 0);
  // the readers must really have overlapped with the writers
  REQUIRE(samples.load() > 0);
  REQUIRE(pairImbalance.load() == 0);
  REQUIRE(racerRowsVisible.load() == 0);

  // Rollback race, deterministic variant: a second connection reads while the
  // transaction is open and again after the rollback - the row must be
  // invisible in both cases.
  db::Connection observer = connectPg();
  const auto countWhere = [&table](db::Connection &connection, const std::string &condition) {
    db::Statement statement(connection);
    return std::stoi(statement.execute("select count(*) from " + table.name() + " where " + condition).getValue(0, 0));
  };
  {
    db::Connection racerConnection = connectPg();
    db::Statement statement(racerConnection);
    db::Transaction transaction(racerConnection);
    statement.execute("insert into " + table.name() + " (name, value) values ('racer', 1)");
    REQUIRE(countWhere(observer, "name = 'racer'") == 0);
    transaction.rollback();
  }
  REQUIRE(countWhere(observer, "name = 'racer'") == 0);

  // Final state: every writer pair committed exactly once, nothing of the racer.
  REQUIRE(countWhere(observer, "right(name, 2) = '_a'") == kWriters * kIterations);
  REQUIRE(countWhere(observer, "right(name, 2) = '_b'") == kWriters * kIterations);
  REQUIRE(countWhere(observer, "name = 'racer'") == 0);
}

TEST_CASE("PostgreSQL: a blocked update times out with the driver text instead of hanging", "[pg]")
{
  requirePg();
  PgTable table(uniqueTableName());
  table.insert("contended", 1);

  // The holder takes the row lock and keeps the transaction open.
  db::Connection holder = connectPg();
  db::Statement holdStatement(holder);
  db::Transaction holdTransaction(holder);
  holdStatement.execute("update " + table.name() + " set value = 2 where name = 'contended'");

  std::string message;
  std::chrono::milliseconds elapsed{ 0 };
  {
    db::Connection blocked = connectPg();
    db::Statement statement(blocked);
    // lock_timeout turns the indefinite row-lock wait into a bounded error;
    // set before BEGIN so it applies to the whole blocked transaction.
    statement.execute("set lock_timeout = '300ms'");
    db::Transaction transaction(blocked);
    const auto start = std::chrono::steady_clock::now();
    try {
      statement.execute("update " + table.name() + " set value = 3 where name = 'contended'");
      FAIL("the blocked update must not succeed while the holder keeps the row lock");
    } catch (const db::SQLException &exception) {
      message = exception.what();
    }
    elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    // cleanup: the failed transaction must roll back without hanging
    transaction.rollback();
  }
  holdTransaction.rollback();

  // The driver text ("canceling statement due to lock timeout") has to survive
  // into the exception message - a generic "statement failed" would hide the
  // actual cause.
  REQUIRE(message.find("lock timeout") != std::string::npos);
  // it waited for the lock instead of failing instantly for another reason ...
  REQUIRE(elapsed >= std::chrono::milliseconds(200));
  // ... and it did not hang beyond the timeout.
  REQUIRE(elapsed < std::chrono::seconds(5));

  // After the holder rolled back the row is free again and its value is the
  // original one - neither of the two blocked updates was applied.
  db::Connection after = connectPg();
  db::Statement afterStatement(after);
  REQUIRE(
    afterStatement.execute("select value from " + table.name() + " where name = 'contended'").getValue(0, 0) == "1");
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

TEST_CASE("PostgreSQL: notify keeps delivering while inserts are running", "[pg]")
{
  requirePg();
  PgTable table(uniqueTableName());

  constexpr int kWrites = 20;

  db::Connection listener = connectPg();
  std::atomic<int> callbacks{ 0 };
  db::Notify notify(listener, [&callbacks]() { callbacks.fetch_add(1); }, table.name());
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Writer thread: every insert is followed by a notification, so the listener
  // is under constant traffic while the rows land. The callback may batch
  // several notifications into one wakeup (the listener drains the queue per
  // select() round-trip), so exact callback counts are not asserted - delivery
  // within a deadline and progress after the load are.
  std::atomic_bool writerDone{ false };
  std::thread writer([&table, &writerDone]() {
    db::Connection connection = connectPg();
    db::Statement statement(connection);
    for (int i = 0; i < kWrites; i++) {
      statement.execute("insert into " + table.name() + " (name, value) values ('load" + std::to_string(i) + "', "
                        + std::to_string(i) + ")");
      statement.execute("notify " + table.name());
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    writerDone.store(true);
  });

  // The first notification must arrive while the load is still running.
  const auto start = std::chrono::steady_clock::now();
  while (!writerDone.load() && callbacks.load() == 0) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
  const auto firstDelivery = std::chrono::steady_clock::now() - start;
  writer.join();

  REQUIRE(callbacks.load() >= 1);
  // Generous deadline: the listener wakes at most one poll interval (1s) after
  // the notification reaches the server.
  REQUIRE(firstDelivery < std::chrono::seconds(5));

  // All rows landed ...
  db::Connection reader = connectPg();
  db::Statement select(reader);
  REQUIRE(select.execute("select count(*) from " + table.name()).getValue(0, 0) == std::to_string(kWrites));

  // ... and the listener is still alive after the load: a final notification
  // sent on an otherwise idle channel must produce another callback.
  const int before = callbacks.load();
  db::Connection writerConnection = connectPg();
  db::Statement notifyStatement(writerConnection);
  notifyStatement.execute("notify " + table.name());
  for (int i = 0; i < 50 && callbacks.load() <= before; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  REQUIRE(callbacks.load() > before);
}
}// namespace lintel