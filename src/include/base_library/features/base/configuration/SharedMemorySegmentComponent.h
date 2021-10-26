#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTCOMPONENT_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTCOMPONENT_H

#include "Component.h"

class SharedMemorySegmentComponent : public Component {
 private:
  static struct Shapes {
    const std::string CONFIG_ROOT = "SharedMemorySegments";
    const std::string SHM_SEGMENT_ROOT = "SharedMemorySegment";
    const std::string SHM_SEGMENT_NAME = "name";
    const std::string SHM_SEGMENT_SIZE = "size";
    const std::string SHM_SEGMENT_AUTO_EXTEND_SIZE = "auto_extend_size";
    const std::string SHM_SEGMENT_MAX_SIZE = "max_size";
    const std::string PATH_ROOT = "Path";
    const std::string PATH_PATH = "path";
  } shape;

 public:
  SharedMemorySegmentComponent();

  std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                            const std::string &fileName,
                                            int32_t lineOffset) override;
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTCOMPONENT_H
