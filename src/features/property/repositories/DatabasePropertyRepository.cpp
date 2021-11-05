#include "base_library/features/property/repositories/DatabasePropertyRepository.h"

#include <utility>

#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/PreparedStatement.h"
#include "base_library/core/persistence/Statement.h"
#include "base_library/core/persistence/Transaction.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/features/property/factories/PropertyFactory.h"

DatabasePropertyRepository::DatabasePropertyRepository(
    std::shared_ptr<DatabaseConnectionConfigurations> connectionConfigurations)
    : PropertyRepository(),
      m_connectionConfigurations(std::move(connectionConfigurations)),
      m_connectionEntry(m_connectionConfigurations->ofDefault()) {}
PropertyRepositoryType DatabasePropertyRepository::getType() {
  return PropertyRepositoryType::DATABASE_REPOSITORY;
}
DataStorage DatabasePropertyRepository::getDataStorage() {
  return m_currentDataStorage;
}
bool DatabasePropertyRepository::isMutable() { return true; }
void DatabasePropertyRepository::save(
    const std::vector<std::shared_ptr<PropertyBase>>& properties) {
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
    for (const auto& item : properties) {
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
  } catch (db::SQLException& exception) {
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
    db::Result result = query.execute(
        "select name, instance_name, class_name, process_name, value, type "
        "from "
        "property");
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
  } catch (db::SQLException& exception) {
    LOG_ERROR("{}", exception.what());
  }
  return properties;
}
