#include <base_library/core/exceptions/SQLException.h>
#include <base_library/core/persistence/Arguments.h>
#include <base_library/core/persistence/Connection.h>
#include <base_library/core/persistence/Cursor.h>
#include <base_library/core/persistence/Statement.h>

#include <catch2/catch_all.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace {

std::string kCursorTestDb() {
    static int counter = 0;
    return (std::filesystem::temp_directory_path() /
            ("cursor_test_" + std::to_string(counter++) + ".db"))
            .string();
}

struct CursorFixture {
    std::string m_dbPath;

    CursorFixture() : m_dbPath(kCursorTestDb()) {
        std::remove(m_dbPath.c_str());
        db::Connection connection(db::ConnectionType::SQLite, m_dbPath);
        db::Statement statement(connection);
        statement.execute(
                "create table items (name text not null, value integer not null)");
    }

    ~CursorFixture() { std::remove(m_dbPath.c_str()); }

    void insert(const std::string &name, int value) {
        db::Connection connection(db::ConnectionType::SQLite, m_dbPath);
        db::Statement statement(connection);
        db::ParameterBuilder builder(connectionEntry());
        builder.add(name).add(value);
        statement.execute("insert into items (name, value) values (?, ?)", builder);
    }

    [[nodiscard]] std::shared_ptr<DatabaseConnectionEntry> connectionEntry() const {
        return std::make_shared<DatabaseConnectionEntry>(
                "cursor_test", m_dbPath, "", "", db::ConnectionType::SQLite,
                "default", 0, "", true);
    }
};

}  // namespace

TEST_CASE("Cursor: iterates all rows with range-for", "[cursor]") {
    CursorFixture fixture;
    for (int i = 0; i < 5; i++) {
        fixture.insert("row" + std::to_string(i), i);
    }

    db::Connection connection(fixture.connectionEntry());
    db::Statement statement(connection);
    db::Cursor cursor = statement.executeCursor(
            "select name, value from items order by value asc");

    int expected = 0;
    for (const auto &row: cursor) {
        REQUIRE(row.of(0).getValue<std::string>() == "row" + std::to_string(expected));
        REQUIRE(row.of(1).getValue<int>() == expected);
        expected++;
    }
    REQUIRE(expected == 5);
}

TEST_CASE("Cursor: crosses batch boundaries", "[cursor]") {
    CursorFixture fixture;
    constexpr int rowCount = 150;  // > default fetchSize (100)
    for (int i = 0; i < rowCount; i++) {
        fixture.insert("row" + std::to_string(i), i);
    }

    db::Connection connection(fixture.connectionEntry());
    db::Statement statement(connection);
    db::Cursor cursor =
            statement.executeCursor("select value from items order by value asc");

    int expected = 0;
    for (const auto &row: cursor) {
        REQUIRE(row.of(0).getValue<int>() == expected);
        expected++;
    }
    REQUIRE(expected == rowCount);
}

TEST_CASE("Cursor: empty result yields exhausted iterator", "[cursor]") {
    CursorFixture fixture;

    db::Connection connection(fixture.connectionEntry());
    db::Statement statement(connection);
    db::Cursor cursor =
            statement.executeCursor("select name from items where value = 42");

    int iterations = 0;
    for (const auto &row: cursor) {
        (void) row;
        iterations++;
    }
    REQUIRE(iterations == 0);
}

TEST_CASE("Cursor: binds parameters", "[cursor]") {
    CursorFixture fixture;
    fixture.insert("alpha", 1);
    fixture.insert("beta", 2);
    fixture.insert("gamma", 3);

    db::Connection connection(fixture.connectionEntry());
    db::Statement statement(connection);
    db::ParameterBuilder builder(fixture.connectionEntry());
    builder.add(std::string("beta"));
    db::Cursor cursor = statement.executeCursor(
            "select value from items where name = ?", builder);

    auto rows = cursor.fetchNext(10);
    REQUIRE(rows.size() == 1);
    REQUIRE(rows[0].of(0).getValue<int>() == 2);
}

TEST_CASE("Cursor: early break leaves connection usable", "[cursor]") {
    CursorFixture fixture;
    for (int i = 0; i < 20; i++) {
        fixture.insert("row" + std::to_string(i), i);
    }

    db::Connection connection(fixture.connectionEntry());
    {
        db::Statement statement(connection);
        db::Cursor cursor =
                statement.executeCursor("select value from items order by value asc");
        int consumed = 0;
        for (const auto &row: cursor) {
            (void) row;
            if (++consumed == 3) {
                break;
            }
        }
        REQUIRE(consumed == 3);
    }  // cursor destroyed before exhaustion

    // the same connection must still work afterwards
    db::Statement statement(connection);
    db::Result result = statement.execute("select count(*) from items");
    REQUIRE(result.getSize() == 1);
}

TEST_CASE("Cursor: fetchNext returns batches until exhausted", "[cursor]") {
    CursorFixture fixture;
    constexpr int rowCount = 70;
    for (int i = 0; i < rowCount; i++) {
        fixture.insert("row" + std::to_string(i), i);
    }

    db::Connection connection(fixture.connectionEntry());
    db::Statement statement(connection);
    db::ParameterBuilder builder(fixture.connectionEntry());
    builder.add(0);
    db::Cursor cursor = statement.executeCursor(
            "select value from items where value >= ? order by value asc", builder);

    const auto firstBatch = cursor.fetchNext(50);
    REQUIRE(firstBatch.size() == 50);
    const auto secondBatch = cursor.fetchNext(50);
    REQUIRE(secondBatch.size() == 20);  // fewer than requested => exhausted
    const auto thirdBatch = cursor.fetchNext(50);
    REQUIRE(thirdBatch.empty());
}

// ── Fetch-size configuration tests ──────────────────────────────────

TEST_CASE("Cursor: setFetchSize(1) fetches one row per backend round-trip", "[cursor]") {
    CursorFixture fixture;
    for (int i = 0; i < 5; i++) {
        fixture.insert("row" + std::to_string(i), i);
    }

    db::Connection connection(fixture.connectionEntry());
    db::Statement statement(connection);
    db::Cursor cursor = statement.executeCursor(
            "select value from items order by value asc");
    cursor.setFetchSize(1);

    // fetchNext(1) should return exactly 1 row (backend returns 1 at a time)
    const auto single1 = cursor.fetchNext(1);
    REQUIRE(single1.size() == 1);
    REQUIRE(single1[0].of(0).getValue<int>() == 0);

    const auto single2 = cursor.fetchNext(1);
    REQUIRE(single2.size() == 1);
    REQUIRE(single2[0].of(0).getValue<int>() == 1);

    // fetchNext with larger N still works — fetches internally 1-by-1
    const auto batch = cursor.fetchNext(10);
    REQUIRE(batch.size() == 3);
    REQUIRE(batch[0].of(0).getValue<int>() == 2);
    REQUIRE(batch[2].of(0).getValue<int>() == 4);
}

TEST_CASE("Cursor: setFetchSize(0) uses streaming fallback", "[cursor]") {
    CursorFixture fixture;
    for (int i = 0; i < 20; i++) {
        fixture.insert("row" + std::to_string(i), i);
    }

    db::Connection connection(fixture.connectionEntry());
    db::Statement statement(connection);
    db::Cursor cursor = statement.executeCursor(
            "select value from items order by value asc");
    cursor.setFetchSize(0);  // streaming: internal buffer = 64

    // Should still iterate all rows correctly
    int expected = 0;
    for (const auto &row: cursor) {
        REQUIRE(row.of(0).getValue<int>() == expected);
        expected++;
    }
    REQUIRE(expected == 20);
}

TEST_CASE("Cursor: setFetchSize(200) buffers large batches", "[cursor]") {
    CursorFixture fixture;
    constexpr int rowCount = 150;
    for (int i = 0; i < rowCount; i++) {
        fixture.insert("row" + std::to_string(i), i);
    }

    db::Connection connection(fixture.connectionEntry());
    db::Statement statement(connection);
    db::Cursor cursor = statement.executeCursor(
            "select value from items order by value asc");
    cursor.setFetchSize(200);

    // All 150 rows should come in one batch (150 < 200)
    const auto batch = cursor.fetchNext(200);
    REQUIRE(batch.size() == 150);

    // No more rows
    const auto empty = cursor.fetchNext(10);
    REQUIRE(empty.empty());
}

TEST_CASE("Cursor: fetchSize respects query filter", "[cursor]") {
    CursorFixture fixture;
    for (int i = 0; i < 10; i++) {
        fixture.insert("row" + std::to_string(i), i);
    }

    db::Connection connection(fixture.connectionEntry());
    db::Statement statement(connection);
    db::ParameterBuilder builder(fixture.connectionEntry());
    builder.add(5);
    db::Cursor cursor = statement.executeCursor(
            "select value from items where value >= ? order by value asc", builder);
    cursor.setFetchSize(3);

    // fetchNext(3) with backend fetchSize=3 → returns exactly 3 rows
    const auto batch1 = cursor.fetchNext(3);
    REQUIRE(batch1.size() == 3);
    REQUIRE(batch1[0].of(0).getValue<int>() == 5);
    REQUIRE(batch1[2].of(0).getValue<int>() == 7);

    // Remaining 2 rows
    const auto batch2 = cursor.fetchNext(3);
    REQUIRE(batch2.size() == 2);
    REQUIRE(batch2[0].of(0).getValue<int>() == 8);
    REQUIRE(batch2[1].of(0).getValue<int>() == 9);

    REQUIRE(cursor.fetchNext(3).empty());
}

TEST_CASE("Cursor: range-for with small fetchSize", "[cursor]") {
    CursorFixture fixture;
    for (int i = 0; i < 7; i++) {
        fixture.insert("row" + std::to_string(i), i);
    }

    db::Connection connection(fixture.connectionEntry());
    db::Statement statement(connection);
    db::Cursor cursor = statement.executeCursor(
            "select value from items order by value asc");
    cursor.setFetchSize(3);

    // range-for should work regardless of fetchSize
    int expected = 0;
    for (const auto &row: cursor) {
        REQUIRE(row.of(0).getValue<int>() == expected);
        expected++;
    }
    REQUIRE(expected == 7);
}

TEST_CASE("Argument: typed getValue deserializes for every connection type", "[argument]")
{
  db::Argument argument("1", "value");

  // UNDEFINED goes through StringifyService.
  REQUIRE(argument.getValue<int>() == 1);
  REQUIRE(argument.getValue<std::string>() == "1");

  // SQLite goes through sqlite::Serialization.
  argument.setConnectionType(db::ConnectionType::SQLite);
  REQUIRE(argument.getValue<int>() == 1);
  REQUIRE_NOTHROW(argument.getValue<int>());

  // Deliberately NOT tested: a ConnectionType::Value outside the enumerator range.
  // Argument::getValue() does have a `default:` branch that throws db::SQLException
  // (CodeQL cpp/missing-return reported the old fall-out-of-function), but that path
  // cannot be reached without undefined behaviour - loading an out-of-range value into
  // an enumeration is UB in C++ and the UndefinedBehaviorSanitizer job flags exactly
  // that. An earlier version of this test built such a value via memcpy; the sanitizer
  // caught it and the test was rewritten. The branch stays as hardening, untested.
}
