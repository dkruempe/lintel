#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYREPOSITORY_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYREPOSITORY_H

#include <base_library/core/models/JsonSerializable.h>

#include <memory>

#include "base_library/core/models/SharedMemorySegment.h"
#include "base_library/core/services/PersistableBean.h"
#include "base_library/core/services/SharedMemoryService.h"
#include "base_library/core/utils/TypeName.h"

enum SharedMemoryType { Map, Set, Vector, Array, Object };

class SharedMemoryRepository : public JsonSerializable {
 private:
  std::shared_ptr<SharedMemorySegment> m_sharedMemorySegment;
  std::size_t m_sizeOfData;
  std::string m_sharedMemoryRepository;
  int32_t m_codeVersion;

 protected:
  template <typename TYPE>
  static constexpr bool isSerializable() {
    return std::is_base_of<JsonSerializable, TYPE>() ||
           std::is_same<SharedMemoryService::ShmString, TYPE>() ||
           std::is_arithmetic<TYPE>();
  }

 public:
  SharedMemoryRepository(
      std::shared_ptr<SharedMemorySegment> sharedMemorySegment,
      std::size_t sizeOfData, std::string_view sharedMemoryRepository,
      int32_t codeVersion);

  [[nodiscard]] const std::shared_ptr<SharedMemorySegment>
      &getSharedMemorySegment() const;
  [[nodiscard]] std::size_t getSizeOfData() const;
  [[nodiscard]] const std::string &getSharedMemoryRepository() const;
  [[nodiscard]] int32_t getCodeVersion() const;
  [[nodiscard]] virtual SharedMemoryType getType() const = 0;

  ~SharedMemoryRepository() override = default;

  virtual void onMigrate(int32_t currentActiveVersion) = 0;
};

template <typename DATA>
class SharedMemorySetRepository : public SharedMemoryRepository {
 private:
  using Set = boost::interprocess::set<
      DATA, std::less<DATA>,
      boost::interprocess::allocator<
          DATA, boost::interprocess::managed_mapped_file::segment_manager>>;
  Set &m_set;

 protected:
  std::shared_ptr<SharedMemoryService> m_sharedMemoryService;
  [[nodiscard]] Set &getSet() const { return m_set; }

 public:
  SharedMemorySetRepository(
      const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
      const std::shared_ptr<SharedMemorySegment> &segment, int32_t codeVersion)
      : SharedMemoryRepository(segment, sizeof(DATA), type_name<DATA>(),
                               codeVersion),
        m_set(sharedMemoryService->constructSet<DATA>(segment,
                                                      type_name<DATA>())),
        m_sharedMemoryService(sharedMemoryService) {}

  [[nodiscard]] SharedMemoryType getType() const override {
    return SharedMemoryType::Set;
  }
};

template <typename DATA, std::size_t MaxSize>
class SharedMemoryArrayRepository : public SharedMemoryRepository {
 private:
  std::array<DATA, MaxSize> &m_array;

 protected:
  std::shared_ptr<SharedMemoryService> m_sharedMemoryService;
  std::array<DATA, MaxSize> &getArray() const { return m_array; }

 public:
  SharedMemoryArrayRepository(
      const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
      const std::shared_ptr<SharedMemorySegment> &segment, int32_t codeVersion)
      : SharedMemoryRepository(segment, sizeof(DATA), type_name<DATA>(),
                               codeVersion),
        m_array(sharedMemoryService->constructArray<DATA, MaxSize>(
            segment, type_name<DATA>())),
        m_sharedMemoryService(sharedMemoryService) {}
  [[nodiscard]] SharedMemoryType getType() const override {
    return SharedMemoryType::Array;
  }
};

template <typename DATA>
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
      const std::shared_ptr<SharedMemorySegment> &segment, int32_t codeVersion)
      : SharedMemoryRepository(segment, sizeof(DATA), type_name<DATA>(),
                               codeVersion),
        m_vector(sharedMemoryService->constructVector<DATA>(segment,
                                                            type_name<DATA>())),
        m_sharedMemoryService(sharedMemoryService) {}
  [[nodiscard]] SharedMemoryType getType() const override {
    return SharedMemoryType::Vector;
  }
};

template <typename KEY, typename VALUE>
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
      const std::shared_ptr<SharedMemorySegment> &segment, int32_t codeVersion,
      int32_t sizeOfData = sizeof(VALUE))
      : SharedMemoryRepository(segment, sizeOfData,
                               std::string(type_name<VALUE>()), codeVersion),
        m_map(sharedMemoryService->constructMap<KEY, VALUE>(
            segment, getSharedMemoryRepository())),
        m_sharedMemoryService(sharedMemoryService) {
    static_assert(isSerializable<KEY>());
    static_assert(isSerializable<VALUE>());
    LOG_TRACE("map loaded with size of {}", m_map.size());
  }
  [[nodiscard]] SharedMemoryType getType() const override {
    return SharedMemoryType::Map;
  }
  virtual void serialize(
      rapidjson::Writer<rapidjson::StringBuffer> *writer) const override {
    writer->StartObject();
    writer->String("repository");
    writer->String("SharedMemoryMapRepository");
    writer->String("map");
    writer->StartArray();
    for (auto &iterator : m_map) {
      KEY key = iterator.first;
      VALUE value = iterator.second;
      LOG_TRACE("serialize map entry");
      writer->StartObject();
      writer->String("key");
      if constexpr (std::is_base_of<JsonSerializable, KEY>()) {
        key.serialize(writer);
      } else if constexpr (std::is_same<SharedMemoryService::ShmString,
                                        KEY>()) {
        writer->String(key.c_str());
      }
      writer->String("value");
      if constexpr (std::is_base_of<JsonSerializable, VALUE>()) {
        value.serialize(writer);
      } else if constexpr (std::is_same<SharedMemoryService::ShmString,
                                        VALUE>()) {
        writer->String(value.c_str());
      }
      writer->EndObject();
    }
    writer->EndArray();
    writer->EndObject();
  }
  virtual bool deserialize(const rapidjson::Value &obj) override {
    bool success = true;
    // repository
    if (obj.HasMember("repository")) {
      std::string repository = obj["repository"].GetString();
      if (repository != "SharedMemoryMapRepository") {
        LOG_ERROR("wrong repository defined => abort serialization");
        return false;
      }
    } else {
      LOG_ERROR("no repository defined => abort serialization");
      return false;
    }

    if (!obj.HasMember("map")) {
      LOG_ERROR("map not defined => abort serialization");
      return false;
    }
    for (const auto &iter : obj["map"].GetArray()) {
      if (!iter.HasMember("key") || !iter.HasMember("value")) {
        LOG_ERROR("failed to deserialize key or value of map");
        success = false;
        break;
      }
      std::shared_ptr<KEY> key;
      if constexpr (std::is_same<JsonSerializable, KEY>()) {
        key->deserialize(iter["key"]);
      } else if constexpr (std::is_same<KEY,
                                        SharedMemoryService::ShmString>()) {
        key = std::make_shared<KEY>(m_sharedMemoryService->constructString(
            getSharedMemorySegment(), iter["key"].GetString()));
      }
      std::shared_ptr<VALUE> value;
      if constexpr (std::is_same<JsonSerializable, VALUE>()) {
        value->deserialize(iter["value"]);
      } else if constexpr (std::is_same<VALUE,
                                        SharedMemoryService::ShmString>()) {
        value = std::make_shared<VALUE>(m_sharedMemoryService->constructString(
            getSharedMemorySegment(), iter["value"].GetString()));
      }
      KEY copyKey = *key;
      VALUE copyValue = *value;
      m_map.insert({copyKey, copyValue});
    }
    return success;
  }
};

template <typename DATA>
class SharedMemoryObjectRepository : public SharedMemoryRepository {
 private:
  DATA &m_data;

 protected:
  std::shared_ptr<SharedMemoryService> m_sharedMemoryService;
  DATA &getData() const { return m_data; }

 public:
  SharedMemoryObjectRepository(
      const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
      const std::shared_ptr<SharedMemorySegment> &segment, int32_t codeVersion)
      : SharedMemoryRepository(segment, sizeof(DATA), type_name<DATA>(),
                               codeVersion),
        m_data(sharedMemoryService->constructObject<DATA>(segment,
                                                          type_name<DATA>())),
        m_sharedMemoryService(sharedMemoryService) {}
  [[nodiscard]] SharedMemoryType getType() const override {
    return SharedMemoryType::Object;
  }
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYREPOSITORY_H
