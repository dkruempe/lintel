#include "base_library/features/base/configuration/SharedMemorySegmentComponent.h"

#include <base_library/core/configuration/ConfigurationException.h>
#include <base_library/core/configuration/EnvironmentConfiguration.h>
#include <base_library/features/base/configuration/SharedMemorySegmentEntry.h>
#include <tinyxml2.h>

#include <filesystem>
import base_library.core.utils;
import base_library.core.utils.type_name;

const SharedMemorySegmentComponent::Shapes SharedMemorySegmentComponent::m_shape{};

SharedMemorySegmentComponent::SharedMemorySegmentComponent(
        std::shared_ptr<EnvironmentConfiguration> environmentConfiguration)
        : Component(m_shape.CONFIG_ROOT),
          m_environmentConfiguration(std::move(environmentConfiguration)) {}

void SharedMemorySegmentComponent::processPathElement(
        tinyxml2::XMLElement *shmElement, int32_t lineNumber,
        std::filesystem::path &path) {
    const char *pathStr = shmElement->Attribute(m_shape.PATH_PATH);
    if (pathStr == nullptr) {
        throw ConfigurationException(getConfigRoot(), "path value is nullptr",
                                     lineNumber);
    }
    std::string tmpPath = pathStr;
    if (!tmpPath.empty() && tmpPath[0] == '~') {
        std::string rest(tmpPath.begin() + 1, tmpPath.end());
        tmpPath = m_environmentConfiguration->of(EnvironmentConfiguration::Home);
        tmpPath += '/';
        tmpPath += rest;
    }
    path = std::filesystem::path(tmpPath);
    if (path.empty() || (!is_directory(path) && exists(path))) {
        throw ConfigurationException(getConfigRoot(), "wrong configured path",
                                     lineNumber);
    }
}

void SharedMemorySegmentComponent::processSegmentElement(
        tinyxml2::XMLElement *shmElement, int32_t lineNumber,
        std::vector<SegmentTemp> &segments) {
    const char *name = shmElement->Attribute(m_shape.SHM_SEGMENT_NAME);
    const char *sizeStr = shmElement->Attribute(m_shape.SHM_SEGMENT_SIZE);
    const char *maxSizeStr =
            shmElement->Attribute(m_shape.SHM_SEGMENT_MAX_SIZE);
    const char *autoExtendSizeStr =
            shmElement->Attribute(m_shape.SHM_SEGMENT_AUTO_EXTEND_SIZE);
    if (name == nullptr || sizeStr == nullptr) {
        throw ConfigurationException(getConfigRoot(), "name or size is nullptr",
                                     lineNumber);
    }
    if ((maxSizeStr != nullptr && autoExtendSizeStr == nullptr) ||
        (autoExtendSizeStr != nullptr && maxSizeStr == nullptr)) {
        throw ConfigurationException(getConfigRoot(),
                                     "max_size or auto_extend_size is nullptr",
                                     lineNumber);
    }
    if (std::string(name).contains(' ')) {
        throw ConfigurationException(getConfigRoot(), "name contains space",
                                     lineNumber);
    }
    SegmentTemp segmentTemp{};
    segmentTemp.m_name = name;
    segmentTemp.m_size = convertToBytes(sizeStr);
    if (maxSizeStr != nullptr) {
        segmentTemp.m_maxSize = convertToBytes(maxSizeStr);
    }
    if (autoExtendSizeStr != nullptr) {
        segmentTemp.m_autoExtendSize = convertToBytes(autoExtendSizeStr);
    }
    segmentTemp.m_autoExtend = true;
    segments.emplace_back(segmentTemp);
}

void SharedMemorySegmentComponent::validateAndCreatePath(
        const std::filesystem::path &path, int32_t lineOffset) {
    if (path.empty()) {
        throw ConfigurationException(
                getConfigRoot(), "Path definition is completely missing",
                lineOffset);
    }
    if (!exists(path)) {
        create_directories(path);
    }
}

std::vector<std::shared_ptr<Entry>> SharedMemorySegmentComponent::buildEntries(
        const std::filesystem::path &path,
        const std::vector<SegmentTemp> &segments) {
    std::vector<std::shared_ptr<Entry>> entries;
    entries.push_back(std::make_shared<SharedMemorySegmentEntry>(
            type_name<SharedMemorySegmentComponent>(),
            std::make_shared<std::filesystem::path>(path)));
    for (const auto &segmentTemp: segments) {
        auto filePath = path.string() + std::filesystem::path::preferred_separator +
                        segmentTemp.m_name + ".bin";
        std::shared_ptr<SharedMemorySegment> sharedMemorySegment = nullptr;
        if (!segmentTemp.m_autoExtend) {
            sharedMemorySegment = std::make_shared<SharedMemorySegment>(
                    filePath, segmentTemp.m_name, segmentTemp.m_size);
        } else {
            sharedMemorySegment = std::make_shared<SharedMemorySegment>(
                    filePath, segmentTemp.m_name, segmentTemp.m_size,
                    segmentTemp.m_autoExtendSize, segmentTemp.m_maxSize);
        }
        entries.push_back(std::make_shared<SharedMemorySegmentEntry>(
                type_name<SharedMemorySegmentComponent>(), sharedMemorySegment));
    }
    return entries;
}

std::vector<std::shared_ptr<Entry>> SharedMemorySegmentComponent::parse(
        const std::string &content, const std::string &fileName,
        int32_t lineOffset) {
    std::vector<SegmentTemp> segments;
    std::filesystem::path path;

    tinyxml2::XMLDocument document;
    document.Parse(content.c_str());

    tinyxml2::XMLElement *rootNode =
            document.FirstChildElement(getConfigRoot().c_str());
    if (rootNode == nullptr) {
        return {};
    }
    int32_t lineNumber = lineOffset;
    for (tinyxml2::XMLElement *shmElement = rootNode->FirstChildElement();
         shmElement != nullptr; shmElement = shmElement->NextSiblingElement()) {
        lineNumber++;
        bool isPath = std::strcmp(shmElement->Name(), m_shape.PATH_ROOT) == 0;
        bool isSegment =
                std::strcmp(shmElement->Name(), m_shape.SHM_SEGMENT_ROOT) == 0;
        if (!isPath && !isSegment) {
            throw ConfigurationException(getConfigRoot(),
                                         "Root is not PATH_ROOT or SHM_SEGMENT_ROOT",
                                         lineNumber);
        }
        if (isPath) {
            processPathElement(shmElement, lineNumber, path);
        } else {
            processSegmentElement(shmElement, lineNumber, segments);
        }
    }
    validateAndCreatePath(path, lineOffset);
    return buildEntries(path, segments);
}
