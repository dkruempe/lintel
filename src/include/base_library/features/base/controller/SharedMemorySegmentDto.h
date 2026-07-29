#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTDTO_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTDTO_H

#include <filesystem>

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/models/SharedMemorySegmentInfo.h"

class SharedMemorySegmentDto : public JsonSerializable {
private:
    // SharedMemorySegment
    std::filesystem::path m_path;
    std::string m_name;
    std::size_t m_size;
    bool m_isAutoExtend;
    std::size_t m_autoExtendSize;
    std::size_t m_maxSize;
    // Information
    bool m_isSanity;
    std::size_t m_currentSize;
    std::size_t m_freeSize;
    std::size_t m_amountNamedObjects;
    std::size_t m_amountUniqueObjects;

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
    explicit SharedMemorySegmentDto(
            const SharedMemorySegmentInfo &sharedMemoryInfo);

    SharedMemorySegmentDto() = default;

    [[nodiscard]] const std::filesystem::path &getPath() const;

    [[nodiscard]] const std::string &getName() const;

    [[nodiscard]] size_t getSize() const;

    [[nodiscard]] bool isAutoExtend() const;

    [[nodiscard]] size_t getAutoExtendSize() const;

    [[nodiscard]] size_t getMaxSize() const;

    [[nodiscard]] bool isSanity() const;

    [[nodiscard]] size_t getCurrentSize() const;

    [[nodiscard]] size_t getFreeSize() const;

    [[nodiscard]] size_t getAmountNamedObjects() const;

    [[nodiscard]] size_t getAmountUniqueObjects() const;

    static const Shapes &getShape();

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    bool deserialize(const rapidjson::Value &obj) override;
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENTDTO_H
