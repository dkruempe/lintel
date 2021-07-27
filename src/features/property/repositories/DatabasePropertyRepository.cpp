#include "base_library/features/property/repositories/DatabasePropertyRepository.h"

#include <utility>

#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/PreparedStatement.h"
#include "base_library/core/persistence/Statement.h"
#include "base_library/core/persistence/Transaction.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/features/property/factories/PropertyFactory.h"

DatabasePropertyRepository::DatabasePropertyRepository(
    std::shared_ptr<ConnectionConfigurations> connectionConfigurations)
    : PropertyRepository(),
      m_connectionConfigurations(std::move(connectionConfigurations)),
      m_connectionEntry(m_connectionConfigurations->of("DEFAULT")) {}
PropertyRepositoryType DatabasePropertyRepository::getType() {
  return PropertyRepositoryType::DATABASE_REPOSITORY;
}
DataStorage DatabasePropertyRepository::getDataStorage() {
  return DataStorage(PropertyRepositoryType::DATABASE_REPOSITORY, "");
}
bool DatabasePropertyRepository::isMutable() { return true; }
void DatabasePropertyRepository::save(
    const std::vector<std::shared_ptr<PropertyBase>>& properties) {
  try {
    db::Connection connection(m_connectionEntry);
    db::Transaction transaction(connection);
    db::PreparedStatement preparedStatement(
        connection,
        "update property set value = ? where name = ? and instance_name = ? "
        "and class_name = ? and process_name = ? and type = ?",
        "update_properties");
    for (const auto& item : properties) {
      preparedStatement.execute({item->toString(), item->getName(),
                                 item->getInstanceName(), item->getClassName(),
                                 item->getProcessName(), item->getType()});
    }
  } catch (db::SQLException& exception) {
    LOG_ERROR("{}", exception.what());
  }
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
