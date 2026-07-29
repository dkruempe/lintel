#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTCOMPONENT_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTCOMPONENT_H

#include <cstddef>
#include <filesystem>
#include <memory>
#include <vector>

#include "Component.h"
#include "base_library/features/base/configuration/EnvironmentConfiguration.h"

namespace tinyxml2 {
    class XMLElement;
}

class SharedMemorySegmentComponent : public Component {
private:
    std::shared_ptr<EnvironmentConfiguration> m_environmentConfiguration;

    static const struct Shapes {
        const char *const CONFIG_ROOT = "SharedMemorySegments";
        const char *const SHM_SEGMENT_ROOT = "SharedMemorySegment";
        const char *const SHM_SEGMENT_NAME = "name";
        const char *const SHM_SEGMENT_SIZE = "size";
        const char *const SHM_SEGMENT_AUTO_EXTEND_SIZE = "auto_extend_size";
        const char *const SHM_SEGMENT_MAX_SIZE = "max_size";
        const char *const PATH_ROOT = "Path";
        const char *const PATH_PATH = "path";
    } m_shape;

    struct SegmentTemp {
        bool m_autoExtend = false;
        std::string m_name;
        std::size_t m_size = 0;
        std::size_t m_maxSize = 0;
        std::size_t m_autoExtendSize = 0;
    };

    void processPathElement(tinyxml2::XMLElement *shmElement, int32_t lineNumber,
                            std::filesystem::path &path);

    void processSegmentElement(tinyxml2::XMLElement *shmElement, int32_t lineNumber,
                               std::vector<SegmentTemp> &segments);

    void validateAndCreatePath(const std::filesystem::path &path, int32_t lineOffset);

    static std::vector<std::shared_ptr<Entry>> buildEntries(
            const std::filesystem::path &path,
            const std::vector<SegmentTemp> &segments);

public:
    explicit SharedMemorySegmentComponent(
            std::shared_ptr<EnvironmentConfiguration> environmentConfiguration);

    std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                              const std::string &fileName,
                                              int32_t lineOffset) override;
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTCOMPONENT_H
