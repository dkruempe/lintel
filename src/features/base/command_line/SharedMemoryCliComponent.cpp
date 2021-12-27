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
        auto res = m_sharedMemoryApi->allOf(m_segmentName.value());
        printSegments(res);
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