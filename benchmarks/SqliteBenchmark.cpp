#include <benchmark/benchmark.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/configuration/DatabaseConnectionComponent.h"
#include "lintel/core/configuration/DatabaseConnectionEntry.h"
#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/core/persistence/Connection.h"
#include "lintel/core/persistence/ConnectionPool.h"
#include "lintel/core/persistence/DatabaseConnectionConfigurations.h"
#include "lintel/core/persistence/ParameterBuilder.h"
#include "lintel/core/persistence/PreparedStatement.h"
#include "lintel/core/persistence/Serialization.h"
#include "lintel/core/persistence/Statement.h"
#include "lintel/core/persistence/Transaction.h"
#include "lintel/core/persistence/sqlite3/Connection.h"
#include "lintel/core/persistence/sqlite3/PreparedStatement.h"
#include "lintel/core/persistence/sqlite3/Statement.h"
#include "lintel/core/persistence/sqlite3/Transaction.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/models/Group.h"
#include "lintel/features/base/models/User.h"
#include "lintel/features/base/repositories/GroupRepository.h"
#include "lintel/features/base/repositories/UserRepository.h"

namespace lintel {
static const char *kBenchDbPath = "/tmp/bench_lintel.db";

static void RemoveBenchDb() { std::remove(kBenchDbPath); }

static void CreateBenchTable(sqlite::Connection &conn)
{
  sqlite::Statement stmt(conn);
  stmt.execute(
    "CREATE TABLE IF NOT EXISTS bench_data ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT NOT NULL,"
    "value TEXT NOT NULL,"
    "category TEXT NOT NULL)");
}

static void SeedBenchData(sqlite::Connection &conn, int count)
{
  sqlite::Transaction txn(conn);
  txn.start();
  sqlite::PreparedStatement stmt(conn, "INSERT INTO bench_data (name, value, category) VALUES (?, ?, ?)", "bench_seed");
  for (int i = 0; i < count; ++i) { stmt.execute({ "name_" + std::to_string(i), std::to_string(i), "cat" }); }
  txn.commit();
}

static void BM_Insert_Single(benchmark::State &state)
{
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

static void BM_Insert_Prepared(benchmark::State &state)
{
  RemoveBenchDb();
  for (auto _ : state) {
    state.PauseTiming();
    sqlite::Connection conn(kBenchDbPath);
    CreateBenchTable(conn);
    state.ResumeTiming();
    sqlite::PreparedStatement stmt(
      conn, "INSERT INTO bench_data (name, value, category) VALUES (?, ?, ?)", "bench_insert");
    stmt.execute({ "test", "42", "benchmark" });
  }
  RemoveBenchDb();
}
BENCHMARK(BM_Insert_Prepared);

static void BM_Insert_PreparedReused(benchmark::State &state)
{
  RemoveBenchDb();
  sqlite::Connection conn(kBenchDbPath);
  CreateBenchTable(conn);
  sqlite::PreparedStatement stmt(
    conn, "INSERT INTO bench_data (name, value, category) VALUES (?, ?, ?)", "bench_insert_reused");
  for (auto _ : state) { stmt.execute({ "test", "42", "benchmark" }); }
  RemoveBenchDb();
}
BENCHMARK(BM_Insert_PreparedReused);

static void BM_Insert_Batch100(benchmark::State &state)
{
  RemoveBenchDb();
  for (auto _ : state) {
    state.PauseTiming();
    sqlite::Connection conn(kBenchDbPath);
    CreateBenchTable(conn);
    state.ResumeTiming();
    sqlite::Transaction txn(conn);
    txn.start();
    sqlite::PreparedStatement stmt(
      conn, "INSERT INTO bench_data (name, value, category) VALUES (?, ?, ?)", "bench_batch");
    for (int i = 0; i < 100; ++i) { stmt.execute({ "name_" + std::to_string(i), std::to_string(i), "cat" }); }
    txn.commit();
  }
  RemoveBenchDb();
}
BENCHMARK(BM_Insert_Batch100);

static void BM_Insert_Batch1000(benchmark::State &state)
{
  RemoveBenchDb();
  for (auto _ : state) {
    state.PauseTiming();
    sqlite::Connection conn(kBenchDbPath);
    CreateBenchTable(conn);
    state.ResumeTiming();
    sqlite::Transaction txn(conn);
    txn.start();
    sqlite::PreparedStatement stmt(
      conn, "INSERT INTO bench_data (name, value, category) VALUES (?, ?, ?)", "bench_batch1k");
    for (int i = 0; i < 1000; ++i) { stmt.execute({ "name_" + std::to_string(i), std::to_string(i), "cat" }); }
    txn.commit();
  }
  RemoveBenchDb();
}
BENCHMARK(BM_Insert_Batch1000);

static void BM_Select_All(benchmark::State &state)
{
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

static void BM_Select_Where(benchmark::State &state)
{
  RemoveBenchDb();
  {
    sqlite::Connection conn(kBenchDbPath);
    CreateBenchTable(conn);
    SeedBenchData(conn, 1000);
  }
  for (auto _ : state) {
    sqlite::Connection conn(kBenchDbPath);
    sqlite::PreparedStatement stmt(conn, "SELECT * FROM bench_data WHERE category = ?", "bench_select_where");
    stmt.execute({ "cat" });
  }
  RemoveBenchDb();
}
BENCHMARK(BM_Select_Where);

static void BM_Select_PreparedReused(benchmark::State &state)
{
  RemoveBenchDb();
  {
    sqlite::Connection conn(kBenchDbPath);
    CreateBenchTable(conn);
    SeedBenchData(conn, 1000);
  }
  sqlite::Connection conn(kBenchDbPath);
  sqlite::PreparedStatement stmt(conn, "SELECT * FROM bench_data WHERE name = ?", "bench_select_reused");
  for (auto _ : state) { stmt.execute({ "name_500" }); }
  RemoveBenchDb();
}
BENCHMARK(BM_Select_PreparedReused);

static void BM_Insert_Transaction100(benchmark::State &state)
{
  RemoveBenchDb();
  for (auto _ : state) {
    state.PauseTiming();
    sqlite::Connection conn(kBenchDbPath);
    CreateBenchTable(conn);
    state.ResumeTiming();
    sqlite::Transaction txn(conn);
    txn.start();
    sqlite::PreparedStatement stmt(
      conn, "INSERT INTO bench_data (name, value, category) VALUES (?, ?, ?)", "bench_txn");
    for (int i = 0; i < 100; ++i) { stmt.execute({ "name_" + std::to_string(i), std::to_string(i), "cat" }); }
    txn.commit();
  }
  RemoveBenchDb();
}
BENCHMARK(BM_Insert_Transaction100);

namespace {

  /** @return a process-unique file path for a benchmark database */
  std::string sqliteBenchPath(const std::string &tag)
  {
    const std::string suffix =
#if defined(_WIN32)
      std::to_string(static_cast<long>(_getpid()));
#else
      std::to_string(static_cast<long>(::getpid()));
#endif
    return (std::filesystem::temp_directory_path() / (tag + "_" + suffix + ".db")).string();
  }

  /** Removes a SQLite database file and its WAL sidecars. */
  void removeSqliteDatabase(const std::string &path)
  {
    std::remove(path.c_str());
    std::remove((path + "-wal").c_str());
    std::remove((path + "-shm").c_str());
  }

  /** @return a connection entry for a SQLite file database */
  std::shared_ptr<DatabaseConnectionEntry> sqliteEntry(const std::string &path)
  {
    return std::make_shared<DatabaseConnectionEntry>(
      type_name<DatabaseConnectionComponent>(), path, "", "", db::ConnectionType::SQLite, "bench_sqlite", 0, "", true);
  }

  // ── Pool acquire vs direct connection ─────────────────────────────────

  /**
   * Prices the two ways to get a connection.
   *
   * A direct `db::Connection` pays `sqlite3_open` plus the PRAGMAs
   * (`busy_timeout`, `journal_mode=WAL`, `foreign_keys`) on every call; a pool
   * lease is a mutex hand-off plus the `SELECT 1` health check on release. The
   * difference is what phase 1 (PRAGMAs) and phase 2 (pool) bought.
   */
  class SqliteConnectionBenchmark : public benchmark::Fixture
  {
  public:
    void SetUp(const benchmark::State &) override
    {
      m_dbPath = sqliteBenchPath("lintel_bench_pool");
      removeSqliteDatabase(m_dbPath);
      m_entry = sqliteEntry(m_dbPath);
      {
        // Create the database file and its WAL before the timing loop, so the
        // direct-connection case does not pay the first-creation cost either.
        db::Connection connection(m_entry);
      }
      db::PoolSettings settings;
      settings.maxConnections = 1;
      m_pool = std::make_unique<db::ConnectionPool>(m_entry, settings);
      auto lease = m_pool->acquire();
      benchmark::DoNotOptimize(lease.connection());
    }

    void TearDown(const benchmark::State &) override
    {
      m_pool.reset();
      removeSqliteDatabase(m_dbPath);
    }

  protected:
    std::string m_dbPath;
    std::shared_ptr<DatabaseConnectionEntry> m_entry;
    std::unique_ptr<db::ConnectionPool> m_pool;
  };

  BENCHMARK_DEFINE_F(SqliteConnectionBenchmark, DirectConnection)(benchmark::State &state)
  {
    for (auto _ : state) {
      db::Connection connection(m_entry);
      benchmark::DoNotOptimize(connection);
    }
  }

  BENCHMARK_DEFINE_F(SqliteConnectionBenchmark, PooledAcquire)(benchmark::State &state)
  {
    for (auto _ : state) {
      db::ConnectionPool::Lease lease = m_pool->acquire();
      benchmark::DoNotOptimize(lease.connection());
    }
  }

  BENCHMARK_REGISTER_F(SqliteConnectionBenchmark, DirectConnection);
  BENCHMARK_REGISTER_F(SqliteConnectionBenchmark, PooledAcquire);

  // ── WAL: writer with and without a concurrent reader ───────────────────

  /**
   * The WAL default from phase 1 in action: a reader on a second connection
   * keeps running while a writer commits its batch, instead of blocking on the
   * write lock (or failing with `SQLITE_BUSY` before `busy_timeout` existed).
   */
  class SqliteWalBenchmark : public benchmark::Fixture
  {
  public:
    void SetUp(const benchmark::State &) override
    {
      m_dbPath = sqliteBenchPath("lintel_bench_wal");
      removeSqliteDatabase(m_dbPath);
      m_entry = sqliteEntry(m_dbPath);
      db::Connection connection(m_entry);
      db::Statement statement(connection);
      statement.execute("create table bench_wal (name text not null, value integer not null)");
    }

    void TearDown(const benchmark::State &) override { removeSqliteDatabase(m_dbPath); }

  protected:
    /** Inserts @p count rows in one transaction on a fresh connection. */
    std::chrono::steady_clock::duration writeBatch(std::size_t count)
    {
      const auto start = std::chrono::steady_clock::now();
      db::Connection connection(m_entry);
      db::Statement statement(connection);
      db::Transaction transaction(connection);
      for (std::size_t i = 0; i < count; ++i) {
        statement.execute("insert into bench_wal (name, value) values ('row', " + std::to_string(i) + ")");
      }
      transaction.commit();
      return std::chrono::steady_clock::now() - start;
    }

    std::string m_dbPath;
    std::shared_ptr<DatabaseConnectionEntry> m_entry;
  };

  BENCHMARK_DEFINE_F(SqliteWalBenchmark, Writer)(benchmark::State &state)
  {
    std::chrono::steady_clock::duration elapsed{};
    for (auto _ : state) { elapsed = writeBatch(100); }
    benchmark::DoNotOptimize(elapsed);
  }

  BENCHMARK_DEFINE_F(SqliteWalBenchmark, WriterWithConcurrentReader)(benchmark::State &state)
  {
    for (auto _ : state) {
      std::atomic<bool> stop{ false };
      std::atomic<std::uint64_t> reads{ 0 };
      std::thread reader([this, &stop, &reads] {
        db::Connection connection(m_entry);
        db::Statement statement(connection);
        while (!stop.load(std::memory_order_relaxed)) {
          statement.execute("select count(*) from bench_wal");
          reads.fetch_add(1, std::memory_order_relaxed);
        }
      });
      const auto elapsed = writeBatch(100);
      stop.store(true, std::memory_order_relaxed);
      reader.join();
      state.SetIterationTime(std::chrono::duration<double>(elapsed).count());
      state.counters["reader_reads"] = static_cast<double>(reads.load());
    }
  }
  BENCHMARK_REGISTER_F(SqliteWalBenchmark, Writer)->UseManualTime()->Iterations(10);
  BENCHMARK_REGISTER_F(SqliteWalBenchmark, WriterWithConcurrentReader)->UseManualTime()->Iterations(10);

  // ── UserRepository::allOf: batched groups (phase 6) vs the N+1 path ────

  constexpr int kBenchUsers = 1000;
  constexpr int kBenchGroups = 3;

  /**
   * Replays `appendUsersOf()` as it looked before commit 8261aad: it resolved
   * every relation row through `groupRepository->of()` (one `std::optional`
   * plus one `Group` copy per row). The current version collects the distinct
   * names once, loads `allOf()` once and resolves the rows from a map.
   *
   * `GroupRepository::of()` is an in-memory lookup over the `onAwake()` cache,
   * so the delta is CPU work in the aggregation, not database round-trips -
   * the same shape the ROADMAP/plan called "N+1" here.
   */
  void appendUsersLegacy(db::Result &result, GroupRepository &groupRepository, std::vector<User> &users)
  {
    std::string lastUserName;
    std::string lastFirstName;
    std::string lastLastName;
    std::string lastPassword;
    std::string lastEMail;
    std::chrono::sys_time<std::chrono::microseconds> lastCreatedTimestamp;
    std::vector<std::string> lastGroupNames;

    auto flush = [&] {
      std::vector<Group> groups;
      for (const auto &groupName : lastGroupNames) {
        const auto found = groupRepository.of(groupName);
        if (!found.has_value()) { continue; }
        groups.push_back(found.value());
      }
      users.emplace_back(lastFirstName,
        lastLastName,
        User::Sex::Male,
        lastEMail,
        lastUserName,
        lastPassword,
        groups,
        lastCreatedTimestamp);
    };
    for (auto &iter : result) {
      std::string groupName = iter.of(0).getValue();
      std::string firstName = iter.of(1).getValue();
      std::string lastName = iter.of(2).getValue();
      std::string eMail = iter.of(3).getValue();
      std::string password = iter.of(4).getValue();
      auto createdTimestamp = iter.of(5).getValue<std::chrono::sys_time<std::chrono::microseconds>>();
      std::string userName = iter.of(6).getValue();
      if (lastUserName != userName && !lastUserName.empty()) {
        flush();
        lastGroupNames.clear();
      }
      lastUserName = userName;
      if (!groupName.empty()) { lastGroupNames.push_back(groupName); }
      lastFirstName = firstName;
      lastLastName = lastName;
      lastEMail = eMail;
      lastPassword = password;
      lastCreatedTimestamp = createdTimestamp;
    }
    if (!lastUserName.empty()) { flush(); }
  }

  /** Builds a populated user/group database and both repositories. */
  class SqliteUserRepositoryBenchmark : public benchmark::Fixture
  {
  public:
    void SetUp(const benchmark::State &) override
    {
      m_dbPath = sqliteBenchPath("lintel_bench_users");
      removeSqliteDatabase(m_dbPath);
      m_entry = sqliteEntry(m_dbPath);

      auto environment = std::make_shared<EnvironmentConfiguration>();
      environment->overrides(EnvironmentConfiguration::ConfigDirectory, "/tmp/nonexistent_bench_cfg");
      auto configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, environment);
      configuration->setEntries(std::vector<std::shared_ptr<Entry>>{ m_entry });
      m_connectionConfigurations = std::make_shared<DatabaseConnectionConfigurations>(configuration);
      m_groupRepository = std::make_shared<GroupRepository>(m_connectionConfigurations);
      m_userRepository = std::make_shared<UserRepository>(m_connectionConfigurations, m_groupRepository);

      createSchema();
      seed();
      m_groupRepository->onAwake();
    }

    void TearDown(const benchmark::State &) override { removeSqliteDatabase(m_dbPath); }

  protected:
    void createSchema()
    {
      db::Connection connection(m_entry);
      db::Statement statement(connection);
      statement.execute(R"(
        CREATE TABLE users (
            first_name TEXT NOT NULL,
            last_name TEXT NOT NULL,
            e_mail TEXT NOT NULL,
            user_name TEXT NOT NULL,
            password TEXT NOT NULL,
            created_timestamp TEXT NOT NULL,
            CONSTRAINT user_pk PRIMARY KEY (user_name)
        );)");
      statement.execute(R"(
        CREATE TABLE groups (
            name text NOT NULL,
            virtual bool NOT NULL,
            CONSTRAINT group_pk PRIMARY KEY (name)
        );)");
      statement.execute(R"(
        CREATE TABLE user_groups_relation (
            user_name text NOT NULL,
            group_name text NOT NULL,
            CONSTRAINT user_groups_pk PRIMARY KEY (user_name, group_name)
        );)");
      statement.execute(R"(
        CREATE TABLE group_groups_relation (
            group_name text NOT NULL,
            base_group_name text NOT NULL,
            CONSTRAINT group_groups_relation_pk PRIMARY KEY (group_name, base_group_name)
        );)");
    }

    /** Seeds kBenchUsers users, each in kBenchGroups groups, in one transaction. */
    void seed()
    {
      db::Connection connection(m_entry);
      db::Statement statement(connection);
      db::Transaction transaction(connection);
      for (int i = 0; i < kBenchGroups; ++i) {
        statement.execute("insert into groups(name, virtual) values ('group" + std::to_string(i) + "', 0)");
      }
      db::PreparedStatement insertUser(connection,
        "insert into users(first_name, last_name, e_mail, user_name, password, created_timestamp) values (?, ?, ?, ?, "
        "?, ?)",
        "bench_seed_user");
      db::PreparedStatement insertRelation(
        connection, "insert into user_groups_relation(user_name, group_name) values (?, ?)", "bench_seed_relation");
      const auto now = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
      for (int u = 0; u < kBenchUsers; ++u) {
        const std::string userName = "user" + std::to_string(u);
        db::ParameterBuilder userBuilder(m_entry);
        userBuilder.add(std::string("First"))
          .add(std::string("Last"))
          .add(std::string("user@test"))
          .add(userName)
          .add(std::string("hash"))
          .add(now);
        insertUser.execute(userBuilder);
        for (int g = 0; g < kBenchGroups; ++g) {
          db::ParameterBuilder relationBuilder(m_entry);
          relationBuilder.add(userName).add("group" + std::to_string(g));
          insertRelation.execute(relationBuilder);
        }
      }
      transaction.commit();
    }

    std::string m_dbPath;
    std::shared_ptr<DatabaseConnectionEntry> m_entry;
    std::shared_ptr<DatabaseConnectionConfigurations> m_connectionConfigurations;
    std::shared_ptr<GroupRepository> m_groupRepository;
    std::shared_ptr<UserRepository> m_userRepository;
  };

  /** The current, batched path. */
  BENCHMARK_DEFINE_F(SqliteUserRepositoryBenchmark, AllOf)(benchmark::State &state)
  {
    std::size_t users = 0;
    for (auto _ : state) {
      auto result = m_userRepository->allOf();
      users = result.size();
      benchmark::DoNotOptimize(result.data());
    }
    state.counters["users"] = static_cast<double>(users);
  }

  /** The pre-phase-6 path, one `groupRepository->of()` per relation row. */
  BENCHMARK_DEFINE_F(SqliteUserRepositoryBenchmark, AllOfNPlus1)(benchmark::State &state)
  {
    std::size_t users = 0;
    for (auto _ : state) {
      db::Connection connection(m_entry);
      db::Statement statement(connection);
      db::Result result = statement.execute(R"(
            select ru.group_name,
                   u.first_name,
                   u.last_name,
                   u.e_mail,
                   u.password,
                   u.created_timestamp,
                   u.user_name
            from users u
            left join user_groups_relation ru
              on u.user_name = ru.user_name
            order by u.user_name asc)");
      std::vector<User> loaded;
      appendUsersLegacy(result, *m_groupRepository, loaded);
      users = loaded.size();
      benchmark::DoNotOptimize(loaded.data());
    }
    state.counters["users"] = static_cast<double>(users);
  }

  BENCHMARK_REGISTER_F(SqliteUserRepositoryBenchmark, AllOf);
  BENCHMARK_REGISTER_F(SqliteUserRepositoryBenchmark, AllOfNPlus1);

}// namespace

}// namespace lintel
