#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYREPOSITORY_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYREPOSITORY_H

#include <base_library/core/models/JsonSerializable.h>

#include <memory>

#include "base_library/core/models/SharedMemorySegment.h"
#include "base_library/core/services/PersistableBean.h"
#include "base_library/core/services/SharedMemoryService.h"
#include "base_library/core/utils/TypeName.h"

enum SharedMemoryType {
    Map, Vector, Array, Object
};

class SharedMemoryRepository : public JsonSerializable {
private:
    std::shared_ptr<SharedMemorySegment> m_sharedMemorySegment;
    std::size_t m_sizeOfData;
    std::string m_sharedMemoryRepository;
    int32_t m_codeVersion;
    std::string m_uuid;

protected:
    template<typename TYPE>
    static constexpr bool isSerializable() {
        return std::is_base_of<JsonSerializable, TYPE>() ||
               std::is_same<SharedMemoryService::ShmString, TYPE>() ||
               std::is_arithmetic<TYPE>();
    }

public:
    SharedMemoryRepository(
            std::shared_ptr<SharedMemorySegment> sharedMemorySegment,
            std::size_t sizeOfData, std::string_view sharedMemoryRepository,
            int32_t codeVersion, std::string uuid);

    [[nodiscard]] const std::shared_ptr<SharedMemorySegment>
    &getSharedMemorySegment() const;

    [[nodiscard]] std::size_t getSizeOfData() const;

    [[nodiscard]] const std::string &getSharedMemoryRepository() const;

    [[nodiscard]] int32_t getCodeVersion() const;

    [[nodiscard]] const std::string &getUuid() const;

    [[nodiscard]] virtual SharedMemoryType getType() const = 0;

    [[nodiscard]] std::string getTypeName() const {
        if (getType() == SharedMemoryType::Map) {
            return "Map";
        }
        if (getType() == SharedMemoryType::Vector) {
            return "Vector";
        }
        if (getType() == SharedMemoryType::Array) {
            return "Array";
        }
        return "Object";
    }

    ~SharedMemoryRepository() override = default;

    virtual void onMigrate(int32_t currentActiveVersion) = 0;
};

template<typename DATA, typename DAO, std::size_t MaxSize>
class SharedMemoryArrayRepository : public SharedMemoryRepository {
private:
    std::array<DATA, MaxSize> &m_array;

protected:
    std::shared_ptr<SharedMemoryService> m_sharedMemoryService;

    std::array<DATA, MaxSize> &getArray() const { return m_array; }

public:
    SharedMemoryArrayRepository(
            const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
            const std::shared_ptr<SharedMemorySegment> &segment, int32_t codeVersion, std::string uuid)
            : SharedMemoryRepository(segment, sizeof(DATA),
                                     std::string(type_name<DATA>()), codeVersion, uuid),
              m_array(sharedMemoryService->constructArray<DATA, MaxSize>(
                      segment, getSharedMemoryRepository())),
              m_sharedMemoryService(sharedMemoryService) {}

    [[nodiscard]] SharedMemoryType getType() const override {
        return SharedMemoryType::Array;
    }

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override {
        writer->StartObject();
        writer->String("repository");
        writer->String("SharedMemoryArrayRepository");
        writer->String("array");
        writer->StartArray();
        for (const auto &iter: m_array) {
            LOG_TRACE("serialize vector entry");
            if constexpr (std::is_base_of<JsonSerializable, DAO>()) {
                DAO dao(iter);
                dao.serialize(writer);
            } else if constexpr (std::is_same<SharedMemoryService::ShmString,
                    DATA>()) {
                writer->String(iter.c_str());
            }
        }
        writer->EndArray();
        writer->EndObject();
    }

    bool deserialize(const rapidjson::Value &obj) override {
        throw std::runtime_error("Unsupported operation");
    }
};

template<typename DATA, typename DAO>
class SharedMemoryVectorRepository : public SharedMemoryRepository {
private:
    using Vector = boost::interprocess::vector<
            DATA,
            boost::interprocess::allocator<
                    DATA, boost::interprocess::managed_mapped_file::segment_manager>>;
    Vector &m_vector;

protected:
    std::shared_ptr<SharedMemoryService> m_sharedMemoryService;

    [[nodiscard]] Vector &getVector() const { return m_vector; }

public:
    SharedMemoryVectorRepository(
            const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
            const std::shared_ptr<SharedMemorySegment> &segment, int32_t codeVersion, std::string uuid)
            : SharedMemoryRepository(segment, sizeof(DATA),
                                     std::string(type_name<DATA>()), codeVersion, uuid),
              m_vector(sharedMemoryService->constructVector<DATA>(
                      segment, getSharedMemoryRepository())),
              m_sharedMemoryService(sharedMemoryService) {}

    [[nodiscard]] SharedMemoryType getType() const override {
        return SharedMemoryType::Vector;
    }

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override {
        writer->StartObject();
        writer->String("repository");
        writer->String("SharedMemoryVectorRepository");
        writer->String("vector");
        writer->StartArray();
        for (const auto &iter: m_vector) {
            LOG_TRACE("serialize vector entry");
            if constexpr (std::is_base_of<JsonSerializable, DAO>()) {
                DAO dao(iter);
                dao.serialize(writer);
            } else if constexpr (std::is_same<SharedMemoryService::ShmString,
                    DATA>()) {
                writer->String(iter.c_str());
            }
        }
        writer->EndArray();
        writer->EndObject();
    }

    bool deserialize(const rapidjson::Value &obj) override {
        throw std::runtime_error("Unsupported operation");
    }
};

template<typename KEY, typename VALUE, typename KeyDao, typename ValueDao>
class SharedMemoryMapRepository : public SharedMemoryRepository {
private:
    using Map = boost::interprocess::map<
            KEY, VALUE, std::less<KEY>,
            boost::interprocess::allocator<
                    std::pair<const KEY, VALUE>,
                    boost::interprocess::managed_mapped_file::segment_manager>>;
    Map &m_map;

protected:
    std::shared_ptr<SharedMemoryService> m_sharedMemoryService;

    [[nodiscard]] Map &getMap() { return m_map; }

public:
    SharedMemoryMapRepository(
            const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
            const std::shared_ptr<SharedMemorySegment> &segment, int32_t codeVersion, std::string uuid,
            int32_t sizeOfData = sizeof(VALUE))
            : SharedMemoryRepository(segment, sizeOfData,
                                     std::string(type_name<VALUE>()), codeVersion, uuid),
              m_map(sharedMemoryService->constructMap<KEY, VALUE>(
                      segment, getSharedMemoryRepository())),
              m_sharedMemoryService(sharedMemoryService) {
        static_assert(isSerializable<KeyDao>());
        static_assert(isSerializable<ValueDao>());
        LOG_TRACE("map loaded with size of {}", m_map.size());
    }

    [[nodiscard]] SharedMemoryType getType() const override {
        return SharedMemoryType::Map;
    }

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override {
        writer->StartObject();
        writer->String("repository");
        writer->String("SharedMemoryMapRepository");
        writer->String("map");
        writer->StartArray();
        for (auto &iterator: m_map) {
            const KEY &key = iterator.first;
            const VALUE &value = iterator.second;
            LOG_TRACE("serialize map entry");
            writer->StartObject();
            writer->String("key");
            if constexpr (std::is_base_of<JsonSerializable, KeyDao>()) {
                KeyDao dao(key);
                dao.serialize(writer);
            } else if constexpr (std::is_same<SharedMemoryService::ShmString,
                    KEY>()) {
                writer->String(key.c_str());
            }
            writer->String("value");
            if constexpr (std::is_base_of<JsonSerializable, ValueDao>()) {
                ValueDao dao(value);
                dao.serialize(writer);
            } else if constexpr (std::is_same<SharedMemoryService::ShmString,
                    VALUE>()) {
                writer->String(value.c_str());
            }
            writer->EndObject();
        }
        writer->EndArray();
        writer->EndObject();
    }

    bool deserialize(const rapidjson::Value &obj) override {
        throw std::runtime_error("Unsupported operation");
    }
};

template<typename DATA, typename DAO>
class SharedMemoryObjectRepository : public SharedMemoryRepository {
private:
    DATA &m_data;

protected:
    std::shared_ptr<SharedMemoryService> m_sharedMemoryService;

    DATA &getData() const { return m_data; }

public:
    SharedMemoryObjectRepository(
            const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
            const std::shared_ptr<SharedMemorySegment> &segment, int32_t codeVersion, std::string uuid)
            : SharedMemoryRepository(segment, sizeof(DATA),
                                     std::string(type_name<DATA>()), codeVersion, uuid),
              m_data(sharedMemoryService->constructObject<DATA>(
                      segment, getSharedMemoryRepository())),
              m_sharedMemoryService(sharedMemoryService) {}

    [[nodiscard]] SharedMemoryType getType() const override {
        return SharedMemoryType::Object;
    }

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override {
        writer->StartObject();
        writer->String("repository");
        writer->String("SharedMemoryObjectRepository");
        writer->String("object");
        if constexpr (std::is_base_of<JsonSerializable, DAO>()) {
            DAO dao(m_data);
            dao.serialize(writer);
        } else if constexpr (std::is_same<SharedMemoryService::ShmString, DATA>()) {
            writer->String(m_data.c_str());
        }
        writer->EndObject();
    }

    bool deserialize(const rapidjson::Value &obj) override {
        throw std::runtime_error("Unsupported operation");
    }
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYREPOSITORY_H
