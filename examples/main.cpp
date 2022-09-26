#include <base_library/core/StartupBuilder.h>
#include <base_library/core/services/SharedMemoryService.h>
#include <base_library/features/base/BaseFeature.h>
#include <base_library/features/base/repositories/SharedMemoryRepository.h>
#include <base_library/features/cli/CommandLineFeature.h>
#include <base_library/features/http/HttpFeature.h>
#include <base_library/features/property/PropertyFeature.h>

class TestData : public JsonSerializable {
 private:
  static struct Shapes {
    const std::string NAME = "name";
    const std::string ADDR = "addr";
    const std::string PLZ = "plz";
    const std::string LOCATION = "location";
    const std::string AGE = "age";
  } m_shape;

 public:
  static int32_t getSize() {
    return sizeof(SharedMemoryService::ShmString) * 4 + sizeof(int32_t);
  }
  SharedMemoryService::ShmString m_name;
  SharedMemoryService::ShmString m_addr;
  SharedMemoryService::ShmString m_plz;
  SharedMemoryService::ShmString m_location;
  int32_t m_age;
  TestData(SharedMemoryService::ShmString name,
           SharedMemoryService::ShmString addr,
           SharedMemoryService::ShmString plz,
           SharedMemoryService::ShmString location, int32_t age)
      : m_name(name),
        m_addr(addr),
        m_plz(plz),
        m_location(location),
        m_age(age) {}
  void serialize(
      rapidjson::Writer<rapidjson::StringBuffer> *writer) const override {
    LOG_TRACE("start serialization");
    writer->StartObject();

    writer->String(m_shape.NAME.c_str());
    writer->String(m_name.c_str());
    LOG_TRACE("add name");
    writer->String(m_shape.ADDR.c_str());
    writer->String(m_addr.c_str());
    LOG_TRACE("add addr");
    writer->String(m_shape.PLZ.c_str());
    writer->String(m_plz.c_str());
    LOG_TRACE("add plz");
    writer->String(m_shape.LOCATION.c_str());
    writer->String(m_location.c_str());
    LOG_TRACE("add location");
    writer->String(m_shape.AGE.c_str());
    writer->String(std::to_string(m_age).c_str());
    LOG_TRACE("add age");
    writer->EndObject();
    LOG_TRACE("finished serialization");
  }
  bool deserialize(const rapidjson::Value &obj) override {
    bool success = true;
    // PROCESS_NAME
    if (obj.HasMember(m_shape.NAME.c_str())) {
      // m_processName = obj[m_shape.PROCESS_NAME.c_str()].GetString();
    } else {
      success = false;
      LOG_ERROR("{} not defined in json serialization", m_shape.NAME.c_str());
    }
    // CLASS_NAME
    if (obj.HasMember(m_shape.ADDR.c_str())) {
      // m_className = obj[m_shape.CLASS_NAME.c_str()].GetString();
    } else {
      success = false;
      LOG_ERROR("{} not defined in json serialization", m_shape.ADDR.c_str());
    }
    // INSTANCE_NAME
    if (obj.HasMember(m_shape.PLZ.c_str())) {
      // m_instanceName = obj[m_shape.INSTANCE_NAME.c_str()].GetString();
    } else {
      success = false;
      LOG_ERROR("{} not defined in json serialization", m_shape.PLZ.c_str());
    }
    // NAME
    if (obj.HasMember(m_shape.LOCATION.c_str())) {
      // m_name = obj[m_shape.NAME.c_str()].GetString();
    } else {
      success = false;
      LOG_ERROR("{} not defined in json serialization",
                m_shape.LOCATION.c_str());
    }
    // VALUE
    if (obj.HasMember(m_shape.AGE.c_str())) {
      // m_value = obj[m_shape.VALUE.c_str()].GetString();
    } else {
      success = false;
      LOG_ERROR("{} not defined in json serialization", m_shape.AGE.c_str());
    }
    // TYPE
    return success;
  }
};

TestData::Shapes TestData::m_shape{};

class ShmVec : public SharedMemoryVectorRepository<TestData> {
 private:
  static constexpr int32_t m_version = 0;

 public:
  ShmVec(const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
         const std::shared_ptr<SharedMemorySegmentManager>
             &sharedMemorySegmentManager)
      : SharedMemoryVectorRepository<TestData>(
            sharedMemoryService, sharedMemorySegmentManager->of("shm_test"),
            m_version) {
    if (getVector().empty()) {
      for (int i = 0; i < 100; i++) {
        TestData d(m_sharedMemoryService->constructString(
                       getSharedMemorySegment(), "Example User"),
                   m_sharedMemoryService->constructString(
                       getSharedMemorySegment(), "Musterstraße 1"),
                   m_sharedMemoryService->constructString(
                       getSharedMemorySegment(), "48496"),
                   m_sharedMemoryService->constructString(
                       getSharedMemorySegment(), "Halverde"), i);
        getVector().push_back(d);
      }
    }
  }

  void onMigrate(int32_t currentActiveVersion) override {}
};

class TestFeature : public Feature {
 private:
  std::shared_ptr<ShmVec> m_shmVec;

 public:
  TestFeature() : Feature(type_name<TestFeature>()) {}

  void registerTypes(Hypodermic::ContainerBuilder &builder) override {
    builder.registerType<ShmVec>()
        .as<SharedMemoryRepository>()
        .asSelf()
        .singleInstance();
  }

  void initialize(std::shared_ptr<Hypodermic::Container> container) override {
    m_shmVec = container->resolve<ShmVec>();
  }
};

int main(int argc, char *argv[]) {
  try {
    std::shared_ptr<StartupBuilder> builder = StartupBuilder::with(argc, argv);
    builder->addFeature<PropertyFeature>();
    builder->addFeature<BaseFeature>();
    builder->addFeature<CommandLineFeature>();
    builder->addFeature<HttpFeature>();
    builder->addFeature<TestFeature>();
    builder->start();
  } catch (std::exception e) {
    LOG_INFO("exception: {}", e.what());
  } catch (...) {
    LOG_INFO("exception was thrown");
  }
  return 0;
}