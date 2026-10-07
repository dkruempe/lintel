#ifndef LINTEL_SHAREDMEMORYPROPERTYREPOSITORY_H
#define LINTEL_SHAREDMEMORYPROPERTYREPOSITORY_H

#include <boost/interprocess/sync/named_upgradable_mutex.hpp>

#include "lintel/features/base/repositories/SharedMemoryRepository.h"
#include "lintel/features/base/services/SharedMemorySegmentManager.h"
#include "lintel/features/property/repositories/PropertyRepository.h"

/** DTO for property data stored in shared memory */
struct PropertyDataDto {
    shm::String m_processName;
    shm::String m_className;
    shm::String m_instanceName;
    shm::String m_name;
    shm::String m_value;
    shm::String m_type;

    static int32_t getSize() {
        return sizeof(shm::String) * 6;
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
  // constexpr char* instead of std::string: static initialization can then not
  // throw (clang-tidy cert-err58-cpp).
    static struct Shapes {
        static constexpr const char *PROCESS_NAME = "process_name";
        static constexpr const char *CLASS_NAME = "class_name";
        static constexpr const char *INSTANCE_NAME = "instance_name";
        static constexpr const char *NAME = "name";
        static constexpr const char *VALUE = "value";
        static constexpr const char *TYPE = "type";
    } m_shape;

    const PropertyDataDto &m_dto;

public:
    PropertyDataDao(const PropertyDataDto &dto) : m_dto(dto) {}

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override {
        writer->StartObject();
        writer->String(m_shape.PROCESS_NAME);
        writer->String(m_dto.m_processName.c_str());
        writer->String(m_shape.CLASS_NAME);
        writer->String(m_dto.m_className.c_str());
        writer->String(m_shape.INSTANCE_NAME);
        writer->String(m_dto.m_instanceName.c_str());
        writer->String(m_shape.NAME);
        writer->String(m_dto.m_name.c_str());
        writer->String(m_shape.VALUE);
        writer->String(m_dto.m_value.c_str());
        writer->String(m_shape.TYPE);
        writer->String(m_dto.m_type.c_str());
        writer->EndObject();
    }

    bool deserialize(const rapidjson::Value &obj) override {
        bool success = true;
        if (!obj.HasMember(m_shape.PROCESS_NAME)) {
            success = false;
            LOG_ERROR("{} not defined in json serialization",
                      m_shape.PROCESS_NAME);
        }
        if (!obj.HasMember(m_shape.CLASS_NAME)) {
            success = false;
            LOG_ERROR("{} not defined in json serialization",
                      m_shape.CLASS_NAME);
        }
    if (!obj.HasMember(m_shape.INSTANCE_NAME)) {
            success = false;
            LOG_ERROR("{} not defined in json serialization",
                      m_shape.INSTANCE_NAME);
        }
        if (!obj.HasMember(m_shape.NAME)) {
            success = false;
            LOG_ERROR("{} not defined in json serialization", m_shape.NAME);
        }
        if (!obj.HasMember(m_shape.VALUE)) {
            success = false;
            LOG_ERROR("{} not defined in json serialization", m_shape.VALUE);
        }
        if (!obj.HasMember(m_shape.TYPE)) {
            success = false;
            LOG_ERROR("{} not defined in json serialization", m_shape.TYPE);
        }
        return success;
    }
};

/** Property repository backed by shared memory */
class SharedMemoryPropertyRepository
        : public SharedMemoryMapRepository<
                shm::String, PropertyDataDto,
                shm::String, PropertyDataDao>,
          public PropertyRepository {
private:
    static constexpr int32_t m_version = 0;
    std::shared_ptr<ProcessName> m_processName;
    DataStorage m_currentDataStorage =
            DataStorage(PropertyRepositoryType::SHM_REPOSITORY, "");
    mutable boost::interprocess::named_upgradable_mutex m_upgradableMutex;
    static constexpr std::string_view UUID = "B3F746E1-D4F0-49DB-A0F7-51F72436263A";

protected:
    void onAcquireReadLock() const override {
        m_upgradableMutex.lock_sharable();
    }

    void onReleaseReadLock() const override {
        m_upgradableMutex.unlock_sharable();
    }

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

#endif  // LINTEL_SHAREDMEMORYPROPERTYREPOSITORY_H
