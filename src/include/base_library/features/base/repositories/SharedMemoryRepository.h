#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYREPOSITORY_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYREPOSITORY_H

#include <base_library/core/models/JsonSerializable.h>

#include <memory>

#include <boost/container/map.hpp>
#include <boost/container/vector.hpp>

#include "base_library/core/models/SharedMemorySegment.h"
#include "base_library/core/services/LoggerMacros.h"
#include "base_library/core/services/PersistableBean.h"
#include "base_library/features/base/services/SharedMemoryService.h"
#include "base_library/core/utils/TypeName.h"

/** Types of shared memory data structures. */
enum SharedMemoryType {
    Map, Vector, Array, Object
};

/**
 * Base class for shared-memory-backed repositories with JSON serialization.
 */
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
    /**
     * Constructor.
     * @param sharedMemorySegment the underlying shared memory segment
     * @param sizeOfData size of each data element in bytes
     * @param sharedMemoryRepository name/path of this repository
     * @param codeVersion version of the code that created this repository
     * @param uuid unique identifier for this repository instance
     */
    SharedMemoryRepository(
            std::shared_ptr<SharedMemorySegment> sharedMemorySegment,
            std::size_t sizeOfData, std::string_view sharedMemoryRepository,
            int32_t codeVersion, std::string uuid);

    /** @return the underlying shared memory segment */
    [[nodiscard]] const std::shared_ptr<SharedMemorySegment>
    &getSharedMemorySegment() const;

    /** @return size of each data element in bytes */
    [[nodiscard]] std::size_t getSizeOfData() const;

    /** @return name/path of this repository */
    [[nodiscard]] const std::string &getSharedMemoryRepository() const;

    /** @return code version */
    [[nodiscard]] int32_t getCodeVersion() const;

    /** @return unique identifier */
    [[nodiscard]] const std::string &getUuid() const;

    /** @return the shared memory data structure type */
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
/**
 * Shared memory repository backed by a fixed-size array.
 * @tparam DATA element type
 * @tparam DAO serializable DAO wrapper for DATA
 * @tparam MaxSize maximum array size
 */
class SharedMemoryArrayRepository : public SharedMemoryRepository {
private:
    mutable std::array<DATA, MaxSize> *m_array;
    mutable std::size_t m_generation;

protected:
    std::shared_ptr<SharedMemoryService> m_sharedMemoryService;

    /** @return reference to the underlying array */
    std::array<DATA, MaxSize> &getArray() const {
        const auto generation = m_sharedMemoryService->getGeneration();
        if (generation != m_generation) {
            m_array = &m_sharedMemoryService->constructArray<DATA, MaxSize>(
                    getSharedMemorySegment(), getSharedMemoryRepository());
            m_generation = generation;
        }
        return *m_array;
    }

public:
    SharedMemoryArrayRepository(
            const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
            const std::shared_ptr<SharedMemorySegment> &segment, int32_t codeVersion, std::string uuid)
            : SharedMemoryRepository(segment, sizeof(DATA),
                                     std::string(type_name<DATA>()), codeVersion, uuid),
              m_array(&sharedMemoryService->constructArray<DATA, MaxSize>(
                      segment, getSharedMemoryRepository())),
              m_generation(sharedMemoryService->getGeneration()),
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
        for (const auto &iter: getArray()) {
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
/**
 * Shared memory repository backed by a dynamic vector.
 * @tparam DATA element type
 * @tparam DAO serializable DAO wrapper for DATA
 */
class SharedMemoryVectorRepository : public SharedMemoryRepository {
private:
    using Vector = boost::container::vector<
            DATA,
            boost::interprocess::allocator<
                    DATA, boost::interprocess::managed_mapped_file::segment_manager>>;
    mutable Vector *m_vector;
    mutable std::size_t m_generation;

protected:
    std::shared_ptr<SharedMemoryService> m_sharedMemoryService;

    [[nodiscard]] Vector &getVector() const {
        const auto generation = m_sharedMemoryService->getGeneration();
        if (generation != m_generation) {
            m_vector = &m_sharedMemoryService->constructVector<DATA>(
                    getSharedMemorySegment(), getSharedMemoryRepository());
            m_generation = generation;
        }
        return *m_vector;
    }

public:
    SharedMemoryVectorRepository(
            const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
            const std::shared_ptr<SharedMemorySegment> &segment, int32_t codeVersion, std::string uuid)
            : SharedMemoryRepository(segment, sizeof(DATA),
                                     std::string(type_name<DATA>()), codeVersion, uuid),
              m_vector(&sharedMemoryService->constructVector<DATA>(
                      segment, getSharedMemoryRepository())),
              m_generation(sharedMemoryService->getGeneration()),
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
        for (const auto &iter: getVector()) {
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
/**
 * Shared memory repository backed by an associative map.
 * @tparam KEY key type
 * @tparam VALUE mapped value type
 * @tparam KeyDao serializable DAO wrapper for KEY
 * @tparam ValueDao serializable DAO wrapper for VALUE
 */
class SharedMemoryMapRepository : public SharedMemoryRepository {
private:
    using Map = boost::container::map<
            KEY, VALUE, std::less<KEY>,
            boost::interprocess::allocator<
                    std::pair<const KEY, VALUE>,
                    boost::interprocess::managed_mapped_file::segment_manager>>;
    mutable Map *m_map;
    mutable std::size_t m_generation;

protected:
    std::shared_ptr<SharedMemoryService> m_sharedMemoryService;

    [[nodiscard]] Map &getMap() const {
        const auto generation = m_sharedMemoryService->getGeneration();
        if (generation != m_generation) {
            m_map = &m_sharedMemoryService->constructMap<KEY, VALUE>(
                    getSharedMemorySegment(), getSharedMemoryRepository());
            m_generation = generation;
        }
        return *m_map;
    }

public:
    SharedMemoryMapRepository(
            const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
            const std::shared_ptr<SharedMemorySegment> &segment, int32_t codeVersion, std::string uuid,
            int32_t sizeOfData = sizeof(VALUE))
            : SharedMemoryRepository(segment, sizeOfData,
                                     std::string(type_name<VALUE>()), codeVersion, uuid),
              m_map(&sharedMemoryService->constructMap<KEY, VALUE>(
                      segment, getSharedMemoryRepository())),
              m_generation(sharedMemoryService->getGeneration()),
              m_sharedMemoryService(sharedMemoryService) {
        static_assert(isSerializable<KeyDao>());
        static_assert(isSerializable<ValueDao>());
        LOG_TRACE("map loaded with size of {}", getMap().size());
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
        for (auto &iterator: getMap()) {
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
/**
 * Shared memory repository backed by a single object.
 * @tparam DATA object type
 * @tparam DAO serializable DAO wrapper for DATA
 */
class SharedMemoryObjectRepository : public SharedMemoryRepository {
private:
    mutable DATA *m_data;
    mutable std::size_t m_generation;

protected:
    std::shared_ptr<SharedMemoryService> m_sharedMemoryService;

    DATA &getData() const {
        const auto generation = m_sharedMemoryService->getGeneration();
        if (generation != m_generation) {
            m_data = &m_sharedMemoryService->constructObject<DATA>(
                    getSharedMemorySegment(), getSharedMemoryRepository());
            m_generation = generation;
        }
        return *m_data;
    }

public:
    SharedMemoryObjectRepository(
            const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
            const std::shared_ptr<SharedMemorySegment> &segment, int32_t codeVersion, std::string uuid)
            : SharedMemoryRepository(segment, sizeof(DATA),
                                     std::string(type_name<DATA>()), codeVersion, uuid),
              m_data(&sharedMemoryService->constructObject<DATA>(
                      segment, getSharedMemoryRepository())),
              m_generation(sharedMemoryService->getGeneration()),
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
            DAO dao(getData());
            dao.serialize(writer);
        } else if constexpr (std::is_same<SharedMemoryService::ShmString, DATA>()) {
            writer->String(getData().c_str());
        }
        writer->EndObject();
    }

    bool deserialize(const rapidjson::Value &obj) override {
        throw std::runtime_error("Unsupported operation");
    }
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYREPOSITORY_H
