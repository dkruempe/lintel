#include <base_library/factories/PropertyFactory.h>
#include <base_library/models/PropertyBase.h>
#include <base_library/repositories/FilePropertyRepository.h>
#include <base_library/services/PropertyService.h>
#include <base_library/strategies/XMLConfigSerializationStrategy.h>
#include <iostream>
#include <libgen.h>
#include <memory>
#include <utility>
#include <vector>

std::vector<std::shared_ptr<PropertyBase>> createProperties() {
  std::vector<std::shared_ptr<PropertyBase>> properties;
  for (int k = 0; k < 2; k++) {
    for (int j = 0; j < 5; j++) {
      for (int i = 0; i < 5; i++) {
        for (int l = 0; l < 5; l++) {
          properties.emplace_back(PropertyFactory::Create(
              "name" + std::to_string(i) + "_" + std::to_string(j) + "_" +
                  std::to_string(k) + "_" + std::to_string(l),
              std::to_string(k), std::to_string(j), std::to_string(i),
              "std::string", "anna_" + std::to_string(i + j * k * (l + 1))));
        }
      }
    }
  }
  return properties;
}

void testSerializeDeserialize() {
  auto properties = createProperties();
  std::sort(properties.begin(), properties.end(),
            [](const std::shared_ptr<PropertyBase> &rhs,
               std::shared_ptr<PropertyBase> &lhs) { return *rhs < *lhs; });
  std::cout << properties.size() << std::endl;
  for (auto &property : properties) {
    std::cout << property->getProcessName() << ": " << property->getClassName()
              << " " << property->getInstanceName() << " "
              << property->toString() << std::endl;
  }
  XMLConfigSerializationStrategy xmlConfigSerializationStrategy;
  std::string content = xmlConfigSerializationStrategy.serialize(properties);
  auto newProperties = xmlConfigSerializationStrategy.deserialize(content);
  std::cout << "DESERIALIZE: size " << newProperties.size() << std::endl;
  for (auto &property : newProperties) {
    std::cout << property->getProcessName() << ": " << property->getClassName()
              << " " << property->getInstanceName() << " "
              << property->toString() << std::endl;
  }
}

class PropertyExampleClass : public AbstractService<PropertyExampleClass> {
public:
  explicit PropertyExampleClass(const std::shared_ptr<PropertyService>& propertyService,
                                std::string instanceName,
                                std::string processName)
      : AbstractService(std::move(processName), std::move(instanceName)) {
    LOAD_PROPERTIES();
  }

  void printProperty() const {
    std::cout << *extra << std::endl;
    std::cout << *string << std::endl;
    std::cout << *enable << std::endl;
  }
  DEFINE_PROPERTY(extra, int32_t, 4711);
  DEFINE_PROPERTY(string, std::string, "Ich bin eine Test Property");
  DEFINE_PROPERTY(enable, bool, false);
};

void testPropertyService(const std::string &processName) {
  XMLConfigSerializationStrategy xmlConfigSerializationStrategy;
  std::shared_ptr<FilePropertyRepository> filePropertyRepository = std::make_shared<FilePropertyRepository>("/Users/dkruempe/cfg/test.xml",
                                                xmlConfigSerializationStrategy);
  std::shared_ptr<PropertyService> propertyService = std::make_shared<PropertyService>(filePropertyRepository, std::make_shared<std::string>(processName));
  PropertyExampleClass A(propertyService, "A", processName);
  PropertyExampleClass B(propertyService, "B", processName);
  propertyService->onInitialize();
  A.printProperty();
  B.printProperty();
}

int main(int argc, char *argv[]) {
  testPropertyService(basename(argv[0]));
  testSerializeDeserialize();
  return 0;
}