#ifndef CPP_SYSTEM_LIBRARY_ABSTRACTSHMCONFIG_H
#define CPP_SYSTEM_LIBRARY_ABSTRACTSHMCONFIG_H

#include <map>
#include <string>
#include <vector>

#include "base_library/models/SharedMemorySegment.h"

class AbstractShmConfig {
private:
  std::vector<SharedMemorySegment> segments;
  std::map<std::string_view, SharedMemorySegment> map;
  static std::map<std::string_view, SharedMemorySegment>
  createMap(std::vector<SharedMemorySegment> &segments,
            const std::map<std::string_view, std::string_view> &map);

protected:
  void set(std::vector<SharedMemorySegment> segmentsIn,
           const std::map<std::string_view, std::string_view> &mapIn);

public:
  /**
   * constructor
   * @param segments SharedMemorySegments
   * @param map Repository name to SharedMemorySegments name map
   */
  AbstractShmConfig(std::vector<SharedMemorySegment> segments,
                    const std::map<std::string_view, std::string_view> &map);
  AbstractShmConfig() = default;
  [[nodiscard]] const SharedMemorySegment &
  of(const std::string_view &repository) const;
  [[nodiscard]] const std::vector<SharedMemorySegment> &getSegments() const;
};

#endif // CPP_SYSTEM_LIBRARY_ABSTRACTSHMCONFIG_H