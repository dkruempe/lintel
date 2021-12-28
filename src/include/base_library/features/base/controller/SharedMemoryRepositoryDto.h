#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYREPOSITORYDTO_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYREPOSITORYDTO_H

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/repositories/SharedMemoryRepository.h"

class SharedMemoryRepositoryDto : public JsonSerializable {
 private:
  std::string m_name;
  std::string m_segmentName;
  SharedMemoryType m_type;
  std::size_t m_size;
  int32_t m_currentVersion;

  static struct Shapes {
    const std::string NAME = "name";
    const std::string SEGMENT_NAME = "segment_name";
    const std::string TYPE = "type";
    const std::string SIZE = "size";
    const std::string VERSION = "version";
  } m_shape;

 public:
  explicit SharedMemoryRepositoryDto(const SharedMemoryRepository &repository);
  SharedMemoryRepositoryDto() = default;
  [[nodiscard]] const std::string &getName() const;
  [[nodiscard]] const std::string &getSegmentName() const;
  [[nodiscard]] SharedMemoryType getType() const;
  [[nodiscard]] size_t getSize() const;
  [[nodiscard]] int32_t getCurrentVersion() const;
  void serialize(
      rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;
  bool deserialize(const rapidjson::Value &obj) override;
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYREPOSITORYDTO_H
