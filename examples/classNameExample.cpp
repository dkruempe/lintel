#include "services/AbstractService.h"
#include "services/PropertyService.h"
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
  PropertyService propertyService ("classNameExample");
  std::cout << propertyService << "\n";
  return 0;
}