#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTDTO_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTDTO_H

#include <filesystem>

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/models/SharedMemorySegmentInfo.h"

/** DTO representing detailed shared memory segment information */
class SharedMemorySegmentDto : public JsonSerializable {
private:
    /** The filesystem path for the segment */
    std::filesystem::path m_path;
    /** The segment name */
    std::string m_name;
    /** The configured segment size */
    std::size_t m_size;
    /** Whether auto-extend is enabled */
    bool m_isAutoExtend;
    /** The auto-extend increment size */
    std::size_t m_autoExtendSize;
    /** The maximum segment size */
    std::size_t m_maxSize;
    /** Whether the segment passed sanity check */
    bool m_isSanity;
    /** The current allocated size */
    std::size_t m_currentSize;
    /** The remaining free size */
    std::size_t m_freeSize;
    /** The number of named objects */
    std::size_t m_amountNamedObjects;
    /** The number of unique objects */
    std::size_t m_amountUniqueObjects;

    /** JSON field name constants */
    static struct Shapes {
        const char *const PATH = "path";
        const char *const NAME = "name";
        const char *const SIZE = "size";
        const char *const AUTO_EXTEND = "auto_extend";
        const char *const AUTO_EXTEND_SIZE = "auto_extend_size";
        const char *const MAX_SIZE = "max_size";
        const char *const SANITY = "sanity";
        const char *const CURRENT_SIZE = "current_size";
        const char *const FREE_SIZE = "free_size";
        const char *const NAMED_OBJECTS = "named_objects";
        const char *const UNIQUE_OBJECTS = "unique_objects";
    } m_shape;

public:
    /** Construct from a SharedMemorySegmentInfo model
     * @param sharedMemoryInfo The source segment info */
    explicit SharedMemorySegmentDto(
            const SharedMemorySegmentInfo &sharedMemoryInfo);

    /** Default constructor */
    SharedMemorySegmentDto() = default;

    /** Get the filesystem path
     * @return The path */
    [[nodiscard]] const std::filesystem::path &getPath() const;

    /** Get the segment name
     * @return The name */
    [[nodiscard]] const std::string &getName() const;

    /** Get the configured segment size
     * @return The size */
    [[nodiscard]] size_t getSize() const;

    /** Check if auto-extend is enabled
     * @return True if auto-extend */
    [[nodiscard]] bool isAutoExtend() const;

    /** Get the auto-extend increment size
     * @return The auto-extend size */
    [[nodiscard]] size_t getAutoExtendSize() const;

    /** Get the maximum segment size
     * @return The max size */
    [[nodiscard]] size_t getMaxSize() const;

    /** Check if the segment passed sanity check
     * @return True if sane */
    [[nodiscard]] bool isSanity() const;

    /** Get the current allocated size
     * @return The current size */
    [[nodiscard]] size_t getCurrentSize() const;

    /** Get the remaining free size
     * @return The free size */
    [[nodiscard]] size_t getFreeSize() const;

    /** Get the number of named objects
     * @return The count */
    [[nodiscard]] size_t getAmountNamedObjects() const;

    /** Get the number of unique objects
     * @return The count */
    [[nodiscard]] size_t getAmountUniqueObjects() const;

    /** Get the shape (field name) constants
     * @return Reference to the Shapes struct */
    static const Shapes &getShape();

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON
     * @param obj The JSON value
     * @return True on success */
    bool deserialize(const rapidjson::Value &obj) override;
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTDTO_H
