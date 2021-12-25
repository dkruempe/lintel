#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYCLICOMPONENT_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYCLICOMPONENT_H

#include "base_library/features/base/controller/SharedMemoryApi.h"
#include "base_library/features/cli/models/CommandLineComponent.h"
#include "base_library/features/cli/models/CommandParser.h"

class SharedMemoryCliComponent : public CommandLineComponent {
 private:
  static constexpr std::string_view m_name = "SharedMemory";
  static constexpr std::string_view m_alias = "Shm";
  static constexpr std::string_view m_description =
      "The component can be used to manage the Shared Memory Segments and "
      "Repositories";
  std::shared_ptr<SharedMemoryApi> m_sharedMemoryApi;

  // Commands
  enum Commands { Undefined, ShowSegments };

  // Flags

  CommandParser<Commands, Undefined> m_commandParser;
  static void printSegments(
      const std::vector<SharedMemorySegmentDto> &segments);

 public:
  explicit SharedMemoryCliComponent(
      std::shared_ptr<SharedMemoryApi> sharedMemoryApi);
  void onCommand(const UserDto &userDto, const std::string &input,
                 const std::vector<std::string> &parameters) override;

  void onHelp() override;

  void onShowMenu() override;

  bool onMenu(const std::string &component) override;

  bool onExit() override;

  void printCommandList() override;
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYCLICOMPONENT_H
