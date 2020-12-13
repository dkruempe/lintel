#include <base_library/repositories/NoPropertyRepository.h>
#include <base_library/services/AbstractService.h>
#include <base_library/services/PropertyService.h>
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
  PropertyService propertyService({propertyRepository}, std::make_shared<ProcessName>(argc, argv));
  std::cout << propertyService << "\n";
  return 0;
}