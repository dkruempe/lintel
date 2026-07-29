#include "base_library/features/property/repositories/DatabasePropertyRepository.h"

#include <base_library/core/exceptions/SQLException.h>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/ConnectionType.h"
#include "base_library/core/persistence/ParameterBuilder.h"
#include "base_library/core/persistence/PreparedStatement.h"
#include "base_library/core/persistence/Statement.h"
#include "base_library/core/persistence/Transaction.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/core/utils/StringUtils.h"
#include "base_library/features/property/factories/PropertyFactory.h"

DatabasePropertyRepository::DatabasePropertyRepository(
        std::shared_ptr<DatabaseConnectionConfigurations> connectionConfigurations,
        const std::shared_ptr<Configuration> &configuration,
        std::shared_ptr<ProcessName> processName)
        : PropertyRepository(PropertyRepositoryType::DATABASE_REPOSITORY,
                             configuration),
          m_connectionConfigurations(std::move(connectionConfigurations)),
          m_connectionEntry(m_connectionConfigurations->ofDefault()),
          m_processName(std::move(processName)) {}

DataStorage DatabasePropertyRepository::getDataStorage() {
    return m_currentDataStorage;
}

void DatabasePropertyRepository::save(
        const std::vector<std::shared_ptr<PropertyBase>> &properties) {
    LOG_TRACE("start persistence of properties");
    try {
        db::Connection connection(m_connectionEntry);
        db::Transaction transaction(connection);
        std::string query;
        switch (m_connectionEntry->getType()) {
            case db::ConnectionType::PostgreSQL:
                query = R"(insert into property (name,
                                         instance_name,
                                         class_name,
                                         process_name,
                                         type,
                                         value)
                   values (?, ?, ?, ?, ?, ?)
                   ON CONFLICT ON CONSTRAINT property_pk
                   DO UPDATE SET value = ?
        )";
                break;
            case db::ConnectionType::SQLite:
                query = R"(insert or replace into property
                  (name, 
                   instance_name,
                   class_name,
                   process_name,
                   type,
                   value) 
                  values (?, ?, ?, ?, ?, ?))";
                break;
            default:
                throw db::SQLException("not supported database type for query");
        }
        db::PreparedStatement preparedStatement(connection, query,
                                                "update_properties");
        for (const auto &item: properties) {
            db::ParameterBuilder builder(m_connectionEntry);
            builder.add(item->getName())
                    .add(item->getInstanceName())
                    .add(item->getClassName())
                    .add(item->getProcessName())
                    .add(item->getType())
                    .add(item->toString())
                    .add(item->toString());
            preparedStatement.execute(builder);
        }
    } catch (db::SQLException &exception) {
        LOG_ERROR("failed to update property {}", exception.what());
    }
    LOG_TRACE("finished persistence of properties");
}

void DatabasePropertyRepository::save(std::shared_ptr<PropertyBase> property) {
    std::vector<std::shared_ptr<PropertyBase>> temp = {property};
    save(temp);
}

std::vector<std::shared_ptr<PropertyBase>> DatabasePropertyRepository::awake() {
    std::vector<std::shared_ptr<PropertyBase>> properties;
    try {
        db::Connection connection(m_connectionEntry);
        db::Transaction transaction(connection);
        db::Statement query(connection);
        std::string stmt = R"(
      select name,
             instance_name,
             class_name,
             process_name,
             value,
             type
      from property
      where process_name = ?
    )";
        db::ParameterBuilder builder(m_connectionEntry);
        builder.add(m_processName->getProcessName());
        db::Result result = query.execute(stmt, builder);
        for (int i = 0; i < result.getSize(); i++) {
            std::string name = result.getValue(i, 0);
            std::string instanceName = result.getValue(i, 1);
            std::string className = result.getValue(i, 2);
            std::string processName = result.getValue(i, 3);
            std::string value = result.getValue(i, 4);
            std::string type = result.getValue(i, 5);
            std::shared_ptr<PropertyBase> property = PropertyFactory::Create(
                    name, instanceName, className, processName, type, value, "", false);
            property->setDataStorage(
                    DataStorage(PropertyRepositoryType::DATABASE_REPOSITORY, ""));
            properties.push_back(property);
        }
    } catch (db::SQLException &exception) {
        LOG_ERROR("{}", exception.what());
    }
    return properties;
}

void DatabasePropertyRepository::deleteOf(
        const std::vector<std::shared_ptr<PropertyBase>> &properties) {
    LOG_INFO("deleteOf properties with size {}", properties.size());
    try {
        db::Connection connection(m_connectionEntry);
        db::Transaction transaction(connection);
        db::PreparedStatement stmt(connection,
                                   R"(delete from property
                                  where process_name = ? 
                                    and class_name = ? 
                                    and instance_name = ? 
                                    and name = ?)",
                                   "delete_properties");
        for (const auto &property: properties) {
            std::stringstream ss;
            ss << *property;
            LOG_TRACE("deleteOf({})", ss.str());
            db::ParameterBuilder builder(m_connectionEntry);
            builder.add(property->getProcessName());
            builder.add(property->getClassName());
            builder.add(property->getInstanceName());
            builder.add(property->getName());
            stmt.execute(builder);
        }
    } catch (db::SQLException &exception) {
        LOG_ERROR("delete failed: {}", exception.what());
    }
}

std::vector<std::shared_ptr<PropertyBase>> DatabasePropertyRepository::allOf(
        const std::string &processName, const std::string &className,
        const std::string &instanceName, const std::string &name) {
    std::string processNameLike = StringUtils::replaceAll(processName, ".*", "%");
    std::string classNameLike = StringUtils::replaceAll(className, ".*", "%");
    std::string instanceNameLike =
            StringUtils::replaceAll(instanceName, ".*", "%");
    std::string nameLike = StringUtils::replaceAll(name, ".*", "%");
    std::vector<std::shared_ptr<PropertyBase>> properties;
    try {
        db::Connection connection(m_connectionEntry);
        db::Transaction transaction(connection);
        db::Statement query(connection);
        std::string stmt = R"(
            select name,
                   instance_name,
                   class_name,
                   process_name,
                   value,
                   type 
            from property
            where process_name like ?
              and class_name like ?
              and instance_name like ?
              and name like ?
            )";
        db::ParameterBuilder builder(m_connectionEntry);
        builder.add(processNameLike);
        builder.add(classNameLike);
        builder.add(instanceNameLike);
        builder.add(nameLike);
        db::Result result = query.execute(stmt, builder);
        for (int i = 0; i < result.getSize(); i++) {
            std::string nameTmp = result.getValue(i, 0);
            std::string instanceNameTmp = result.getValue(i, 1);
            std::string classNameTmp = result.getValue(i, 2);
            std::string processNameTmp = result.getValue(i, 3);
            std::string value = result.getValue(i, 4);
            std::string type = result.getValue(i, 5);
            std::shared_ptr<PropertyBase> property =
                    PropertyFactory::Create(nameTmp, instanceNameTmp, classNameTmp,
                                            processNameTmp, type, value, "", false);
            property->setDataStorage(
                    DataStorage(PropertyRepositoryType::DATABASE_REPOSITORY, ""));
            properties.push_back(property);
        }
    } catch (db::SQLException &exception) {
        LOG_ERROR("{}", exception.what());
    }
    return properties;
}