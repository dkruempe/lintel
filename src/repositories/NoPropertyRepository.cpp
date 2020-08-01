#include "base_library/repositories/NoPropertyRepository.h"
#include <vector>
void NoPropertyRepository::save(std::shared_ptr<PropertyBase> property) {}

std::vector<std::shared_ptr<PropertyBase>> NoPropertyRepository::awake() {
  return std::vector<std::shared_ptr<PropertyBase>>();
}
void NoPropertyRepository::save(
    const std::vector<std::shared_ptr<PropertyBase>> &properties) {}