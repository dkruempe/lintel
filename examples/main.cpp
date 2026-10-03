#include <base_library/core/StartupBuilder.h>
#include <base_library/core/services/SharedMemoryService.h>
#include <base_library/features/base/BaseFeature.h>
#include <base_library/features/base/repositories/SharedMemoryRepository.h>
#include <base_library/features/base/services/EventBusService.h>
#include <base_library/features/cli/CommandLineFeature.h>
#include <base_library/features/http/HttpFeature.h>
#include <base_library/features/property/PropertyFeature.h>
#include <base_library/features/property/services/PropertyService.h>

#include "Hypodermic/Container.h"
#include "Hypodermic/ContainerBuilder.h"

#include <utility>

struct ArrayDto {
    char m_name[100]{};
    char m_addr[100]{};
    char m_plz[100]{};
    char m_location[100]{};
    int32_t m_age = 0;

    bool operator<(const ArrayDto &rhs) const {
        if (m_name < rhs.m_name) return true;
        if (rhs.m_name < m_name) return false;
        if (m_addr < rhs.m_addr) return true;
        if (rhs.m_addr < m_addr) return false;
        if (m_plz < rhs.m_plz) return true;
        if (rhs.m_plz < m_plz) return false;
        if (m_location < rhs.m_location) return true;
        if (rhs.m_location < m_location) return false;
        return m_age < rhs.m_age;
    }

    bool operator>(const ArrayDto &rhs) const { return rhs < *this; }

    bool operator<=(const ArrayDto &rhs) const { return !(rhs < *this); }

    bool operator>=(const ArrayDto &rhs) const { return !(*this < rhs); }

    bool operator==(const ArrayDto &rhs) const {
        return m_name == rhs.m_name && m_addr == rhs.m_addr && m_plz == rhs.m_plz &&
               m_location == rhs.m_location && m_age == rhs.m_age;
    }

    bool operator!=(const ArrayDto &rhs) const { return !(rhs == *this); }
};

class ArrayDao : public JsonSerializable {
private:
    const ArrayDto &m_dto;

    static struct Shapes {
        const std::string NAME = "name";
        const std::string ADDR = "addr";
        const std::string PLZ = "plz";
        const std::string LOCATION = "location";
        const std::string AGE = "age";
    } m_shape;

public:
    explicit ArrayDao(const ArrayDto &dto) : m_dto(dto) {}

    static int32_t getSize() { return sizeof(char[100]) * 4 + sizeof(int32_t); }

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override {
        writer->StartObject();
        writer->String(m_shape.NAME.c_str());
        writer->String(m_dto.m_name);
        writer->String(m_shape.ADDR.c_str());
        writer->String(m_dto.m_addr);
        writer->String(m_shape.PLZ.c_str());
        writer->String(m_dto.m_plz);
        writer->String(m_shape.LOCATION.c_str());
        writer->String(m_dto.m_location);
        writer->String(m_shape.AGE.c_str());
        writer->String(std::to_string(m_dto.m_age).c_str());
        writer->EndObject();
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

struct TestDataDto {
    shm::String m_name;
    shm::String m_addr;
    shm::String m_plz;
    shm::String m_location;
    int32_t m_age = 0;

    bool operator==(const TestDataDto &rhs) const {
        return m_name == rhs.m_name && m_addr == rhs.m_addr && m_plz == rhs.m_plz &&
               m_location == rhs.m_location && m_age == rhs.m_age;
    }

    bool operator!=(const TestDataDto &rhs) const { return !(rhs == *this); }

    bool operator<(const TestDataDto &rhs) const {
        if (m_name < rhs.m_name) return true;
        if (rhs.m_name < m_name) return false;
        if (m_addr < rhs.m_addr) return true;
        if (rhs.m_addr < m_addr) return false;
        if (m_plz < rhs.m_plz) return true;
        if (rhs.m_plz < m_plz) return false;
        if (m_location < rhs.m_location) return true;
        if (rhs.m_location < m_location) return false;
        return m_age < rhs.m_age;
    }

    bool operator>(const TestDataDto &rhs) const { return rhs < *this; }

    bool operator<=(const TestDataDto &rhs) const { return !(rhs < *this); }

    bool operator>=(const TestDataDto &rhs) const { return !(*this < rhs); }
};

class TestDataDao : public JsonSerializable {
private:
    const TestDataDto &m_dto;

    static struct Shapes {
        const std::string NAME = "name";
        const std::string ADDR = "addr";
        const std::string PLZ = "plz";
        const std::string LOCATION = "location";
        const std::string AGE = "age";
    } m_shape;

public:
    explicit TestDataDao(const TestDataDto &dto) : m_dto(dto) {}

    static int32_t getSize() {
        return sizeof(shm::String) * 4 + sizeof(int32_t);
    }

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override {
        writer->StartObject();
        writer->String(m_shape.NAME.c_str());
        writer->String(m_dto.m_name.c_str());
        writer->String(m_shape.ADDR.c_str());
        writer->String(m_dto.m_addr.c_str());
        writer->String(m_shape.PLZ.c_str());
        writer->String(m_dto.m_plz.c_str());
        writer->String(m_shape.LOCATION.c_str());
        writer->String(m_dto.m_location.c_str());
        writer->String(m_shape.AGE.c_str());
        writer->String(std::to_string(m_dto.m_age).c_str());
        writer->EndObject();
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

TestDataDao::Shapes TestDataDao::m_shape{};
ArrayDao::Shapes ArrayDao::m_shape{};

class ShmObject : public SharedMemoryObjectRepository<ArrayDto, ArrayDao> {
private:
    static constexpr int32_t m_version = 0;
    static constexpr std::string_view UUID = "04B33BE5-2AE2-4466-A5AD-3C6B89027DD0";

public:
    ShmObject(const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
              const std::shared_ptr<SharedMemorySegmentManager>
              &sharedMemorySegmentManager)
            : SharedMemoryObjectRepository(sharedMemoryService,
                                           sharedMemorySegmentManager->of("shm_test"),
                                           m_version, std::string{UUID}) {
        if (getData().m_age == 0) {
            ArrayDto &dto = getData();
            std::strncpy(dto.m_name, "Example User", 100);
            std::strncpy(dto.m_addr, "Musterstraße 1", 100);
            std::strncpy(dto.m_plz, "48496", 100);
            std::strncpy(dto.m_location, "Example City", 100);
            dto.m_age = 1337;
        }
    }

    void onMigrate(int32_t currentActiveVersion) override {}
};

class ShmArray : public SharedMemoryArrayRepository<ArrayDto, ArrayDao, 100> {
private:
    static constexpr int32_t m_version = 0;
    static constexpr std::string_view UUID = "F6DF706E-AB1A-4F7B-A8C7-0D60165E2517";

public:
    ShmArray(const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
             const std::shared_ptr<SharedMemorySegmentManager>
             &sharedMemorySegmentManager)
            : SharedMemoryArrayRepository(sharedMemoryService,
                                          sharedMemorySegmentManager->of("shm_test"),
                                          m_version, std::string{UUID}) {
        if (getArray()[99].m_age == 0) {
            for (int i = 0; i < 100; i++) {
                ArrayDto &dto = getArray()[i];
                std::strncpy(dto.m_name, "Example User", 100);
                std::strncpy(dto.m_addr, "Musterstraße 1", 100);
                std::strncpy(dto.m_plz, "48496", 100);
                std::strncpy(dto.m_location, "Example City", 100);
                dto.m_age = i;
            }
        }
    }

    void onMigrate(int32_t currentActiveVersion) override {}
};

class ShmVec : public SharedMemoryVectorRepository<TestDataDto, TestDataDao> {
private:
    static constexpr int32_t m_version = 0;
    static constexpr std::string_view UUID = "103F04D3-3000-40E0-AEF3-EA83CFC12942";

public:
    ShmVec(const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
           const std::shared_ptr<SharedMemorySegmentManager>
           &sharedMemorySegmentManager)
            : SharedMemoryVectorRepository<TestDataDto, TestDataDao>(
            sharedMemoryService, sharedMemorySegmentManager->of("shm_test"),
            m_version, std::string{UUID}) {
        if (getVector().empty()) {
            for (int i = 0; i < 100; i++) {
                TestDataDto dto{
                        .m_name = shm::constructString(
                                *m_sharedMemoryService,
                                getSharedMemorySegment(), "Example User"),
                        .m_addr = shm::constructString(
                                *m_sharedMemoryService,
                                getSharedMemorySegment(), "Musterstraße 1"),
                        .m_plz = shm::constructString(*m_sharedMemoryService,
                                                      getSharedMemorySegment(),
                                                      "48496"),
                        .m_location = shm::constructString(
                                *m_sharedMemoryService,
                                getSharedMemorySegment(), "Example City"),
                        .m_age = i};
                getVector().push_back(dto);
            }
        }
    }

    void onMigrate(int32_t currentActiveVersion) override {}
};

enum TestEnum {
    Test
};

class TestFeature : public Feature<TestEnum> {
private:
    std::shared_ptr<ShmVec> m_shmVec;

public:
    explicit TestFeature(std::shared_ptr<Features> features) : Feature(TestEnum::Test, std::move(features)) {}

    void registerTypes(Hypodermic::ContainerBuilder &builder) override {
        builder.registerType<ShmVec>()
                .as<SharedMemoryRepository>()
                .asSelf()
                .singleInstance();
        builder.registerType<ShmArray>()
                .as<SharedMemoryRepository>()
                .asSelf()
                .singleInstance();
        builder.registerType<ShmObject>()
                .as<SharedMemoryRepository>()
                .asSelf()
                .singleInstance();
    }

    void initialize(std::shared_ptr<Hypodermic::Container> container) override {
        m_shmVec = container->resolve<ShmVec>();
        // publish property changes on the event bus so that the worker
        // processes receive them through the shared memory mirror
        auto propertyService = container->resolve<PropertyService>();
        auto eventBusService = container->resolve<EventBusService>();
        propertyService->setPropertyChangeBus(eventBusService->of("main"),
                                              "main");
    }
};

int main(int argc, char *argv[]) {
    std::shared_ptr<StartupBuilder> builder = StartupBuilder::with(argc, argv);
    builder->addFeature<PropertyFeature>();
    builder->addFeature<BaseFeature>();
    builder->addFeature<CommandLineFeature>();
    builder->addFeature<HttpFeature>();
    builder->addFeature<TestFeature>();
    builder->start();
    return 0;
}