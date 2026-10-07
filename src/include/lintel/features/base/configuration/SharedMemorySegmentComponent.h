#ifndef LINTEL_SHAREDMEMORYSEGMENTCOMPONENT_H
#define LINTEL_SHAREDMEMORYSEGMENTCOMPONENT_H

#include <cstddef>
#include <filesystem>
#include <memory>
#include <vector>

#include "Component.h"
#include "lintel/features/base/configuration/EnvironmentConfiguration.h"

namespace tinyxml2 {
    class XMLElement;
}

/** Component for parsing shared memory segment configurations from XML */
class SharedMemorySegmentComponent : public Component {
private:
    /** The environment configuration for resolving paths */
    std::shared_ptr<EnvironmentConfiguration> m_environmentConfiguration;

    /** XML element name constants for shared memory segment parsing */
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

    /** Temporary storage for segment data during parsing */
    struct SegmentTemp {
        bool m_autoExtend = false; /**< Whether auto-extend is enabled */
        std::string m_name; /**< Segment name */
        std::size_t m_size = 0; /**< Segment size */
        std::size_t m_maxSize = 0; /**< Maximum segment size */
        std::size_t m_autoExtendSize = 0; /**< Auto-extend increment size */
    };

    /** Process a Path XML element
     * @param shmElement The XML element to process
     * @param lineNumber Current line number
     * @param path Output path reference */
    void processPathElement(tinyxml2::XMLElement *shmElement, int32_t lineNumber,
                            std::filesystem::path &path);

    /** Process a SharedMemorySegment XML element
     * @param shmElement The XML element to process
     * @param lineNumber Current line number
     * @param segments Output vector of segment temps */
    void processSegmentElement(tinyxml2::XMLElement *shmElement, int32_t lineNumber,
                               std::vector<SegmentTemp> &segments);

    /** Validate and create the shared memory path if it does not exist
     * @param path The path to validate
     * @param lineOffset Line offset for error reporting */
    void validateAndCreatePath(const std::filesystem::path &path, int32_t lineOffset);

    /** Build entries from parsed path and segment data
     * @param path The shared memory filesystem path
     * @param segments The parsed segment data
     * @return Vector of shared memory segment entries */
    static std::vector<std::shared_ptr<Entry>> buildEntries(
            const std::filesystem::path &path,
            const std::vector<SegmentTemp> &segments);

public:
    /** Construct a SharedMemorySegmentComponent
     * @param environmentConfiguration The environment configuration */
    explicit SharedMemorySegmentComponent(
            std::shared_ptr<EnvironmentConfiguration> environmentConfiguration);

    /** Parse shared memory segment configuration XML
     * @param content XML content to parse
     * @param fileName Source file name for error reporting
     * @param lineOffset Line offset for error reporting
     * @return Vector of parsed shared memory segment entries */
    std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                              const std::string &fileName,
                                              int32_t lineOffset) override;
};

#endif  // LINTEL_SHAREDMEMORYSEGMENTCOMPONENT_H
