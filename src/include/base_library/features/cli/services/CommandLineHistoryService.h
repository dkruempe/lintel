#ifndef COMMANDLINEHISTORYSERVICE_H
#define COMMANDLINEHISTORYSERVICE_H
#include "base_library/core/services/AbstractService.h"
#include "base_library/core/services/FileService.h"
#include "base_library/features/base/services/AuthService.h"
#include "base_library/features/cli/models/CommandHistoryEntry.h"
#include <optional>

/**
 * Service is responsible for the serialization of the entered commands and saving
 * for each user those entered entries. The entered cli command will be saved locally
 * in a bash like text file.
 *
 * data structure:
 * - file will be saved as CSV (TS, USER_NAME, Command)
 */
class CommandLineHistoryService : public AbstractService<CommandLineHistoryService>
{
private:
  DEFINE_PROPERTY(m_maxCommandHistoryEntries,
    std::size_t,
    1000,
    "Maximal count of history command entries per local save file",
    true);
  DEFINE_PROPERTY(m_historyFileName, std::string, ".history", "Filename of local history file", false);

  // runtime parameters
  std::vector<CommandHistoryEntry> m_history;
  std::optional<std::size_t> m_position;
  std::string m_menu;

  std::vector<CommandHistoryEntry> internalAllOf();

public:
  CommandLineHistoryService(const std::shared_ptr<ProcessName> &processName);
  /**
   * returns all available historized commands
   */
  std::vector<CommandHistoryEntry> allOf();

  /**
   * @return next of history element
   */
  std::optional<CommandHistoryEntry> nextOf(std::string menu);

  /**
   * @return previous of history element
   */
  std::optional<CommandHistoryEntry> previousOf(std::string menu);

  /**
   * searches first match backwards (newest first)
   * @param command first chars
   * @param menu
   * @return returns match if found
   */
  std::optional<CommandHistoryEntry> startsWith(std::string command, std::string menu);

  /**
   * historizes the last entered command in the local file
   * @param commandHistoryEntry history entry
   */
  void historizeOf(CommandHistoryEntry commandHistoryEntry);

  void onInitialize() override;
};
#endif// COMMANDLINEHISTORYSERVICE_H
