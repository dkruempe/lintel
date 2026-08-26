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
    constexpr int rowCount = 150;  // > BATCH_SIZE (64)
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
