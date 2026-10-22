#include <lintel/core/exceptions/SQLException.h>
#include <lintel/core/persistence/Connection.h>
#include <lintel/core/persistence/Statement.h>

#include <catch2/catch_all.hpp>

#include <cstdio>
#include <filesystem>
#include <string>

#include "../helpers/PlatformCompat.h"

namespace lintel {
namespace {

  std::string kPragmaTestDb()
  {
    static int counter = 0;
    return (std::filesystem::temp_directory_path()
            / ("sqlite_pragma_test_" + std::to_string(::getpid()) + "_" + std::to_string(counter++) + ".db"))
      .string();
  }

  /// Creates a unique temp DB path; removes the file (and WAL leftovers) again.
  struct PragmaFixture
  {
    std::string m_dbPath;

    PragmaFixture() : m_dbPath(kPragmaTestDb()) { static_cast<void>(std::remove(m_dbPath.c_str())); }

    ~PragmaFixture()
    {
      static_cast<void>(std::remove(m_dbPath.c_str()));
      // WAL mode creates -wal/-shm sidecar files next to the DB file
      static_cast<void>(std::remove((m_dbPath + "-wal").c_str()));
      static_cast<void>(std::remove((m_dbPath + "-shm").c_str()));
    }

    [[nodiscard]] std::shared_ptr<DatabaseConnectionEntry> connectionEntry() const
    {
      return std::make_shared<DatabaseConnectionEntry>(
        "pragma_test", m_dbPath, "", "", db::ConnectionType::SQLite, "default", 0, "", true);
    }
  };

  std::string queryPragma(const db::Connection &connection, const std::string &pragma)
  {
    db::Statement statement(connection);
    db::Result result = statement.execute(pragma);
    REQUIRE(result.getSize() == 1);
    return result.of(0).of(0).getValue<std::string>();
  }

}// namespace

TEST_CASE("SQLite: busy_timeout is set to 5000 ms on file databases", "[sqlite][pragma]")
{
  PragmaFixture fixture;
  db::Connection connection(db::ConnectionType::SQLite, fixture.m_dbPath);
  REQUIRE(queryPragma(connection, "PRAGMA busy_timeout") == "5000");
}

TEST_CASE("SQLite: busy_timeout is set on :memory: databases", "[sqlite][pragma]")
{
  db::Connection connection(db::ConnectionType::SQLite, ":memory:");
  REQUIRE(queryPragma(connection, "PRAGMA busy_timeout") == "5000");
}

TEST_CASE("SQLite: journal_mode is WAL for file databases", "[sqlite][pragma]")
{
  PragmaFixture fixture;
  db::Connection connection(db::ConnectionType::SQLite, fixture.m_dbPath);
  REQUIRE(queryPragma(connection, "PRAGMA journal_mode") == "wal");
}

TEST_CASE("SQLite: journal_mode is WAL via DatabaseConnectionEntry", "[sqlite][pragma]")
{
  PragmaFixture fixture;
  db::Connection connection(fixture.connectionEntry());
  REQUIRE(queryPragma(connection, "PRAGMA journal_mode") == "wal");
}

TEST_CASE("SQLite: journal_mode stays memory for :memory: databases", "[sqlite][pragma]")
{
  db::Connection connection(db::ConnectionType::SQLite, ":memory:");
  REQUIRE(queryPragma(connection, "PRAGMA journal_mode") != "wal");
}

TEST_CASE("SQLite: foreign_keys ON rejects violating inserts", "[sqlite][pragma]")
{
  PragmaFixture fixture;
  db::Connection connection(db::ConnectionType::SQLite, fixture.m_dbPath);
  db::Statement statement(connection);
  statement.execute("create table parent (id integer primary key)");
  statement.execute("create table child (id integer primary key, parent_id integer references parent(id))");
  statement.execute("insert into parent (id) values (1)");

  // valid insert passes
  REQUIRE_NOTHROW(statement.execute("insert into child (id, parent_id) values (1, 1)"));
  // missing parent must fail with a constraint violation
  REQUIRE_THROWS_AS(statement.execute("insert into child (id, parent_id) values (2, 999)"), db::SQLException);
}

TEST_CASE("SQLite: execute runs every statement of a script", "[sqlite][execute]")
{
  db::Connection connection(db::ConnectionType::SQLite, ":memory:");
  db::Statement statement(connection);

  // DDL plus DML in a single call: only the first statement would run if the prepare tail were ignored
  statement.execute(
    "create table multi (id integer primary key, name text);\n"
    "insert into multi (id, name) values (1, 'first');\n"
    "insert into multi (id, name) values (2, 'second');");

  db::Result rows = statement.execute("select name from multi order by id");
  REQUIRE(rows.getSize() == 2);
  REQUIRE(rows.of(0).of(0).getValue<std::string>() == "first");
  REQUIRE(rows.of(1).of(0).getValue<std::string>() == "second");

  // the rows of all statements end up in the same result
  db::Result combined = statement.execute("select 1 as value; select 2 as value");
  REQUIRE(combined.getSize() == 2);
}

TEST_CASE("SQLite: execute reports a failure in a later statement", "[sqlite][execute]")
{
  db::Connection connection(db::ConnectionType::SQLite, ":memory:");
  db::Statement statement(connection);
  statement.execute("create table later (id integer primary key)");

  REQUIRE_THROWS_AS(
    statement.execute("insert into later (id) values (1); insert into missing (id) values (2);"), db::SQLException);
}

TEST_CASE("SQLite: foreign_keys ON applies to DatabaseConnectionEntry connections", "[sqlite][pragma]")
{
  PragmaFixture fixture;
  db::Connection connection(fixture.connectionEntry());
  db::Statement statement(connection);
  statement.execute("create table parent (id integer primary key)");
  statement.execute("create table child (id integer primary key, parent_id integer references parent(id))");

  REQUIRE_THROWS_AS(statement.execute("insert into child (id, parent_id) values (1, 42)"), db::SQLException);
}
}// namespace lintel
