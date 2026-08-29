#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENT_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENT_H

#include <filesystem>
#include <string>

/**
 * Represents a shared memory segment with path, name, size, and auto-extend configuration.
 */
class SharedMemorySegment {
private:
    std::filesystem::path m_path;
    std::string m_name;
    std::size_t m_size;
    bool m_isAutoExtend;
    std::size_t m_autoExtendSize;
    std::size_t m_maxSize;

public:
    /**
     * Constructs a shared memory segment with a fixed size.
     * @param sharedMemoryPath path to the shared memory file
     * @param name name of the segment
     * @param size size of the segment in bytes
     */
    SharedMemorySegment(std::filesystem::path sharedMemoryPath, std::string name,
                        std::size_t size);

    /**
     * Constructs a shared memory segment with auto-extend capability.
     * @param sharedMemoryPath path to the shared memory file
     * @param name name of the segment
     * @param size initial size of the segment in bytes
     * @param autoExtendSize number of bytes to extend by when full
     * @param maxSize maximum allowed size in bytes
     */
    SharedMemorySegment(std::filesystem::path sharedMemoryPath, std::string name,
                        std::size_t size, std::size_t autoExtendSize,
                        std::size_t maxSize);

    /** @return path to the shared memory file */
    [[nodiscard]] std::filesystem::path getPath() const;

    /** @return reference to the segment name */
    [[nodiscard]] const std::string &getName() const;

    /** @return size of the segment in bytes */
    [[nodiscard]] constexpr size_t getSize() const { return m_size; }

    /** @return true if the segment can auto-extend when full */
    [[nodiscard]] constexpr bool isAutoExtend() const { return m_isAutoExtend; }

    /** @return number of bytes to extend by when full */
    [[nodiscard]] constexpr size_t getAutoExtendSize() const { return m_autoExtendSize; }

    /** @return maximum allowed size in bytes */
    [[nodiscard]] constexpr size_t getMaxSize() const { return m_maxSize; }
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENT_H
