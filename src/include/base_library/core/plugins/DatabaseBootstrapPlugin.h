#ifndef CPP_BASE_LIBRARY_DATABASEBOOTSTRAPPLUGIN_H
#define CPP_BASE_LIBRARY_DATABASEBOOTSTRAPPLUGIN_H

#include <cstdint>
#include <filesystem>
#include <optional>

#include "base_library/config.h"
#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"
#include "base_library/core/persistence/Result.h"
#include "base_library/core/plugins/BootstrapPlugin.h"

/**
 * DatabaseBootstrapPlugin:
 *
 * The following plugin is for variant to configure / initialize  databases.
 *
 * Configuration Path and type of configuration files:
 * CONFIG_DIRECTORY / database / name-of-database
 * - init_schema_name_version_number.sql
 * - data_schema_name_version_number.sql
 *
 * The init files are responsible for the creation or changing of the structure
 * in the database itself. This means tables, trigger, etc.. Of course, also
 * table extensions, or just changes, deletion in the table itself.
 *
 * The data files are responsible for initialization of default data in the
 * database itself. For example for configurations. Note, that this is not a
 * way to just insert test data etc.. For this each database type have an
 * available cli tool. Just like postgresql or sqlite3 or gui tools like
 * DBeaver. For tracking the state of the initialization state the bootstrap
 * plugin will create automatically an schema_version table. This table will
 * keep track of initialized version.
 *
 * Note:
 *
 * The initialization works in the following way. The version number implies
 * the sequence of the files. The information in the schema_version table
 * implies the start point of the init and data files.
 *
 * We'll first init the current version and then execute the data include.
 */
class DatabaseBootstrapPlugin : public BootstrapPlugin {
private:
    struct FileInformation {
        std::string m_schemaName;
        int32_t m_version;
        std::filesystem::path m_filePath;
        bool m_isInitFile;
    };
    std::filesystem::path m_configPath =
            std::string(CONFIG_DIRECTORY) +
            std::filesystem::path::preferred_separator + "database";
    std::shared_ptr<DatabaseConnectionConfigurations> m_connectionConfigurations;

    static constexpr std::string_view m_createSchemaVersion =
            R"(CREATE TABLE schema_version (name TEXT NOT NULL PRIMARY KEY,version BIGINT NOT NULL);)";

    static std::optional<db::Result> hasSchemaVersionTable(
            const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry);

    static void createSchemaVersionTable(
            const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry);

    static void initDatabase(
            const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry,
            const std::map<std::string, std::vector<FileInformation>> &initFiles,
            const std::map<std::string, int32_t> &schemaVersions);

    static void updateSchemaVersion(
            const db::Connection &connection,
            const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry,
            const std::string &schemaName, int32_t schemaVersion, bool insert = true);

    static void executeInitFile(
            db::Connection &connection,
            const FileInformation &item,
            const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry,
            int32_t &currentSchemaVersion);

    static std::string schemaNameOf(const std::string &fileName);

    static int32_t versionOf(const std::string &fileName);

    void handle(const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry);

public:
    explicit DatabaseBootstrapPlugin(
            std::shared_ptr<DatabaseConnectionConfigurations>
            connectionConfigurations);

    void onStart() override;

    BootstrapSequence getPriority() override;
};

#endif  // CPP_BASE_LIBRARY_DATABASEBOOTSTRAPPLUGIN_H
