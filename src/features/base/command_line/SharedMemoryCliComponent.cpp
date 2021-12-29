#include "base_library/features/base/command_line/SharedMemoryCliComponent.h"

#include <base_library/core/utils/MemorySize.h>

#include <tabulate/table.hpp>
SharedMemoryCliComponent::SharedMemoryCliComponent(
    std::shared_ptr<SharedMemoryApi> sharedMemoryApi)
    : CommandLineComponent(m_name, m_alias),
      m_sharedMemoryApi(std::move(sharedMemoryApi)) {
  m_commandParser.addCommand(
      Command("show_segments", "Show all segments")
          .addArgument({"--segment-name", "-s"}, &m_segmentName,
                       "Segment Name or pattern matching"),
      ShowSegments);
  m_commandParser.addCommand(
      Command("show_repositories", "Show all repositories")
          .addArgument({"--segment-name", "-s"}, &m_segmentName,
                       "Segment Name or pattern matching")
          .addArgument({"--repository-name", "-r"}, &m_repositoryName,
                       "Repository Name or pattern matching"),
      ShowRepositories);
  m_commandParser.addCommand(Command("shrink_segment", "Shrink segment")
                                 .addArgument({"--segment-name", "-s"},
                                              &m_segmentName, "Segment Name"),
                             ShrinkSegment);
}
void SharedMemoryCliComponent::onCommand(
    const UserDto & /*userDto*/, const std::string &input,
    const std::vector<std::string> &parameters) {
  try {
    Commands command = m_commandParser.parse(input, parameters);
    switch (command) {
      case ShowSegments: {
        if (!m_segmentName.has_value()) {
          m_segmentName = std::make_optional(".*");
        }
        auto res = m_sharedMemoryApi->allSegmentsOf(m_segmentName.value());
        printSegments(res);
        m_segmentName->clear();
        break;
      }
      case ShowRepositories: {
        if (!m_segmentName.has_value()) {
          m_segmentName = std::make_optional(".*");
        }
        if (!m_repositoryName.has_value()) {
          m_repositoryName = std::make_optional(".*");
        }
        auto res = m_sharedMemoryApi->allRepositoriesOf(
            m_repositoryName.value(), m_segmentName.value());
        printRepositories(res);
        m_segmentName->clear();
        m_repositoryName->clear();
        break;
      }
      case ShrinkSegment: {
        if (!m_segmentName.has_value()) {
          std::cout << "ERROR: please set segment name\n";
          return;
        }
        m_sharedMemoryApi->shrinkOf(m_segmentName.value());
        m_segmentName->clear();
        break;
      }
      default:
        break;
    }
  } catch (std::exception &exception) {
    std::cerr << "ERROR: " << exception.what() << "\n";
  }
}
void SharedMemoryCliComponent::printSegments(
    const std::vector<SharedMemorySegmentDto> &segments) {
  tabulate::Table table;
  table.add_row({"No.", "SegmentName", "Size", "AutoExtend", "AutoExtendSize",
                 "MaxSize", "CurrentSize", "FreeSize", "NamedObjects",
                 "UniqueObjects", "Sanity"});
  std::size_t count = 0;
  for (const auto &iter : segments) {
    table.add_row({std::to_string(++count), iter.getName(),
                   MemorySize::serialize(iter.getSize()),
                   iter.isAutoExtend() ? "true" : "false",
                   MemorySize::serialize(iter.getAutoExtendSize()),
                   MemorySize::serialize(iter.getMaxSize()),
                   MemorySize::serialize(iter.getCurrentSize()),
                   MemorySize::serialize(iter.getFreeSize()),
                   std::to_string(iter.getAmountNamedObjects()),
                   std::to_string(iter.getAmountUniqueObjects()),
                   iter.isSanity() ? "true" : "false"});
  }
  std::cout << table.str() << "\n";
}
bool SharedMemoryCliComponent::onExit() { return true; }
void SharedMemoryCliComponent::printCommandList() {
  m_commandParser.printCommandList();
}
void SharedMemoryCliComponent::onShowMenu() {
  std::cout << "No subMenu available\n";
}
bool SharedMemoryCliComponent::onMenu(const std::string & /*component*/) {
  return true;
}
void SharedMemoryCliComponent::onHelp() {
  m_commandParser.printHelp(getName(), getAlias(), m_description);
}
void SharedMemoryCliComponent::printRepositories(
    const std::vector<SharedMemoryRepositoryDto> &repositories) {
  tabulate::Table table;
  table.add_row(
      {"No.", "SegmentName", "RepositoryName", "Size", "Type", "Version"});
  std::size_t count = 0;
  for (const auto &iter : repositories) {
    table.add_row({std::to_string(++count), iter.getSegmentName(),
                   iter.getName(), std::to_string(iter.getSize()),
                   std::string(magic_enum::enum_name<>(iter.getType())),
                   std::to_string(iter.getCurrentVersion())});
  }
  std::cout << table.str() << "\n";
}
