#ifndef LINTEL_SHAREDMEMORYCLICOMPONENT_H
#define LINTEL_SHAREDMEMORYCLICOMPONENT_H

#include "lintel/features/cli/models/CommandLineComponent.h"
#include "lintel/features/cli/models/CommandParser.h"
#include "lintel/features/http/controllers/SharedMemoryApi.h"

namespace lintel {
/** CLI component for managing shared memory segments and repositories */
class SharedMemoryCliComponent : public CommandLineComponent
{
private:
  static constexpr std::string_view m_name = "SharedMemory";
  static constexpr std::string_view m_alias = "Shm";
  static constexpr std::string_view m_description =
    "The component can be used to manage the Shared Memory Segments and "
    "Repositories";
  std::shared_ptr<SharedMemoryApi> m_sharedMemoryApi;

  // Commands
  enum Commands {
    Undefined,
    ShowSegments,
    ShowRepositories,
    ShowStats,
    Cleanup,
    ShrinkSegment,
    GrowSegment,
    ExportRepository
  };

  // Flags
  std::optional<std::string> m_segmentName;
  std::optional<std::string> m_size;
  std::optional<std::string> m_repositoryName;
  std::optional<std::string> m_file;
  std::optional<std::string> m_uuid;

  CommandParser<Commands, Undefined> m_commandParser;

  /** Print formatted segment DTOs to console */
  static void printSegments(const std::vector<SharedMemorySegmentDto> &segments);

  /** Print formatted repository DTOs to console */
  static void printRepositories(const std::vector<SharedMemoryRepositoryDto> &repositories);

  /** Print the statistics of one segment to console */
  static void printStats(const SharedMemorySegmentStats &stats);

  /** Handle the export repository command */
  void handleExportRepository();

public:
  /** @param sharedMemoryApi API for shared memory operations */
  explicit SharedMemoryCliComponent(std::shared_ptr<SharedMemoryApi> sharedMemoryApi);

  void onCommand(const UserDto &userDto, const std::string &input, const std::vector<std::string> &parameters) override;

  void onHelp() override;

  void onShowMenu() override;

  bool onMenu(const std::string &component) override;

  bool onExit() override;

  void printCommandList(std::set<std::string> menuAlias) override;

  std::vector<std::string> allCommandsOf() override;
};

}// namespace lintel
#endif// LINTEL_SHAREDMEMORYCLICOMPONENT_H
