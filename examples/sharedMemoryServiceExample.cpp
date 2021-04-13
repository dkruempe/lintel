#include <iostream>

#include <base_library/services/SharedMemoryService.h>

struct Property {
  char name[100];
  char type[100];
  char value[100];

  friend std::ostream &operator<<(std::ostream &os, const Property &aProperty) {
    os << "name: " << aProperty.name << " type: " << aProperty.type
       << " value: " << aProperty.value;
    return os;
  }
};

void testPropertiesArray(SharedMemoryService &shmService) {
  std::cout << "\nTEST ARRAY" << std::endl;
  std::array<Property, 50> &properties =
      shmService.constructArray<Property, 50>("array", "ShmProperties");
  std::cout << "sizeOf(array):" << sizeof(std::array<Property, 50>)
            << std::endl;
  std::cout << "before insert" << std::endl;
  for (auto &property : properties) {
    std::cout << property << std::endl;
  }
  std::strncpy(properties[0].name, "Anna", 100);
  std::strncpy(properties[0].type, "Krümpelmann", 100);
  std::strncpy(properties[0].value, "28", 100);
  std::strncpy(properties[1].name, "Dominik", 100);
  std::strncpy(properties[1].type, "Krümpelmann", 100);
  std::strncpy(properties[1].value, "30", 100);
  std::cout << "after insert" << std::endl;
  for (auto &property : properties) {
    std::cout << property << std::endl;
  }
}

void testPropertiesMap(SharedMemoryService &shmService) {
  std::cout << "\nTEST MAP" << std::endl;
  auto &propertyMap =
      shmService.constructMap<int32_t, Property>("map", "ShmPropertyMap");
  Property property{"Anna", "28", "Krümpelmann"};
  Property secondProperty{"Dominik", "30", "Krümpelmann"};
  std::cout << "before insert" << std::endl;
  for (auto &[id, property] : propertyMap) {
    std::cout << "<" << id << "," << property << ">" << std::endl;
  }
  propertyMap.insert(std::make_pair<>(4711, property));
  propertyMap.insert(std::make_pair<>(4712, secondProperty));
  std::cout << "after insert" << std::endl;
  for (auto &[id, property] : propertyMap) {
    std::cout << "<" << id << "," << property << ">" << std::endl;
  }
}

void testSet(SharedMemoryService &shmService) {
  std::cout << "\nTEST SET" << std::endl;
  auto &set = shmService.constructSet<int32_t>("vectorSet", "ShmPropertySet");
  std::cout << "before insert" << std::endl;
  for (auto &iter : set) {
    std::cout << iter << std::endl;
  }
  set.insert(4711);
  set.insert(4712);
  set.insert(4711);
  std::cout << "after insert" << std::endl;
  for (auto &iter : set) {
    std::cout << iter << std::endl;
  }
}

void testPropertiesVector(SharedMemoryService &shmService) {
  std::cout << "\nTEST VECTOR" << std::endl;
  auto &vector =
      shmService.constructVector<Property>("vectorSet", "ShmPropertyVector");
  std::cout << "before insert" << std::endl;
  for (auto &iter : vector) {
    std::cout << iter << std::endl;
  }
  Property property{"Anna", "28", "Krümpelmann"};
  Property secondProperty{"Dominik", "30", "Krümpelmann"};
  vector.push_back(property);
  vector.push_back(secondProperty);
  std::cout << "after insert" << std::endl;
  for (auto &iter : vector) {
    std::cout << iter << std::endl;
  }
}

class SharedMemorySegmentConfiguration : public AbstractShmConfig {
 private:
 public:
  SharedMemorySegmentConfiguration()
      : AbstractShmConfig(
            {SharedMemorySegment("/Users/dkruempe/shm", "array", 1ul << 30),
             SharedMemorySegment("/Users/dkruempe/shm", "map", 1ul << 30),
             SharedMemorySegment("/Users/dkruempe/shm", "vectorSet",
                                 1ul << 30)},
            {{"ShmSet", "set_shm"}, {"ShmMap", "map_shm"}}){};
};

int main(int argc, char *argv[]) {
  std::shared_ptr<SharedMemorySegmentConfiguration> config =
      std::make_shared<SharedMemorySegmentConfiguration>();
  SharedMemoryService shmService(config);
  std::cout << shmService << std::endl;
  testPropertiesArray(shmService);
  testPropertiesMap(shmService);
  testSet(shmService);
  testPropertiesVector(shmService);

  return 0;
}