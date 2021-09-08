#include "base_library/core/plugins/DatabaseBootstrapPlugin.h"

#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/Statement.h"
#include "base_library/core/persistence/Transaction.h"
#include "base_library/core/services/FileService.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/core/utils/StringUtils.h"

void DatabaseBootstrapPlugin::onStart() {
  for (const auto &connectionEntry : m_connectionConfigurations->allof()) {
    try {
      handle(connectionEntry);
    } catch (const db::SQLException &exception) {
      LOG_ERROR("{} - failed to ahndle database bootstrap plugin - {}", connectionEntry->getName(), exception.what());
      throw exception;
    } catch (std::exception &exception) {
      LOG_ERROR("{} - failed to handle database bootstrap plugin - {}",
                connectionEntry->getName(), exception.what());
    }
  }
}
void DatabaseBootstrapPlugin::handle(
    const std::shared_ptr<ConnectionEntry> &connectionEntry) {
  auto optResult = hasSchemaVersionTable(connectionEntry);
  std::filesystem::path dbPath = m_configPath.string() +
                                 std::filesystem::path::preferred_separator +
                                 connectionEntry->getName();
  if (!optResult.has_value()) {
    createSchemaVersionTable(connectionEntry);
  }
  if (!std::filesystem::exists(dbPath)) {
    LOG_WARN("{} - {} does not exists", connectionEntry->getName(),
             dbPath.string());
    return;
  }
  if (!std::filesystem::is_directory(dbPath)) {
    LOG_WARN("{} - {} is not directory", connectionEntry->getName(),
             dbPath.string());
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
      schemaVersions.insert(
          {iter.of("name").getValue(), iter.of("version").getValue<int32_t>()});
    }
  }
  std::map<std::string, std::vector<FileInformation>> files;
  std::function<void(const std::filesystem::path &)> handleFile =
      [&](const std::filesystem::path &path) {
        // check file type
        bool isInitFile =
            StringUtils::startsWith(path.filename().string(), "init_");
        bool isDataFile =
            StringUtils::startsWith(path.filename().string(), "data_");
        // exception handling for unknown files
        if (!isInitFile && !isDataFile) {
          LOG_WARN("{} - {} is no init or data file -> ignore",
                   connectionEntry->getName(), path.string());
          return;
        }
        // get information of filename
        std::string schemaName = schemaNameOf(path);
        int32_t version = versionOf(path);
        FileInformation fileInformation{schemaName, version, path, isInitFile};
        auto found = files.find(schemaName);
        if (found == files.end()) {
          std::vector<FileInformation> temp{fileInformation};
          files.insert({schemaName, temp});
          return;
        }
        found->second.push_back(fileInformation);
      };
  for (auto &iter : std::filesystem::directory_iterator(dbPath)) {
    handleFile(iter.path());
  }
  initDatabase(connectionEntry, files, schemaVersions);
}
BootstrapSequence DatabaseBootstrapPlugin::getPriority() {
  return {BootstrapSequence::Database};
}
DatabaseBootstrapPlugin::DatabaseBootstrapPlugin(
    std::shared_ptr<ConnectionConfigurations> connectionConfigurations)
    : m_connectionConfigurations(std::move(connectionConfigurations)) {}
std::optional<db::Result> DatabaseBootstrapPlugin::hasSchemaVersionTable(
    const std::shared_ptr<ConnectionEntry> &connectionEntry) {
  try {
    db::Connection connection(connectionEntry);
    db::Statement statement(connection);
    db::Result result =
        statement.execute("select name, version from schema_version");
    return std::make_optional(result);
  } catch (std::exception &exception) {
    LOG_INFO("{} - schema_version table does not exists or another error {}",
             connectionEntry->getName(), exception.what());
  }
  return std::nullopt;
}
void DatabaseBootstrapPlugin::createSchemaVersionTable(
    const std::shared_ptr<ConnectionEntry> &connectionEntry) {
  // don't handle for database exception to make sure that bootstrap procedure
  // aborts for database connection
  db::Connection connection(connectionEntry);
  db::Statement statement(connection);
  statement.execute(std::string(m_createSchemaVersion));
}
void DatabaseBootstrapPlugin::initDatabase(
    const std::shared_ptr<ConnectionEntry> &connectionEntry,
    const std::map<std::string, std::vector<FileInformation>> &initFiles,
    const std::map<std::string, int32_t> &schemaVersions) {
  for (const auto &[schemaName, inits] : initFiles) {
    auto found = schemaVersions.find(schemaName);
    int32_t schemaVersionStart = 0;
    if (found != schemaVersions.end()) {
      schemaVersionStart = found->second;
    }
    std::vector<FileInformation> fileInformations = inits;
    fileInformations.erase(
        std::remove_if(fileInformations.begin(), fileInformations.end(),
                       [&](const FileInformation &fileInformation) {
                         return fileInformation.m_version <= schemaVersionStart;
                       }),
        fileInformations.end());
    std::sort(fileInformations.begin(), fileInformations.end(),
              [](const FileInformation &a, const FileInformation &b) -> bool {
                if (a.m_version < b.m_version) {
                  return true;
                }
                return a.m_isInitFile && !b.m_isInitFile;
              });
    db::Connection connection(connectionEntry);
    int32_t currentSchemaVersion = 0;
    for (const auto &item : fileInformations) {
      if (item.m_isInitFile && currentSchemaVersion + 1 != item.m_version) {
        LOG_ERROR(
            "{} - initialization failed bc. of version jump between files",
            connectionEntry->getName());
        return;
      }
      if (!item.m_isInitFile && currentSchemaVersion != item.m_version) {
        LOG_ERROR(
            "{} - initialization failed bc. of data insertion for not "
            "available schema_version",
            connectionEntry->getName());
        return;
      }
      if (item.m_isInitFile) {
        currentSchemaVersion = item.m_version;
      }
      db::Transaction transaction(connection);
      FileService fileService(item.m_filePath);
      std::string content = fileService.readFile();
      content.erase(std::remove_if(content.begin(), content.end(),
                                   [](const char c) { return c == '\n'; }),
                    content.end());
      if (content.empty()) {
        LOG_ERROR("{} - initialization failed bc. of empty file",
                  connectionEntry->getName());
        return;
      }
      std::vector<std::string> tokens = StringUtils::split(content, ';');
      for (const auto &token : tokens) {
        db::Statement statement(connection);
        statement.execute(token);
      }
    }
    updateSchemaVersion(connection, schemaName, currentSchemaVersion,
                        schemaVersionStart == 0);
  }
}
std::string DatabaseBootstrapPlugin::schemaNameOf(const std::string &fileName) {
  auto beginPos = fileName.find("_schema_");
  auto endPos = fileName.find("_version_");
  if (beginPos == std::string::npos || endPos == std::string::npos) {
    return "";
  }
  return fileName.substr(beginPos + 8, endPos - beginPos - 8);
}
int32_t DatabaseBootstrapPlugin::versionOf(const std::string &fileName) {
  auto beginPos = fileName.find("_version_");
  if (beginPos == std::string::npos) {
    return -1;
  }
  return std::stoi(
      fileName.substr(beginPos + 9, fileName.length() - beginPos - 9));
}
void DatabaseBootstrapPlugin::updateSchemaVersion(
    const db::Connection &connection, const std::string &schemaName,
    int32_t schemaVersion, bool insert) {
  if (schemaVersion <= 0) {
    // no persistence
    return;
  }
  if (insert) {
    db::Statement statement(connection);
    statement.execute("insert into schema_version(name, version) values(?, ?)",
                      {schemaName, std::to_string(schemaVersion)});
    return;
  }
  db::Statement statement(connection);
  statement.execute("update schema_version set version = ? where name = ?",
                    {std::to_string(schemaVersion), schemaName});
}
