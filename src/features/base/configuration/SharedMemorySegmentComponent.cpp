#include "base_library/features/base/configuration/SharedMemorySegmentComponent.h"

#include <base_library/core/utils/TypeName.h>
#include <base_library/features/base/configuration/ConfigurationException.h>
#include <base_library/features/base/configuration/EnvironmentConfiguration.h>
#include <base_library/features/base/configuration/SharedMemorySegmentEntry.h>
#include <tinyxml2.h>

#include <filesystem>

SharedMemorySegmentComponent::Shapes SharedMemorySegmentComponent::shape{};

SharedMemorySegmentComponent::SharedMemorySegmentComponent(
    std::shared_ptr<EnvironmentConfiguration> environmentConfiguration)
    : Component(shape.CONFIG_ROOT),
      m_environmentConfiguration(std::move(environmentConfiguration)) {}
std::vector<std::shared_ptr<Entry>> SharedMemorySegmentComponent::parse(
    const std::string& content, const std::string& fileName,
    int32_t lineOffset) {
  // cache for later construction of SharedMemorySegmentEntry
  struct SegmentTemp {
    bool m_autoExtend = false;
    std::string m_name;
    std::size_t m_size;
    std::size_t m_maxSize;
    std::size_t m_autoExtendSize;
  };
  std::vector<SegmentTemp> segments;
  std::filesystem::path path;

  tinyxml2::XMLDocument document;
  document.Parse(content.c_str());

  tinyxml2::XMLElement* rootNode =
      document.FirstChildElement(getConfigRoot().c_str());
  if (rootNode == nullptr) {
    return {};
  }
  int32_t lineNumber = lineOffset;
  for (tinyxml2::XMLElement* shmElement = rootNode->FirstChildElement();
       shmElement != nullptr; shmElement = shmElement->NextSiblingElement()) {
    lineNumber++;
    bool isPath = std::strcmp(shmElement->Name(), shape.PATH_ROOT.c_str()) == 0;
    bool isSegment =
        std::strcmp(shmElement->Name(), shape.SHM_SEGMENT_ROOT.c_str()) == 0;
    if (!isPath && !isSegment) {
      throw ConfigurationException(getConfigRoot(),
                                   "Root is not PATH_ROOT or SHM_SEGMENT_ROOT",
                                   lineNumber);
    }
    const char* name = shmElement->Attribute(shape.SHM_SEGMENT_NAME.c_str());
    const char* sizeStr = shmElement->Attribute(shape.SHM_SEGMENT_SIZE.c_str());
    const char* pathStr = shmElement->Attribute(shape.PATH_PATH.c_str());
    const char* maxSizeStr =
        shmElement->Attribute(shape.SHM_SEGMENT_MAX_SIZE.c_str());
    const char* autoExtendSizeStr =
        shmElement->Attribute(shape.SHM_SEGMENT_AUTO_EXTEND_SIZE.c_str());
    if (isPath) {
      if (pathStr == nullptr) {
        throw ConfigurationException(getConfigRoot(), "path value is nullptr",
                                     lineNumber);
      }
      std::string tmpPath = pathStr;
      // is home directory ? => replace with home extension
      if (tmpPath[0] == '~') {
        // skip first char bc. of home variable
        std::string rest(tmpPath.begin() + 1, tmpPath.end());
        tmpPath =
            m_environmentConfiguration->of(EnvironmentConfiguration::Home);
        tmpPath += '/';
        tmpPath += rest;
      }
      path = std::filesystem::path(tmpPath);
      if (path.empty() || (!is_directory(path) && exists(path))) {
        throw ConfigurationException(getConfigRoot(), "wrong configured path",
                                     lineNumber);
      }
    } else {
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
      if (std::string(name).find(' ') != std::string::npos) {
        throw ConfigurationException(getConfigRoot(), "name contains space",
                                     lineNumber);
      }
      SegmentTemp segmentTemp;
      segmentTemp.m_name = name;
      segmentTemp.m_size = convertToBytes(sizeStr);
      if (maxSizeStr != nullptr) {
        segmentTemp.m_maxSize = convertToBytes(maxSizeStr);
        segmentTemp.m_autoExtendSize = convertToBytes(autoExtendSizeStr);
        segmentTemp.m_autoExtend = true;
      }
      segments.emplace_back(segmentTemp);
    }
  }
  if (path.empty()) {
    throw ConfigurationException(
        getConfigRoot(), "Path definition is completely missing", lineOffset);
  }
  if (!exists(path)) {
    create_directories(path);
  }
  std::vector<std::shared_ptr<Entry>> entries;
  entries.push_back(std::make_shared<SharedMemorySegmentEntry>(
      type_name<SharedMemorySegmentComponent>(),
      std::make_shared<std::filesystem::path>(path)));
  for (const auto& segmentTemp : segments) {
    std::shared_ptr<SharedMemorySegment> sharedMemorySegment = nullptr;
    if (!segmentTemp.m_autoExtend) {
      sharedMemorySegment = std::make_shared<SharedMemorySegment>(
          path.string() + std::filesystem::path::preferred_separator +
              segmentTemp.m_name + ".bin",
          segmentTemp.m_name, segmentTemp.m_size);
    } else {
      sharedMemorySegment = std::make_shared<SharedMemorySegment>(
          path.string() + std::filesystem::path::preferred_separator +
              segmentTemp.m_name + ".bin",
          segmentTemp.m_name, segmentTemp.m_size, segmentTemp.m_autoExtendSize,
          segmentTemp.m_maxSize);
    }
    entries.push_back(std::make_shared<SharedMemorySegmentEntry>(
        type_name<SharedMemorySegmentComponent>(), sharedMemorySegment));
  }
  return entries;
}
