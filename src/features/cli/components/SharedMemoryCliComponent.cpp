#include "lintel/features/cli/components/SharedMemoryCliComponent.h"

#include <fstream>
#include <lintel/core/platform/Platform.h>
#include <lintel/core/utils/MemorySize.h>
#include <lintel/core/utils/StringUtils.h>
#include <lintel/core/utils/TableBuilder.h>
#include <rapidjson/prettywriter.h>
#include <stdexcept>

namespace lintel {
SharedMemoryCliComponent::SharedMemoryCliComponent(std::shared_ptr<SharedMemoryApi> sharedMemoryApi)
  : CommandLineComponent(m_name, m_alias), m_sharedMemoryApi(std::move(sharedMemoryApi))
{
  m_commandParser.addCommand(
    Command("show_segments", "Show all segments")
      .addArgument({ "--segment-name", "-s" }, &m_segmentName, "Segment Name or pattern matching"),
    ShowSegments);
  m_commandParser.addCommand(
    Command("show_repositories", "Show all repositories")
      .addArgument({ "--segment-name", "-s" }, &m_segmentName, "Segment Name or pattern matching")
      .addArgument({ "--repository-name", "-r" }, &m_repositoryName, "Repository Name or pattern matching"),
    ShowRepositories);
  m_commandParser.addCommand(Command("show_stats", "Show the runtime statistics of one segment")
                               .addArgument({ "--segment-name", "-s" }, &m_segmentName, "Segment Name"),
    ShowStats);
  m_commandParser.addCommand(Command("cleanup", "Remove shared memory objects whose owner process is gone"), Cleanup);
  m_commandParser.addCommand(
    Command("recover_segments", "Reclaim the remap locks of segments whose holder process is gone"), RecoverSegments);
  m_commandParser.addCommand(
    Command("shrink_segment", "Shrink segment").addArgument({ "--segment-name", "-s" }, &m_segmentName, "Segment Name"),
    ShrinkSegment);
  m_commandParser.addCommand(Command("grow_segment", "Grow segment")
                               .addArgument({ "--segment-name", "-s" }, &m_segmentName, "Segment Name")
                               .addArgument({ "--grow-size", "-g" }, &m_size, "Grow Size"),
    GrowSegment);
  m_commandParser.addCommand(
    Command("export_repository", "Export Repository to json")
      .addArgument({ "--repository-name", "-r" }, &m_repositoryName, "Repository name")
      .addArgument({ "--uuid", "-u" }, &m_uuid, "UUID")
      .addArgument({ "--file", "-f" }, &m_file, "File to write the export to (default: stdout)"),
    ExportRepository);
}

void SharedMemoryCliComponent::onCommand(const UserDto & /*userDto*/,
  const std::string &input,
  const std::vector<std::string> &parameters)
{
  try {
    Commands command = m_commandParser.parse(input, parameters);
    switch (command) {
    case ShowSegments: {
      if (!m_segmentName.has_value()) { m_segmentName = std::make_optional(".*"); }
      auto res = m_sharedMemoryApi->allSegmentsOf(m_segmentName.value());
      printSegments(res);
      m_segmentName->clear();
      break;
    }
    case ShowRepositories: {
      if (!m_segmentName.has_value()) { m_segmentName = std::make_optional(".*"); }
      if (!m_repositoryName.has_value()) { m_repositoryName = std::make_optional(".*"); }
      auto res = m_sharedMemoryApi->allRepositoriesOf(m_repositoryName.value(), m_segmentName.value());
      printRepositories(res);
      m_segmentName->clear();
      m_repositoryName->clear();
      break;
    }
    case ShowStats: {
      if (!m_segmentName.has_value()) {
        std::cout << "ERROR: please set segment name\n";
        return;
      }
      const std::optional<SharedMemorySegmentStats> stats = m_sharedMemoryApi->statsOf(m_segmentName.value());
      if (!stats.has_value()) {
        std::cout << "ERROR: no statistics for segment " << m_segmentName.value() << "\n";
        m_segmentName->clear();
        return;
      }
      printStats(stats.value());
      m_segmentName->clear();
      break;
    }
    case Cleanup: {
      const std::optional<SharedMemoryCleanupSummary> summary = m_sharedMemoryApi->cleanupOf();
      if (!summary.has_value()) {
        std::cout << "ERROR: cleanup was rejected\n";
        return;
      }
      std::cout << "Cleanup scanned " << summary->scanned << " entries: " << summary->orphans << " orphans, "
                << summary->removed << " removed, " << summary->failed << " failures\n";
      break;
    }
    case RecoverSegments: {
      const std::optional<std::size_t> recovered = m_sharedMemoryApi->recoverOf();
      if (!recovered.has_value()) {
        std::cout << "ERROR: recover was rejected\n";
        return;
      }
      std::cout << "Recovered " << recovered.value() << " orphaned remap locks\n";
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
    case GrowSegment: {
      if (!m_segmentName.has_value()) {
        std::cout << "ERROR: please set segment name\n";
        return;
      }
      if (!m_size.has_value()) {
        std::cout << "ERROR: please set grow size\n";
        return;
      }
      m_sharedMemoryApi->growOf(m_segmentName.value(), m_size.value());
      m_segmentName->clear();
      break;
    }
    case ExportRepository:
      handleExportRepository();
      break;
    default:
    case Undefined:
      break;
    }
  } catch (std::exception &exception) {
    std::cerr << "ERROR: " << exception.what() << "\n";
  }
}

void SharedMemoryCliComponent::handleExportRepository()
{
  if (!m_uuid.has_value()) {
    std::cout << "ERROR: please set uuid\n";
    return;
  }
  std::string result = m_sharedMemoryApi->repositoryOf(m_uuid.value());
  if (m_file.has_value()) {
    const std::string file = lintel::expandHomeDirectory(m_file.value());
    std::filesystem::path path(file);
    bool parentPathExits = std::filesystem::exists(path.parent_path());
    if (!parentPathExits) { std::filesystem::create_directories(path.parent_path()); }
    // codeql[cpp/path-injection] The export target is the operator's own
    // --file argument processed in the operator's own process, so it grants no
    // access that the caller does not already have. Every other component
    // (segment, repository, UUID) is resolved by the service; this output path
    // is the single deliberate user choice. There is no privileged boundary to
    // enforce here, hence no sanitizer the rule could accept.
    std::ofstream out = std::ofstream(path, std::ofstream::trunc | std::ofstream::out);
    if (!out.is_open()) { throw std::runtime_error("cannot open export file " + path.string()); }
    out << result;
    std::cout << "Exported SharedMemory to >" << path.filename() << "<\n";
  } else {
    rapidjson::Document document;
    document.Parse(result.c_str());
    rapidjson::StringBuffer stringBuffer;
    rapidjson::PrettyWriter writer(stringBuffer);
    document.Accept(writer);
    std::cout << stringBuffer.GetString() << "\n";
  }
  if (m_file.has_value()) { m_file->clear(); }
  if (m_repositoryName.has_value()) { m_repositoryName->clear(); }
  if (m_segmentName.has_value()) { m_segmentName->clear(); }
}

void SharedMemoryCliComponent::printSegments(const std::vector<SharedMemorySegmentDto> &segments)
{
  TableBuilder<11> builder;
  builder.add({ "No.",
    "SegmentName",
    "Size",
    "AutoExtend",
    "AutoExtendSize",
    "MaxSize",
    "CurrentSize",
    "FreeSize",
    "NamedObjects",
    "UniqueObjects",
    "Sanity" });
  std::size_t count = 0;
  for (const auto &iter : segments) {
    builder.add({ std::to_string(++count),
      iter.getName(),
      MemorySize::serialize(iter.getSize()),
      iter.isAutoExtend() ? "true" : "false",
      MemorySize::serialize(iter.getAutoExtendSize()),
      MemorySize::serialize(iter.getMaxSize()),
      MemorySize::serialize(iter.getCurrentSize()),
      MemorySize::serialize(iter.getFreeSize()),
      std::to_string(iter.getAmountNamedObjects()),
      std::to_string(iter.getAmountUniqueObjects()),
      iter.isSanity() ? "true" : "false" });
  }
  std::cout << builder.build() << "\n";
}

bool SharedMemoryCliComponent::onExit() { return true; }

void SharedMemoryCliComponent::printCommandList(std::set<std::string> menuAlias)
{
  m_commandParser.printCommandList(menuAlias);
}

void SharedMemoryCliComponent::onShowMenu() { std::cout << "No subMenu available\n"; }

bool SharedMemoryCliComponent::onMenu(const std::string & /*component*/) { return false; }

void SharedMemoryCliComponent::onHelp() { m_commandParser.printHelp(getName(), getAlias(), m_description); }

void SharedMemoryCliComponent::printRepositories(const std::vector<SharedMemoryRepositoryDto> &repositories)
{
  TableBuilder<7> builder;
  builder.add({ "No.", "Uuid", "SegmentName", "RepositoryName", "Size", "Type", "Version" });
  std::size_t count = 0;
  for (const auto &iter : repositories) {
    builder.add({ std::to_string(++count),
      iter.getUuid(),
      iter.getSegmentName(),
      iter.getName(),
      std::to_string(iter.getSize()),
      std::string(magic_enum::enum_name<>(iter.getType())),
      std::to_string(iter.getCurrentVersion()) });
  }
  std::cout << builder.build() << "\n";
}

void SharedMemoryCliComponent::printStats(const SharedMemorySegmentStats &stats)
{
  TableBuilder<2> builder;
  builder.add({ "Field", "Value" });
  builder.add({ "Name", stats.name });
  builder.add({ "AllocatedBytes", std::to_string(stats.allocatedBytes) });
  builder.add({ "CapacityBytes", std::to_string(stats.capacityBytes) });
  builder.add({ "FreeBytes", std::to_string(stats.freeBytes) });
  builder.add({ "GrowCount", std::to_string(stats.growCount) });
  builder.add({ "ShrinkCount", std::to_string(stats.shrinkCount) });
  builder.add({ "RecoveryCount", std::to_string(stats.recoveryCount) });
  builder.add({ "Generation", std::to_string(stats.generation) });
  std::cout << builder.build() << "\n";
}

std::vector<std::string> SharedMemoryCliComponent::allCommandsOf() { return m_commandParser.allCommandsOf(); }
}// namespace lintel
