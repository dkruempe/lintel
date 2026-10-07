#ifndef LINTEL_SHAREDMEMORYREPOSITORYDTO_H
#define LINTEL_SHAREDMEMORYREPOSITORYDTO_H

#include "lintel/core/models/JsonSerializable.h"
#include "lintel/features/base/repositories/SharedMemoryRepository.h"

/** DTO representing a shared memory repository */
class SharedMemoryRepositoryDto : public JsonSerializable {
private:
    /** The repository UUID */
    std::string m_uuid;
    /** The repository name */
    std::string m_name;
    /** The segment name this repository belongs to */
    std::string m_segmentName;
    /** The shared memory type */
    SharedMemoryType m_type;
    /** The size of the repository */
    std::size_t m_size;
    /** The current version number */
    int32_t m_currentVersion;

    /** JSON field name constants */
    static struct Shapes {
        const char *const UUID = "uuid";
        const char *const NAME = "name";
        const char *const SEGMENT_NAME = "segment_name";
        const char *const TYPE = "type";
        const char *const SIZE = "size";
        const char *const VERSION = "version";
    } m_shape;

public:
    /** Construct from a SharedMemoryRepository model
     * @param repository The source repository */
    explicit SharedMemoryRepositoryDto(const SharedMemoryRepository &repository);

    /** Default constructor */
    SharedMemoryRepositoryDto() = default;

    /** Get the UUID
     * @return The UUID string */
    [[nodiscard]] const std::string &getUuid() const;

    /** Get the repository name
     * @return The name */
    [[nodiscard]] const std::string &getName() const;

    /** Get the segment name
     * @return The segment name */
    [[nodiscard]] const std::string &getSegmentName() const;

    /** Get the shared memory type
     * @return The type */
    [[nodiscard]] SharedMemoryType getType() const;

    /** Get the repository size
     * @return The size */
    [[nodiscard]] size_t getSize() const;

    /** Get the current version
     * @return The version number */
    [[nodiscard]] int32_t getCurrentVersion() const;

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON
     * @param obj The JSON value
     * @return True on success */
    bool deserialize(const rapidjson::Value &obj) override;
};

#endif  // LINTEL_SHAREDMEMORYREPOSITORYDTO_H
