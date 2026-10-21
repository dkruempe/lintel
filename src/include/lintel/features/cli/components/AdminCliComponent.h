#ifndef LINTEL_ADMINCLICOMPONENT_H
#define LINTEL_ADMINCLICOMPONENT_H

#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "lintel/features/cli/models/CommandLineComponent.h"
#include "lintel/features/cli/models/CommandParser.h"
#include "lintel/features/http/controllers/AdminApi.h"

namespace lintel {
/** CLI component that drives the administrative endpoints of the running server */
class AdminCliComponent : public CommandLineComponent
{
private:
  static constexpr std::string_view m_name = "Admin";
  static constexpr std::string_view m_alias = "Adm";
  static constexpr std::string_view m_description =
    "The component can be used to reload the TLS certificate and to change the log level";

  std::shared_ptr<AdminApi> m_adminApi;

  enum Commands : std::uint8_t { Undefined, ReloadTls, LogLevel };

  // Flags
  std::optional<std::string> m_level;

  CommandParser<Commands, Undefined> m_commandParser;

  /** @return the value of the required flag or throw when it is missing
   * @param value the flag value
   * @param flag the flag name used in the error message */
  static const std::string &requiredOf(const std::optional<std::string> &value, const char *flag);

  /** Print the per-resource result of a reload and the overall outcome
   * @param response the answer of the controller */
  static void printReloadResults(const AdminTlsReloadResponse &response);

public:
  /** @param adminApi API for the administrative endpoints */
  explicit AdminCliComponent(std::shared_ptr<AdminApi> adminApi);

  void onHelp() override;

  void
    onCommand(const UserDto &userDto, const std::string &command, const std::vector<std::string> &parameters) override;

  void onShowMenu() override;

  bool onMenu(const std::string &component) override;

  bool onExit() override;

  void printCommandList(std::set<std::string> menuAlias) override;

  std::vector<std::string> allCommandsOf() override;
};

}// namespace lintel
#endif// LINTEL_ADMINCLICOMPONENT_H
