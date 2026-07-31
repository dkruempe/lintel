#include "base_library/features/cli/services/CommandLineHistoryService.h"
#include "base_library/config.h"
#include "base_library/core/exceptions/FileServiceIsNotFileException.h"
#include "base_library/core/services/FileService.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/core/utils/StringUtils.h"

#include "base_library/features/cli/models/CommandHistoryEntry.h"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>

CommandLineHistoryService::CommandLineHistoryService(const std::shared_ptr<ProcessName> &processName)
  : PropertyRegistration(processName->getProcessName()), m_position(std::nullopt)
{
  m_maxCommandHistoryEntries = registerProperty<std::size_t>(
          "m_maxCommandHistoryEntries", static_cast<std::size_t>(1000),
          "Maximal count of history command entries per local save file", true,
          __FILE__, __LINE__);
  m_historyFileName = registerProperty<std::string>(
          "m_historyFileName", std::string(".history"),
          "Filename of local history file", false,
          __FILE__, __LINE__);
}

std::optional<CommandHistoryEntry> CommandLineHistoryService::nextOf(std::string menu)
{
  if (m_history.empty()) {
    return std::nullopt;
  }
  if (m_menu != menu) {
    m_position = std::nullopt;
  }
  if (!m_position.has_value()) {
    m_position = std::make_optional(0);
  }
  if (m_position.value() >= m_history.size()) {
    m_position = std::make_optional(m_history.size() + 1);
    return std::nullopt;
  }
  m_position = std::make_optional(m_position.value() + 1);
  for (std::size_t i = m_position.value(); i <= m_history.size(); i++) {
    auto iter = m_history[i - 1];
    if (iter.getMenu() != menu) {
      continue;
    }
    m_position = std::make_optional(i);
    m_menu = menu;
    return std::make_optional(iter);
  }
  return std::nullopt;
}

std::optional<CommandHistoryEntry> CommandLineHistoryService::previousOf(std::string menu)
{
  if (m_history.empty()) {
    return std::nullopt;
  }
  if (m_menu != menu) {
    m_position = std::nullopt;
  }
  if (!m_position.has_value()) {
    m_position = m_history.size() + 1;
  }
  if (m_position.value() == 0) {
    return std::nullopt;
  }
  if (m_position.value() > m_history.size() + 1) {
    m_position = m_history.size() + 1;
  }
  m_position = std::make_optional(m_position.value() - 1);
  for (std::size_t i = m_position.value(); i > 0; i--) {
    auto iter = m_history[i - 1];
    if (iter.getMenu() != menu) {
      continue;
    }
    m_position = std::make_optional(i);
    m_menu = menu;
    return std::make_optional(iter);
  }
  return std::nullopt;
}

std::optional<CommandHistoryEntry> CommandLineHistoryService::startsWith(std::string command, std::string menu)
{
  if (m_history.empty()) { return std::nullopt; }
  for (std::size_t i = m_history.size(); i > 0; i--) {
    CommandHistoryEntry iter = m_history[i - 1];
    if (iter.getMenu() != menu) {
      continue;
    }
    if (StringUtils::startsWith(iter.getCommand(), command)) { return std::make_optional(iter); }
  }
  return std::nullopt;
}


void CommandLineHistoryService::historizeOf(CommandHistoryEntry commandHistoryEntry)
{
  std::filesystem::path path(
    std::string(CONFIG_DIRECTORY) + std::filesystem::path::preferred_separator + m_historyFileName->getValue());
  if (m_maxCommandHistoryEntries->getValue() > m_history.size()) {
    std::ofstream file(path.string(), std::ios_base::app);
    if (!file.is_open()) { return; }
    file << commandHistoryEntry.asCsvEntry() << "\n";
    file.flush();
    m_history.push_back(commandHistoryEntry);
    m_position = std::make_optional(m_history.size() + 1);
    return;
  }
  m_history.erase(m_history.begin());
  m_history.push_back(commandHistoryEntry);
  std::ofstream file(path.string());
  if (!file.is_open()) { return; }
  for (const auto &history : m_history) { file << history.asCsvEntry() << "\n"; }
  file.flush();
  m_position = std::make_optional(m_history.size() + 1);
}


std::vector<CommandHistoryEntry> CommandLineHistoryService::internalAllOf()
{
  std::filesystem::path path(
    std::string(CONFIG_DIRECTORY) + std::filesystem::path::preferred_separator + m_historyFileName->getValue());
  FileService fileService(path);
  try {
    FileService::Stream stream = fileService.createStream();
    std::vector<CommandHistoryEntry> commandHistoryEntries;
    while (!stream.isEndOfFile()) {
      std::string line = stream.getLine();
      std::vector<std::string> entries = StringUtils::split(line, ';');
      auto timestamp = StringifyService<date::sys_time<std::chrono::microseconds>>::deserializeFromString(entries[0]);
      std::string userName = entries[1];
      std::string command = entries[2];
      std::string menu = entries[3];
      commandHistoryEntries.emplace_back(command, userName, timestamp, menu);
    }
    return commandHistoryEntries;
  } catch (const FileServiceIsNotFileException &exception) {
    // file not found
    return {};
  }
}

void CommandLineHistoryService::onInitialize()
{
  m_history = internalAllOf();
}


std::vector<CommandHistoryEntry> CommandLineHistoryService::allOf() { return m_history; }
