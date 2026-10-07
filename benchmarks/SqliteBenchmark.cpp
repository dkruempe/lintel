#include <benchmark/benchmark.h>

#include <cstdio>
#include <string>
#include <vector>

#include "lintel/core/persistence/sqlite3/Connection.h"
#include "lintel/core/persistence/sqlite3/PreparedStatement.h"
#include "lintel/core/persistence/sqlite3/Statement.h"
#include "lintel/core/persistence/sqlite3/Transaction.h"

static const char *kBenchDbPath = "/tmp/bench_lintel.db";

static void RemoveBenchDb() { std::remove(kBenchDbPath); }

static void CreateBenchTable(sqlite::Connection &conn) {
    sqlite::Statement stmt(conn);
    stmt.execute(
        "CREATE TABLE IF NOT EXISTS bench_data ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "value TEXT NOT NULL,"
        "category TEXT NOT NULL)");
}

static void SeedBenchData(sqlite::Connection &conn, int count) {
    sqlite::Transaction txn(conn);
    txn.start();
    sqlite::PreparedStatement stmt(
        conn,
        "INSERT INTO bench_data (name, value, category) VALUES (?, ?, ?)",
        "bench_seed");
    for (int i = 0; i < count; ++i) {
        stmt.execute({"name_" + std::to_string(i), std::to_string(i), "cat"});
    }
    txn.commit();
}

static void BM_Insert_Single(benchmark::State &state) {
    RemoveBenchDb();
    for (auto _ : state) {
        state.PauseTiming();
        sqlite::Connection conn(kBenchDbPath);
        CreateBenchTable(conn);
        state.ResumeTiming();
        sqlite::Statement stmt(conn);
        stmt.execute(
            "INSERT INTO bench_data (name, value, category) "
            "VALUES ('test', '42', 'benchmark')");
    }
    RemoveBenchDb();
}
BENCHMARK(BM_Insert_Single);

static void BM_Insert_Prepared(benchmark::State &state) {
    RemoveBenchDb();
    for (auto _ : state) {
        state.PauseTiming();
        sqlite::Connection conn(kBenchDbPath);
        CreateBenchTable(conn);
        state.ResumeTiming();
        sqlite::PreparedStatement stmt(
            conn,
            "INSERT INTO bench_data (name, value, category) VALUES (?, ?, ?)",
            "bench_insert");
        stmt.execute({"test", "42", "benchmark"});
    }
    RemoveBenchDb();
}
BENCHMARK(BM_Insert_Prepared);

static void BM_Insert_PreparedReused(benchmark::State &state) {
    RemoveBenchDb();
    sqlite::Connection conn(kBenchDbPath);
    CreateBenchTable(conn);
    sqlite::PreparedStatement stmt(
        conn,
        "INSERT INTO bench_data (name, value, category) VALUES (?, ?, ?)",
        "bench_insert_reused");
    for (auto _ : state) {
        stmt.execute({"test", "42", "benchmark"});
    }
    RemoveBenchDb();
}
BENCHMARK(BM_Insert_PreparedReused);

static void BM_Insert_Batch100(benchmark::State &state) {
    RemoveBenchDb();
    for (auto _ : state) {
        state.PauseTiming();
        sqlite::Connection conn(kBenchDbPath);
        CreateBenchTable(conn);
        state.ResumeTiming();
        sqlite::Transaction txn(conn);
        txn.start();
        sqlite::PreparedStatement stmt(
            conn,
            "INSERT INTO bench_data (name, value, category) VALUES (?, ?, ?)",
            "bench_batch");
        for (int i = 0; i < 100; ++i) {
            stmt.execute(
                {"name_" + std::to_string(i), std::to_string(i), "cat"});
        }
        txn.commit();
    }
    RemoveBenchDb();
}
BENCHMARK(BM_Insert_Batch100);

static void BM_Insert_Batch1000(benchmark::State &state) {
    RemoveBenchDb();
    for (auto _ : state) {
        state.PauseTiming();
        sqlite::Connection conn(kBenchDbPath);
        CreateBenchTable(conn);
        state.ResumeTiming();
        sqlite::Transaction txn(conn);
        txn.start();
        sqlite::PreparedStatement stmt(
            conn,
            "INSERT INTO bench_data (name, value, category) VALUES (?, ?, ?)",
            "bench_batch1k");
        for (int i = 0; i < 1000; ++i) {
            stmt.execute(
                {"name_" + std::to_string(i), std::to_string(i), "cat"});
        }
        txn.commit();
    }
    RemoveBenchDb();
}
BENCHMARK(BM_Insert_Batch1000);

static void BM_Select_All(benchmark::State &state) {
    RemoveBenchDb();
    {
        sqlite::Connection conn(kBenchDbPath);
        CreateBenchTable(conn);
        SeedBenchData(conn, 1000);
    }
    for (auto _ : state) {
        sqlite::Connection conn(kBenchDbPath);
        sqlite::Statement stmt(conn);
        stmt.execute("SELECT * FROM bench_data");
    }
    RemoveBenchDb();
}
BENCHMARK(BM_Select_All);

static void BM_Select_Where(benchmark::State &state) {
    RemoveBenchDb();
    {
        sqlite::Connection conn(kBenchDbPath);
        CreateBenchTable(conn);
        SeedBenchData(conn, 1000);
    }
    for (auto _ : state) {
        sqlite::Connection conn(kBenchDbPath);
        sqlite::PreparedStatement stmt(
            conn,
            "SELECT * FROM bench_data WHERE category = ?",
            "bench_select_where");
        stmt.execute({"cat"});
    }
    RemoveBenchDb();
}
BENCHMARK(BM_Select_Where);

static void BM_Select_PreparedReused(benchmark::State &state) {
    RemoveBenchDb();
    {
        sqlite::Connection conn(kBenchDbPath);
        CreateBenchTable(conn);
        SeedBenchData(conn, 1000);
    }
    sqlite::Connection conn(kBenchDbPath);
    sqlite::PreparedStatement stmt(
        conn,
        "SELECT * FROM bench_data WHERE name = ?",
        "bench_select_reused");
    for (auto _ : state) {
        stmt.execute({"name_500"});
    }
    RemoveBenchDb();
}
BENCHMARK(BM_Select_PreparedReused);

static void BM_Insert_Transaction100(benchmark::State &state) {
    RemoveBenchDb();
    for (auto _ : state) {
        state.PauseTiming();
        sqlite::Connection conn(kBenchDbPath);
        CreateBenchTable(conn);
        state.ResumeTiming();
        sqlite::Transaction txn(conn);
        txn.start();
        sqlite::PreparedStatement stmt(
            conn,
            "INSERT INTO bench_data (name, value, category) VALUES (?, ?, ?)",
            "bench_txn");
        for (int i = 0; i < 100; ++i) {
            stmt.execute(
                {"name_" + std::to_string(i), std::to_string(i), "cat"});
        }
        txn.commit();
    }
    RemoveBenchDb();
}
BENCHMARK(BM_Insert_Transaction100);
