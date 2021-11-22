#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYREPOSITORY_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYREPOSITORY_H

#include <memory>

#include "base_library/core/models/SharedMemorySegment.h"
#include "base_library/core/services/PersistableBean.h"
#include "base_library/core/services/SharedMemoryService.h"
#include "base_library/core/utils/TypeName.h"

class SharedMemoryRepository {
 private:
  std::shared_ptr<SharedMemorySegment> m_sharedMemorySegment;
  std::size_t m_sizeOfData;
  std::string m_sharedMemoryRepository;
  int32_t m_codeVersion;

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

  virtual ~SharedMemoryRepository() = default;

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
      const std::shared_ptr<SharedMemorySegment> &segment, int32_t codeVersion)
      : SharedMemoryRepository(segment, sizeof(VALUE),
                               std::string(type_name<VALUE>()), codeVersion),
        m_map(sharedMemoryService->constructMap<KEY, VALUE>(
            segment, getSharedMemoryRepository())),
        m_sharedMemoryService(sharedMemoryService) {
    LOG_TRACE("map loaded with size of {}", m_map.size());
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
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYREPOSITORY_H
