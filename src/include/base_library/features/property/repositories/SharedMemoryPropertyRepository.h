#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYPROPERTYREPOSITORY_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYPROPERTYREPOSITORY_H

#include <boost/interprocess/sync/named_upgradable_mutex.hpp>

#include "base_library/core/services/LoggerMacros.h"
#include "base_library/features/base/repositories/SharedMemoryRepository.h"
#include "base_library/features/base/services/SharedMemorySegmentManager.h"
#include "base_library/features/property/repositories/PropertyRepository.h"

/** DTO for property data stored in shared memory */
struct PropertyDataDto {
    SharedMemoryService::ShmString m_processName;
    SharedMemoryService::ShmString m_className;
    SharedMemoryService::ShmString m_instanceName;
    SharedMemoryService::ShmString m_name;
    SharedMemoryService::ShmString m_value;
    SharedMemoryService::ShmString m_type;

    static int32_t getSize() {
        return sizeof(SharedMemoryService::ShmString) * 6;
    }

    bool operator<(const PropertyDataDto &rhs) const;

    bool operator>(const PropertyDataDto &rhs) const;

    bool operator<=(const PropertyDataDto &rhs) const;

    bool operator>=(const PropertyDataDto &rhs) const;

    bool operator==(const PropertyDataDto &rhs) const;

    bool operator!=(const PropertyDataDto &rhs) const;
};

/** DAO for serializing PropertyDataDto to/from JSON */
class PropertyDataDao : public JsonSerializable {
private:
    static struct Shapes {
        const std::string PROCESS_NAME = "process_name";
        const std::string CLASS_NAME = "class_name";
        const std::string INSTANCE_NAME = "instance_name";
        const std::string NAME = "name";
        const std::string VALUE = "value";
        const std::string TYPE = "type";
    } m_shape;

    const PropertyDataDto &m_dto;

public:
    PropertyDataDao(const PropertyDataDto &dto) : m_dto(dto) {}

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override {
        writer->StartObject();
        writer->String(m_shape.PROCESS_NAME.c_str());
        writer->String(m_dto.m_processName.c_str());
        writer->String(m_shape.CLASS_NAME.c_str());
        writer->String(m_dto.m_className.c_str());
        writer->String(m_shape.INSTANCE_NAME.c_str());
        writer->String(m_dto.m_instanceName.c_str());
        writer->String(m_shape.NAME.c_str());
        writer->String(m_dto.m_name.c_str());
        writer->String(m_shape.VALUE.c_str());
        writer->String(m_dto.m_value.c_str());
        writer->String(m_shape.TYPE.c_str());
        writer->String(m_dto.m_type.c_str());
        writer->EndObject();
    }

    bool deserialize(const rapidjson::Value &obj) override {
        bool success = true;
        if (obj.HasMember(m_shape.PROCESS_NAME.c_str())) {
        } else {
            success = false;
            LOG_ERROR("{} not defined in json serialization",
                      m_shape.PROCESS_NAME.c_str());
        }
        if (obj.HasMember(m_shape.CLASS_NAME.c_str())) {
        } else {
            success = false;
            LOG_ERROR("{} not defined in json serialization",
                      m_shape.CLASS_NAME.c_str());
        }
        if (obj.HasMember(m_shape.INSTANCE_NAME.c_str())) {
        } else {
            success = false;
            LOG_ERROR("{} not defined in json serialization",
                      m_shape.INSTANCE_NAME.c_str());
        }
        if (obj.HasMember(m_shape.NAME.c_str())) {
        } else {
            success = false;
            LOG_ERROR("{} not defined in json serialization", m_shape.NAME.c_str());
        }
        if (obj.HasMember(m_shape.VALUE.c_str())) {
        } else {
            success = false;
            LOG_ERROR("{} not defined in json serialization", m_shape.VALUE.c_str());
        }
        if (obj.HasMember(m_shape.TYPE.c_str())) {
        } else {
            success = false;
            LOG_ERROR("{} not defined in json serialization", m_shape.TYPE.c_str());
        }
        return success;
    }
};

/** Property repository backed by shared memory */
class SharedMemoryPropertyRepository
        : public SharedMemoryMapRepository<
                SharedMemoryService::ShmString, PropertyDataDto,
                SharedMemoryService::ShmString, PropertyDataDao>,
          public PropertyRepository {
private:
    static constexpr int32_t m_version = 0;
    std::shared_ptr<ProcessName> m_processName;
    DataStorage m_currentDataStorage =
            DataStorage(PropertyRepositoryType::SHM_REPOSITORY, "");
    boost::interprocess::named_upgradable_mutex m_upgradableMutex;
    static constexpr std::string_view UUID = "B3F746E1-D4F0-49DB-A0F7-51F72436263A";

public:
    SharedMemoryPropertyRepository(
            const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
            const std::shared_ptr<SharedMemorySegmentManager>
            &sharedMemorySegmentManager,
            const std::shared_ptr<Configuration> &configuration,
            std::shared_ptr<ProcessName> processName);

    DataStorage getDataStorage() override;

    void save(
            const std::vector<std::shared_ptr<PropertyBase>> &properties) override;

    void save(std::shared_ptr<PropertyBase> property) override;

    std::vector<std::shared_ptr<PropertyBase>> awake() override;

    std::vector<std::shared_ptr<PropertyBase>> allOf(
            const std::string &processName, const std::string &className,
            const std::string &instanceName, const std::string &name) override;

    void onMigrate(int32_t currentActiveVersion) override;

    void deleteOf(
            const std::vector<std::shared_ptr<PropertyBase>> &properties) override;
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYPROPERTYREPOSITORY_H
