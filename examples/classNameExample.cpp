#include "base_library/core/services/AbstractService.h"
#include <base_library/features/property/services/PropertyService.h>
#include <base_library/features/property/repositories/NoPropertyRepository.h>
#include <iostream>

class Example : public AbstractService<Example> {
private:
  constexpr static std::string_view prefix = "static std::string_view ";
  constexpr static std::string_view suffix = "::getClassName()";

public:
  Example() : AbstractService("classNameExample", "example") {}
};

int main(int argc, char *argv[]) {
  Example example;
  std::cout << example << "\n";
  std::shared_ptr<NoPropertyRepository> propertyRepository = std::make_shared<NoPropertyRepository>();
  std::vector<std::shared_ptr<AbstractServiceInterface>> abstractInterfaces = {};
  PropertyService propertyService({propertyRepository}, std::make_shared<ProcessName>(argc, argv), abstractInterfaces);
  for (auto &property : propertyService.allProperties()) {
    std::cout << *property << "\n";
  }
  return 0;
}