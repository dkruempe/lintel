#include <benchmark/benchmark.h>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

#include "lintel/core/configuration/DatabaseConnectionEntry.h"
#include "lintel/core/persistence/Connection.h"
#include "lintel/core/persistence/ConnectionPool.h"
#include "lintel/core/persistence/Cursor.h"
#include "lintel/core/persistence/Notify.h"
#include "lintel/core/persistence/ParameterBuilder.h"
#include "lintel/core/persistence/PreparedStatement.h"
#include "lintel/core/persistence/Statement.h"
#include "lintel/core/persistence/Transaction.h"

/**
 * PostgreSQL runtime benchmarks (`ROADMAP.md` P2/P5, `docs/persistence-plan.md` Phase 8).
 *
 * The cases mirror what the pooling work of phases 2-6 changed on the wire:
 *
 *  - `PgCursorBenchmark/FetchSize` prices the server-side cursor at 1/10/100/1000
 *    rows per round-trip over a 10k-row table. This is the number the
 *    `kDefaultFetchSize = 100` decision in `Cursor.h` is based on.
 *  - `PgConnectionBenchmark/TransactionBatch` vs `/IndividualTransactions`
 *    compares one transaction of `kBatchInserts` inserts with `kBatchInserts`
 *    single-transaction inserts (the commit round-trips).
 *  - `PgConnectionBenchmark/PreparedStatementReuse` vs `/ParameterizedStatement`
 *    compares a reused named prepared statement (the path phase 4 touched) with
 *    a parameterized `db::Statement` that prepares anonymously on every call.
 *  - `PgPoolBenchmark/AcquireRelease` prices `ConnectionPool::acquire()`/release
 *    (including the `PQstatus` health check) for 1/4/16 slots; `/DirectConnection`
 *    pays connect + auth on every call - the difference is the pool's point.
 *  - `BM_Pg_NotifyRoundTrip` measures the LISTEN/NOTIFY round-trip latency.
 *
 * Without a reachable server every case reports "skipped" instead of failing -
 * the same contract as the TLS cases in `HttpBenchmark.cpp`. In CI the
 * `benchmarks` job provides a `postgres:17` service; locally
 * `PGHOST`/`PGPORT`/`PGUSER`/`PGPASSWORD`/`PGDATABASE` default to the
 * `docker compose up postgres` credentials (127.0.0.1:5432, test/test/test).
 */

namespace lintel {
namespace {

  constexpr int kBatchInserts = 100;
  constexpr int kCursorRows = 10000;
  constexpr int kNotifyIterations = 50;

  const char *const kNoServer = "no PostgreSQL server reachable (start one with 'docker compose up postgres')";

  /** @return the environment value or @p fallback when unset/empty */
  std::string envOr(const char *name, const char *fallback)
  {
    const char *value = std::getenv(name);
    return (value != nullptr && *value != '\0') ? value : fallback;
  }

  /** @return a process-unique suffix so parallel runs cannot collide */
  std::string uniqueSuffix()
  {
#if defined(_WIN32)
    return std::to_string(static_cast<long>(_getpid()));
#else
    return std::to_string(static_cast<long>(::getpid()));
#endif
  }

  /** @return a libpq connection string built from the environment */
  std::string pgConnInfo()
  {
    std::string info = "hostaddr=";
    info += envOr("PGHOST", "127.0.0.1");
    info += " port=";
    info += envOr("PGPORT", "5432");
    info += " user=";
    info += envOr("PGUSER", "test");
    info += " password=";
    info += envOr("PGPASSWORD", "test");
    info += " dbname=";
    info += envOr("PGDATABASE", "test");
    return info;
  }

  /** @return the connection entry the pool and the direct benchmarks build from */
  std::shared_ptr<DatabaseConnectionEntry> pgEntry()
  {
    return std::make_shared<DatabaseConnectionEntry>("bench_pg",
      envOr("PGHOST", "127.0.0.1"),
      envOr("PGUSER", "test"),
      envOr("PGPASSWORD", "test"),
      db::ConnectionType::PostgreSQL,
      "bench_pg",
      std::atoi(envOr("PGPORT", "5432").c_str()),
      envOr("PGDATABASE", "test"),
      true);
  }

  /** @return true once a PostgreSQL server answered (probed once per process) */
  bool pgAvailable()
  {
    static const bool available = [] {
      try {
        db::Connection probe(db::ConnectionType::PostgreSQL, pgConnInfo());
        return true;
      } catch (const std::exception &) {
        return false;
      }
    }();
    return available;
  }

  /**
   * Base fixture: drops and creates the scratch table in `SetUp`, drops it again
   * in `TearDown`. Both run outside the timed loop (google/benchmark fixtures
   * surround the iteration loop), so the setup is never measured.
   */
  class PgBenchmark : public benchmark::Fixture
  {
  public:
    void SetUp(const benchmark::State &) override
    {
      if (!pgAvailable()) { return; }
      m_available = true;
      m_entry = pgEntry();
      m_table = "bench_pg_" + uniqueSuffix();
      withStatement([this](db::Statement &statement) {
        statement.execute("drop table if exists " + m_table);
        statement.execute(
          "create table " + m_table + " (id serial primary key, name text not null, value integer not null)");
      });
      onCreate();
    }

    void TearDown(const benchmark::State &) override
    {
      if (!m_available) { return; }
      try {
        withStatement([&](db::Statement &statement) { statement.execute("drop table if exists " + m_table); });
      } catch (...) {
        // A destructor must not throw; a leftover table is harmless because
        // the name carries the pid and the next run drops it again.
      }
    }

    [[nodiscard]] bool available() const { return m_available; }

  protected:
    /** Hook for derived fixtures that need rows in the table. */
    virtual void onCreate() {}

    /** Runs @p function on a short-lived connection + statement. */
    template<class Function> void withStatement(Function &&function)
    {
      db::Connection connection(m_entry);
      db::Statement statement(connection);
      function(statement);
    }

    /** Seeds @p rowCount rows without a round-trip per row. */
    void seedRows(std::size_t rowCount)
    {
      withStatement([this, rowCount](db::Statement &statement) {
        statement.execute("insert into " + m_table + " (name, value) select 'row ' || g, g from generate_series(1, "
                          + std::to_string(rowCount) + ") as g");
      });
    }

    bool m_available = false;
    std::shared_ptr<DatabaseConnectionEntry> m_entry;
    std::string m_table;
  };

  /** Fixture with one persistent connection shared by every iteration. */
  class PgConnectionBenchmark : public PgBenchmark
  {
  public:
    void SetUp(const benchmark::State &state) override
    {
      PgBenchmark::SetUp(state);
      if (m_available) { m_connection = std::make_unique<db::Connection>(m_entry); }
    }

  protected:
    std::unique_ptr<db::Connection> m_connection;
  };

  /** Skips the case when the fixture could not reach a server. */
  bool requireServer(benchmark::State &state, const PgBenchmark &fixture)
  {
    if (!fixture.available()) {
      state.SkipWithMessage(kNoServer);
      return false;
    }
    return true;
  }

  // ── Cursor batches ─────────────────────────────────────────────────────

  /** Seeds the 10k-row table once; the batch size varies per registered Arg. */
  class PgCursorBenchmark : public PgConnectionBenchmark
  {
  protected:
    void onCreate() override { seedRows(kCursorRows); }
  };

  BENCHMARK_DEFINE_F(PgCursorBenchmark, FetchSize)(benchmark::State &state)
  {
    if (!requireServer(state, *this)) { return; }
    for (auto _ : state) {
      db::Statement statement(*m_connection);
      db::Cursor cursor = statement.executeCursor("select name, value from " + m_table + " order by id");
      cursor.setFetchSize(static_cast<std::size_t>(state.range(0)));
      std::size_t rows = 0;
      for (;;) {
        std::vector<db::Arguments> batch = cursor.fetchNext(1000);
        if (batch.empty()) { break; }
        rows += batch.size();
        benchmark::DoNotOptimize(batch.data());
      }
      benchmark::DoNotOptimize(rows);
    }
    state.counters["rows"] = kCursorRows;
    state.counters["rows_per_round_trip"] = static_cast<double>(state.range(0));
  }
  BENCHMARK_REGISTER_F(PgCursorBenchmark, FetchSize)->Arg(1)->Arg(10)->Arg(100)->Arg(1000);

  // ── Transaction batches ────────────────────────────────────────────────

  BENCHMARK_DEFINE_F(PgConnectionBenchmark, TransactionBatch)(benchmark::State &state)
  {
    if (!requireServer(state, *this)) { return; }
    db::PreparedStatement statement(
      *m_connection, "insert into " + m_table + " (name, value) values (?, ?)", "bench_txn_batch");
    for (auto _ : state) {
      db::Transaction transaction(*m_connection);
      for (int i = 0; i < kBatchInserts; i++) {
        db::ParameterBuilder builder(m_entry);
        builder.add(std::string("row ")).add(i);
        statement.execute(builder);
      }
      transaction.commit();
    }
    state.counters["inserts"] = kBatchInserts;
  }

  BENCHMARK_DEFINE_F(PgConnectionBenchmark, IndividualTransactions)(benchmark::State &state)
  {
    if (!requireServer(state, *this)) { return; }
    db::PreparedStatement statement(
      *m_connection, "insert into " + m_table + " (name, value) values (?, ?)", "bench_single_insert");
    for (auto _ : state) {
      for (int i = 0; i < kBatchInserts; i++) {
        db::Transaction transaction(*m_connection);
        db::ParameterBuilder builder(m_entry);
        builder.add(std::string("row ")).add(i);
        statement.execute(builder);
        transaction.commit();
      }
    }
    state.counters["inserts"] = kBatchInserts;
  }

  // ── Prepared statement reuse ───────────────────────────────────────────
  BENCHMARK_DEFINE_F(PgConnectionBenchmark, PreparedStatementReuse)(benchmark::State &state)
  {
    if (!requireServer(state, *this)) { return; }
    db::PreparedStatement statement(
      *m_connection, "insert into " + m_table + " (name, value) values (?, ?)", "bench_reuse");
    int value = 0;
    for (auto _ : state) {
      db::ParameterBuilder builder(m_entry);
      builder.add(std::string("row ")).add(value++);
      statement.execute(builder);
    }
  }

  BENCHMARK_DEFINE_F(PgConnectionBenchmark, ParameterizedStatement)(benchmark::State &state)
  {
    if (!requireServer(state, *this)) { return; }
    int value = 0;
    for (auto _ : state) {
      db::Statement statement(*m_connection);
      db::ParameterBuilder builder(m_entry);
      builder.add(std::string("row ")).add(value++);
      statement.execute("insert into " + m_table + " (name, value) values (?, ?)", builder);
    }
  }
  BENCHMARK_REGISTER_F(PgConnectionBenchmark, TransactionBatch);
  BENCHMARK_REGISTER_F(PgConnectionBenchmark, IndividualTransactions);
  BENCHMARK_REGISTER_F(PgConnectionBenchmark, PreparedStatementReuse);
  BENCHMARK_REGISTER_F(PgConnectionBenchmark, ParameterizedStatement);

  // ── Pool acquire vs direct connection ──────────────────────────────────

  class PgPoolBenchmark : public PgBenchmark
  {
  public:
    void SetUp(const benchmark::State &state) override
    {
      PgBenchmark::SetUp(state);
      if (!m_available) { return; }
      db::PoolSettings settings;
      settings.maxConnections = static_cast<std::size_t>(state.range(0));
      settings.acquireTimeout = std::chrono::milliseconds(5000);
      m_pool = std::make_unique<db::ConnectionPool>(m_entry, settings);
      // Warm the pool up: the first acquire connects (and authenticates), which
      // must not land in the measured loop.
      auto lease = m_pool->acquire();
      benchmark::DoNotOptimize(lease.connection());
    }

  protected:
    std::unique_ptr<db::ConnectionPool> m_pool;
  };

  BENCHMARK_DEFINE_F(PgPoolBenchmark, AcquireRelease)(benchmark::State &state)
  {
    if (!requireServer(state, *this)) { return; }
    for (auto _ : state) {
      db::ConnectionPool::Lease lease = m_pool->acquire();
      benchmark::DoNotOptimize(lease.connection());
    }
  }
  BENCHMARK_REGISTER_F(PgPoolBenchmark, AcquireRelease)->Arg(1)->Arg(4)->Arg(16);

  BENCHMARK_DEFINE_F(PgPoolBenchmark, DirectConnection)(benchmark::State &state)
  {
    if (!requireServer(state, *this)) { return; }
    for (auto _ : state) {
      db::Connection connection(m_entry);
      benchmark::DoNotOptimize(connection);
    }
  }
  BENCHMARK_REGISTER_F(PgPoolBenchmark, DirectConnection);

  // ── Notify latency ─────────────────────────────────────────────────────

  void BM_Pg_NotifyRoundTrip(benchmark::State &state)
  {
    if (!pgAvailable()) {
      state.SkipWithMessage(kNoServer);
      return;
    }
    const std::shared_ptr<DatabaseConnectionEntry> entry = pgEntry();
    const std::string channel = "bench_pg_notify_" + uniqueSuffix();

    db::Connection listener(entry);
    std::mutex mutex;
    std::condition_variable condition;
    std::uint64_t delivered = 0;
    db::Notify notify(
      listener,
      [&] {
        {
          std::lock_guard<std::mutex> lock(mutex);
          ++delivered;
        }
        condition.notify_all();
      },
      channel);
    // Attach the LISTEN before the first NOTIFY: the callback runs on the
    // listener's own thread, so the round-trip is awaited through the mutex.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    db::Connection writer(entry);
    db::Statement statement(writer);
    std::uint64_t sent = 0;
    for (auto _ : state) {
      const auto start = std::chrono::steady_clock::now();
      ++sent;
      statement.execute("notify " + channel);
      {
        std::unique_lock<std::mutex> lock(mutex);
        if (!condition.wait_for(lock, std::chrono::seconds(5), [&] { return delivered >= sent; })) {
          state.SkipWithMessage("the NOTIFY round-trip did not complete within 5 seconds");
          return;
        }
      }
      state.SetIterationTime(std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count());
    }
  }
  BENCHMARK(BM_Pg_NotifyRoundTrip)->UseManualTime()->Iterations(kNotifyIterations);

}// namespace
}// namespace lintel
