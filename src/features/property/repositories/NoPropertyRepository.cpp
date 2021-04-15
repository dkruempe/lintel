#include "base_library/features/property/repositories/NoPropertyRepository.h"
#include <vector>
void NoPropertyRepository::save(std::shared_ptr<PropertyBase> property) {}

std::vector<std::shared_ptr<PropertyBase>> NoPropertyRepository::awake() {
  return std::vector<std::shared_ptr<PropertyBase>>();
}
void NoPropertyRepository::save(
    const std::vector<std::shared_ptr<PropertyBase>> &properties) {}
PropertyRepositoryType NoPropertyRepository::getType() {
  return PropertyRepositoryType::DEFAULT;
}
bool NoPropertyRepository::isMutable() { return true; }

DataStorage NoPropertyRepository::getDataStorage() {
  return DataStorage(getType(), "");
}