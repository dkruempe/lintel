#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYSERVICE_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYSERVICE_H

#include <array>
#include <atomic>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <filesystem>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <utility>

#include "base_library/core/exceptions/ShmSegmentNotFound.h"
#include "base_library/core/models/SharedMemorySegment.h"
#include "base_library/core/services/AbstractService.h"
#include "base_library/features/base/services/SchedulerService.h"
#include "base_library/features/base/services/SharedMemorySegmentManager.h"
#include "base_library/features/property/models/Property.h"

class SharedMemoryService : public AbstractService<SharedMemoryService> {
 private:
  struct MappedFile {
    std::shared_ptr<boost::interprocess::managed_mapped_file>
        m_managedMappedFile;
    std::shared_ptr<SharedMemorySegment> m_sharedMemorySegment;
  };
  // variables
  std::map<std::string, MappedFile> m_segments;
  std::shared_ptr<SchedulerService> m_schedulerService;
  std::atomic_bool m_running;
  // properties
  DEFINE_PROPERTY(m_scheduleRate, std::chrono::seconds, std::chrono::seconds(2),
                  "schedule rate of user tokens checks in seconds", true);
  DEFINE_PROPERTY(m_autoExtend, bool, true, "auto extend shared memory", true);
  DEFINE_PROPERTY(m_autoExtendEpsilon, std::size_t, 1000,
                  "epsilon when auto extend gets triggered", true);

  // initializer function
  static std::map<std::string, MappedFile> create(
      const std::vector<std::shared_ptr<SharedMemorySegment>> &set);

  void onCheck();

 public:
  typedef boost::interprocess::allocator<
      char, boost::interprocess::managed_mapped_file::segment_manager>
      charAllocator;
  typedef boost::interprocess::basic_string<char, std::char_traits<char>,
                                            charAllocator>
      ShmString;

  SharedMemoryService(const std::shared_ptr<SharedMemorySegmentManager>
                          &sharedMemorySegmentManager,
                      std::shared_ptr<SchedulerService> schedulerService,
                      const std::shared_ptr<ProcessName> &processName);

  virtual ~SharedMemoryService() = default;
  void onInitialize() override;
  void onShutdown() override;

  /**
   * grows the size of the mentioned shared memory block
   * @param sharedMemoryName
   * @param grow size to be added to shared memory itself
   */
  static void growOf(const SharedMemorySegment &segment, std::size_t grow);

  /**
   * shrinks the shared memory size to the minimum
   * @param sharedMemoryName
   */
  static void shrinkOf(const SharedMemorySegment &segment);

  /**
   * shows state of the mentioned shared memory segment
   * @param sharedMemoryName
   * @return string with the printed information
   */
  [[nodiscard]] std::string showStateOf(
      const std::string &sharedMemoryName) const;

  template <class Object, std::size_t size>
  std::array<Object, size> &constructArray(const std::string &sharedMemoryName,
                                           const std::string &name) {
    try {
      auto &segment = m_segments.at(sharedMemoryName);
      return *(
          segment.m_managedMappedFile
              ->find_or_construct<std::array<Object, size>>(name.c_str())());
    } catch (std::out_of_range &exception) {
      throw ShmSegmentNotFound(sharedMemoryName);
    }
  }

  template <class Object>
  boost::interprocess::set<
      Object, std::less<Object>,
      boost::interprocess::allocator<
          Object, boost::interprocess::managed_mapped_file::segment_manager>> &
  constructSet(const std::string &sharedMemoryName, const std::string &name) {
    typedef boost::interprocess::allocator<
        Object, boost::interprocess::managed_mapped_file::segment_manager>
        persistentSetAllocator;
    try {
      auto &segment = m_segments.at(sharedMemoryName);
      persistentSetAllocator allocator(
          segment.m_managedMappedFile->get_segment_manager());
      return *(segment.m_managedMappedFile
                   ->find_or_construct<boost::interprocess::set<
                       Object, std::less<Object>, persistentSetAllocator>>(
                       name.c_str())(std::less<Object>(), allocator));
    } catch (std::out_of_range &exception) {
      throw ShmSegmentNotFound(sharedMemoryName);
    }
  }

  template <class Key, class Value>
  boost::interprocess::map<
      Key, Value, std::less<Key>,
      boost::interprocess::allocator<
          std::pair<const Key, Value>,
          boost::interprocess::managed_mapped_file::segment_manager>> &
  constructMap(const std::string &sharedMemoryName, const std::string &name) {
    typedef std::pair<const Key, Value> pairType;
    typedef boost::interprocess::allocator<
        pairType, boost::interprocess::managed_mapped_file::segment_manager>
        persistentMapAllocator;
    try {
      auto &segment = m_segments.at(sharedMemoryName);
      persistentMapAllocator allocator(
          segment.m_managedMappedFile->get_segment_manager());
      return *(segment.m_managedMappedFile
                   ->find_or_construct<boost::interprocess::map<
                       Key, Value, std::less<Key>, persistentMapAllocator>>(
                       name.c_str())(std::less<Key>(), allocator));
    } catch (std::out_of_range &exception) {
      throw ShmSegmentNotFound(sharedMemoryName);
    }
  }

  template <class Object>
  boost::interprocess::vector<
      Object,
      boost::interprocess::allocator<
          Object, boost::interprocess::managed_mapped_file::segment_manager>>
      &constructVector(const std::string &sharedMemoryName,
                       const std::string &name) {
    typedef boost::interprocess::allocator<
        Object, boost::interprocess::managed_mapped_file::segment_manager>
        persistentVectorAllocator;
    try {
      auto &segment = m_segments.at(sharedMemoryName);
      persistentVectorAllocator allocator(
          segment.m_managedMappedFile->get_segment_manager());
      return *(segment.m_managedMappedFile->find_or_construct<
               boost::interprocess::vector<Object, persistentVectorAllocator>>(
          name.c_str())(allocator));
    } catch (std::out_of_range &exception) {
      throw ShmSegmentNotFound(sharedMemoryName);
    }
  }

  template <class Object>
  Object &constructObject(const std::string &sharedMemoryName,
                          const std::string &name) {
    try {
      auto &segment = m_segments.at(sharedMemoryName);
      return *(segment.m_managedMappedFile->find_or_construct<Object>(
          name.c_str())());
    } catch (std::out_of_range &exception) {
      throw ShmSegmentNotFound(sharedMemoryName);
    }
  }

  ShmString constructString(const std::string &sharedMemoryName,
                            const std::string &string);

  friend std::ostream &operator<<(std::ostream &os,
                                  const SharedMemoryService &service);
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYSERVICE_H
