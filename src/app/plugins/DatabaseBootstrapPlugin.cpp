#include "lintel/app/plugins/DatabaseBootstrapPlugin.h"

#include <chrono>
#include <cstddef>

#include <lintel/core/persistence/ConnectionType.h>

#include "lintel/core/persistence/Connection.h"
#include "lintel/core/persistence/ConnectionPool.h"
#include "lintel/core/persistence/ConnectionPoolRegistry.h"
#include "lintel/core/persistence/ScopedConnection.h"
#include "lintel/core/persistence/Statement.h"
#include "lintel/core/persistence/Transaction.h"
#include "lintel/core/services/FileService.h"
#include "lintel/core/services/LoggerService.h"
#include "lintel/core/services/ReadinessRegistry.h"
#include "lintel/core/utils/StringUtils.h"

namespace lintel {
void DatabaseBootstrapPlugin::onStart()
{
  for (const auto &connectionEntry : m_connectionConfigurations->allOf()) {
    // register before handling: a failing bootstrap must still show up as an
    // unready dependency instead of silently missing from `GET /ready`
    registerConnectionPool(connectionEntry);
    registerReadinessCheck(connectionEntry);
    try {
      handle(connectionEntry);
    } catch (db::SQLException &exception) {
      LOG_ERROR("{} - failed to handle database bootstrap plugin - {}", connectionEntry->getName(), exception.what());
    } catch (std::exception &exception) {
      LOG_ERROR("{} - failed to handle database bootstrap plugin - {}", connectionEntry->getName(), exception.what());
    }
  }
}

void DatabaseBootstrapPlugin::registerReadinessCheck(const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry)
{
  ReadinessRegistry::instance().registerCheck(
    "database:" + connectionEntry->getName(), [connectionEntry](std::string &detail) {
      try {
        // SELECT 1 is portable across PostgreSQL and SQLite and needs no schema
        auto &registry = db::ConnectionPoolRegistry::instance();
        if (registry.hasPool(connectionEntry->getName())) {
          auto lease = registry.poolOf(connectionEntry->getName())->acquire();
          db::Statement statement(lease.connection());
          statement.execute("SELECT 1");
          return true;
        }
        db::ScopedConnection scopedConnection(connectionEntry);
        db::Statement statement(scopedConnection.connection());
        statement.execute("SELECT 1");
        return true;
      } catch (const std::exception &exception) {
        detail = exception.what();
        return false;
      }
    });
}

void DatabaseBootstrapPlugin::registerConnectionPool(const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry)
{
  const int32_t poolSize = connectionEntry->getPoolSize();
  if (poolSize <= 0) {
    LOG_INFO(
      "{} - connection pooling disabled (poolSize={}); no pool registered", connectionEntry->getName(), poolSize);
    return;
  }
  db::PoolSettings settings;
  settings.maxConnections = static_cast<std::size_t>(poolSize);
  settings.acquireTimeout = std::chrono::milliseconds(connectionEntry->getPoolAcquireTimeoutMs());
  auto pool = std::make_shared<db::ConnectionPool>(connectionEntry, settings);
  db::ConnectionPoolRegistry::instance().registerPool(connectionEntry->getName(), std::move(pool));
  LOG_INFO("{} - connection pool registered (poolSize={}, acquireTimeout={} ms)",
    connectionEntry->getName(),
    poolSize,
    connectionEntry->getPoolAcquireTimeoutMs());
}

void DatabaseBootstrapPlugin::handle(const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry)
{
  if (connectionEntry->getType() == db::ConnectionType::SQLite) {
    // open close connection for indirectly create of sqlite database
    db::ScopedConnection scopedConnection(connectionEntry);
  }
  auto optResult = hasSchemaVersionTable(connectionEntry);
  std::filesystem::path dbPath = m_configPath / connectionEntry->getName();
  if (!optResult.has_value()) { createSchemaVersionTable(connectionEntry); }
  if (!std::filesystem::exists(dbPath)) {
    LOG_WARN("{} - {} does not exists", connectionEntry->getName(), dbPath.string());
    return;
  }
  if (!std::filesystem::is_directory(dbPath)) {
    LOG_WARN("{} - {} is not directory", connectionEntry->getName(), dbPath.string());
    return;
  }
  if (std::filesystem::is_empty(dbPath)) {
    LOG_WARN("{} - {} is empty", connectionEntry->getName(), dbPath.string());
    return;
  }
  std::map<std::string, int32_t> schemaVersions;
  if (optResult.has_value()) {
    db::Result result = optResult.value();
    for (const auto &iter : result) {
      schemaVersions.insert({ iter.of("name").getValue(), iter.of("version").getValue<int32_t>() });
    }
  }
  std::map<std::string, std::vector<FileInformation>> files;
  std::function<void(const std::filesystem::path &)> handleFile = [&](const std::filesystem::path &path) {
    // check file type
    bool isInitFile = StringUtils::startsWith(path.filename().string(), "init_");
    bool isDataFile = StringUtils::startsWith(path.filename().string(), "data_");
    // exception handling for unknown files
    if (!isInitFile && !isDataFile) {
      LOG_WARN("{} - {} is no init or data file -> ignore", connectionEntry->getName(), path.string());
      return;
    }
    // get information of filename
    std::string schemaName = schemaNameOf(path.string());
    int32_t version = versionOf(path.string());
    FileInformation fileInformation{ schemaName, version, path, isInitFile };
    auto found = files.find(schemaName);
    if (found == files.end()) {
      std::vector<FileInformation> temp{ fileInformation };
      files.insert({ schemaName, temp });
      return;
    }
    found->second.push_back(fileInformation);
  };
  for (auto &iter : std::filesystem::directory_iterator(dbPath)) { handleFile(iter.path()); }
  initDatabase(connectionEntry, files, schemaVersions);
}

BootstrapSequence DatabaseBootstrapPlugin::getPriority() { return { BootstrapSequence::Database }; }

DatabaseBootstrapPlugin::DatabaseBootstrapPlugin(
  std::shared_ptr<DatabaseConnectionConfigurations> connectionConfigurations)
  : m_connectionConfigurations(std::move(connectionConfigurations))
{}

std::optional<db::Result> DatabaseBootstrapPlugin::hasSchemaVersionTable(
  const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry)
{
  try {
    db::ScopedConnection scopedConnection(connectionEntry);
    db::Statement statement(scopedConnection.connection());
    db::Result result = statement.execute("select name, version from schema_version");
    return std::make_optional(result);
  } catch (std::exception &exception) {
    LOG_INFO(
      "{} - schema_version table does not exists or another error {}", connectionEntry->getName(), exception.what());
  }
  return std::nullopt;
}

void DatabaseBootstrapPlugin::createSchemaVersionTable(const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry)
{
  // don't handle for database exception to make sure that bootstrap procedure
  // aborts for database connection
  db::ScopedConnection scopedConnection(connectionEntry);
  db::Statement statement(scopedConnection.connection());
  statement.execute(std::string(m_createSchemaVersion));
}

void DatabaseBootstrapPlugin::executeInitFile(db::Connection &connection,
  const FileInformation &item,
  const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry,
  int32_t &currentSchemaVersion)
{
  if (item.m_isInitFile) { currentSchemaVersion = item.m_version; }
  db::Transaction transaction(connection);
  FileService fileService(item.m_filePath);
  std::string content = fileService.readFile();
  content.erase(std::remove_if(content.begin(), content.end(), [](const char c) { return c == '\n'; }), content.end());
  if (content.empty()) {
    LOG_ERROR("{} - initialization failed bc. of empty file", connectionEntry->getName());
    return;
  }
  std::vector<std::string> tokens = StringUtils::split(content, ';');
  for (const auto &token : tokens) {
    db::Statement statement(connection);
    statement.execute(token);
  }
  transaction.commit();
}

void DatabaseBootstrapPlugin::initDatabase(const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry,
  const std::map<std::string, std::vector<FileInformation>> &initFiles,
  const std::map<std::string, int32_t> &schemaVersions)
{
  for (const auto &[schemaName, inits] : initFiles) {
    auto found = schemaVersions.find(schemaName);
    int32_t schemaVersionStart = 0;
    if (found != schemaVersions.end()) { schemaVersionStart = found->second; }
    std::vector<FileInformation> fileInformations = inits;
    fileInformations.erase(
      std::remove_if(fileInformations.begin(),
        fileInformations.end(),
        [&](const FileInformation &fileInformation) { return fileInformation.m_version <= schemaVersionStart; }),
      fileInformations.end());
    // Strict weak ordering: sort by version first, and only within the same version run the
    // init file before the data file. Comparing only "init before data" as a tie-breaker after
    // the version comparison is not a strict weak ordering and is undefined behaviour for
    // std::sort.
    std::sort(
      fileInformations.begin(), fileInformations.end(), [](const FileInformation &a, const FileInformation &b) -> bool {
        if (a.m_version != b.m_version) { return a.m_version < b.m_version; }
        return a.m_isInitFile && !b.m_isInitFile;
      });
    db::ScopedConnection scopedConnection(connectionEntry);
    // Continue from the recorded version so that higher-versioned files form a migration path
    // for an already initialised database. Starting at 0 made the first init file of an upgrade
    // look like a version jump and silently dropped the migration.
    int32_t currentSchemaVersion = schemaVersionStart;
    for (const auto &item : fileInformations) {
      if (item.m_isInitFile && currentSchemaVersion + 1 != item.m_version) {
        LOG_ERROR("{} - initialization failed bc. of version jump between files", connectionEntry->getName());
        return;
      }
      if (!item.m_isInitFile && currentSchemaVersion != item.m_version) {
        LOG_ERROR(
          "{} - initialization failed bc. of data insertion for not "
          "available schema_version",
          connectionEntry->getName());
        return;
      }
      executeInitFile(scopedConnection.connection(), item, connectionEntry, currentSchemaVersion);
    }
    updateSchemaVersion(
      scopedConnection.connection(), connectionEntry, schemaName, currentSchemaVersion, schemaVersionStart == 0);
  }
}

std::string DatabaseBootstrapPlugin::schemaNameOf(const std::string &fileName)
{
  auto beginPos = fileName.find("_schema_");
  auto endPos = fileName.find("_version_");
  if (beginPos == std::string::npos || endPos == std::string::npos) { return ""; }
  return fileName.substr(beginPos + 8, endPos - beginPos - 8);
}

int32_t DatabaseBootstrapPlugin::versionOf(const std::string &fileName)
{
  auto beginPos = fileName.find("_version_");
  if (beginPos == std::string::npos) { return -1; }
  return std::stoi(fileName.substr(beginPos + 9, fileName.length() - beginPos - 9));
}

void DatabaseBootstrapPlugin::updateSchemaVersion(const db::Connection &connection,
  const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry,
  const std::string &schemaName,
  int32_t schemaVersion,
  bool insert)
{
  if (schemaVersion <= 0) {
    // no persistence
    return;
  }
  if (insert) {
    db::Statement statement(connection);
    db::ParameterBuilder builder(connectionEntry);
    builder.add(schemaName).add(schemaVersion);
    statement.execute("insert into schema_version(name, version) values(?, ?)", builder);
    return;
  }
  db::Statement statement(connection);
  db::ParameterBuilder builder(connectionEntry);
  builder.add(schemaVersion).add(schemaName);
  statement.execute("update schema_version set version = ? where name = ?", builder);
}
}// namespace lintel
