#include <lintel/core/persistence/Connection.h>
#include <lintel/core/persistence/Statement.h>
#include <lintel/core/persistence/ParameterBuilder.h>
#include <lintel/core/persistence/Transaction.h>

#include <catch2/catch_all.hpp>

TEST_CASE("SQLite: create in-memory connection and execute query") {
    db::Connection conn(db::ConnectionType::SQLite, ":memory:");
    db::Statement stmt(conn);

    auto result = stmt.execute("CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)");
    REQUIRE(result.getSize() == 0);
}

TEST_CASE("SQLite: insert and select data") {
    db::Connection conn(db::ConnectionType::SQLite, ":memory:");
    db::Statement stmt(conn);

    stmt.execute("CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)");
    stmt.execute("INSERT INTO test VALUES (1, 'Alice')");
    stmt.execute("INSERT INTO test VALUES (2, 'Bob')");

    auto result = stmt.execute("SELECT * FROM test ORDER BY id");
    REQUIRE(result.getSize() == 2);
    REQUIRE(result.getValue(0, 0) == "1");
    REQUIRE(result.getValue(0, 1) == "Alice");
    REQUIRE(result.getValue(1, 0) == "2");
    REQUIRE(result.getValue(1, 1) == "Bob");
}

TEST_CASE("SQLite: iterate over result") {
    db::Connection conn(db::ConnectionType::SQLite, ":memory:");
    db::Statement stmt(conn);

    stmt.execute("CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)");
    stmt.execute("INSERT INTO test VALUES (1, 'Alice')");
    stmt.execute("INSERT INTO test VALUES (2, 'Bob')");
    stmt.execute("INSERT INTO test VALUES (3, 'Charlie')");

    auto result = stmt.execute("SELECT * FROM test ORDER BY id");
    int count = 0;
    for (auto it = result.begin(); it != result.end(); ++it) {
        auto &args = *it;
        REQUIRE(args.size() == 2);
        count++;
    }
    REQUIRE(count == 3);
}

TEST_CASE("SQLite: transaction commit") {
    db::Connection conn(db::ConnectionType::SQLite, ":memory:");
    db::Statement stmt(conn);
    db::Transaction tx(conn);

    tx.start();
    stmt.execute("CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)");
    stmt.execute("INSERT INTO test VALUES (1, 'Alice')");
    tx.commit();

    auto result = stmt.execute("SELECT * FROM test");
    REQUIRE(result.getSize() == 1);
}

TEST_CASE("SQLite: transaction rollback") {
    db::Connection conn(db::ConnectionType::SQLite, ":memory:");
    db::Statement stmt(conn);
    db::Transaction tx(conn);

    tx.start();
    stmt.execute("CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)");
    stmt.execute("INSERT INTO test VALUES (1, 'Alice')");
    tx.rollback();

    auto result = stmt.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='test'");
    REQUIRE(result.getSize() == 0);
}

TEST_CASE("SQLite: savepoint and rollback to savepoint") {
    db::Connection conn(db::ConnectionType::SQLite, ":memory:");
    db::Statement stmt(conn);
    db::Transaction tx(conn);

    tx.start();
    stmt.execute("CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)");
    stmt.execute("INSERT INTO test VALUES (1, 'Alice')");
    tx.save("sp1");
    stmt.execute("INSERT INTO test VALUES (2, 'Bob')");
    tx.rollbackTo("sp1");

    auto result = stmt.execute("SELECT * FROM test");
    REQUIRE(result.getSize() == 1);
    tx.commit();
}

TEST_CASE("SQLite: prepared statement with parameters") {
    db::Connection conn(db::ConnectionType::SQLite, ":memory:");
    db::Statement stmt(conn);

    stmt.execute("CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)");
    stmt.execute("INSERT INTO test VALUES (1, 'Alice')");
    stmt.execute("INSERT INTO test VALUES (2, 'Bob')");

    auto result = stmt.execute("SELECT * FROM test WHERE name = 'Alice'");
    REQUIRE(result.getSize() == 1);
    REQUIRE(result.getValue(0, 1) == "Alice");
}

TEST_CASE("SQLite: parameters via ParameterBuilder") {
    db::Connection conn(db::ConnectionType::SQLite, ":memory:");
    db::Statement stmt(conn);

    stmt.execute("CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)");

    auto result = stmt.execute("SELECT 1 + 1");
    REQUIRE(result.getSize() == 1);
    REQUIRE(result.getValue(0, 0) == "2");
}

TEST_CASE("SQLite: empty result on CREATE") {
    db::Connection conn(db::ConnectionType::SQLite, ":memory:");
    db::Statement stmt(conn);

    auto result = stmt.execute("CREATE TABLE test (id INTEGER)");
    REQUIRE(result.getSize() == 0);
    REQUIRE(result.getNumOfAttributes() == 0);
}

TEST_CASE("SQLite: select from empty table") {
    db::Connection conn(db::ConnectionType::SQLite, ":memory:");
    db::Statement stmt(conn);

    stmt.execute("CREATE TABLE test (id INTEGER)");
    auto result = stmt.execute("SELECT * FROM test");
    REQUIRE(result.getSize() == 0);
}
