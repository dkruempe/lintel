#include "lintel/features/cli/components/AdminCliComponent.h"

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace lintel {
namespace {
  /** @param response the answer of the controller
   * @return the number of resources the reload reported as failed */
  std::size_t failedCountOf(const AdminTlsReloadResponse &response)
  {
    std::size_t failed = 0;
    for (const auto &result : response.results) {
      if (!result.reloaded) { ++failed; }
    }
    return failed;
  }
}// namespace

AdminCliComponent::AdminCliComponent(std::shared_ptr<AdminApi> adminApi)
  : CommandLineComponent(m_name, m_alias), m_adminApi(std::move(adminApi))
{
  m_commandParser.addCommand(Command("reload_tls", "Reload the TLS certificate of the running server"), ReloadTls);
  m_commandParser.addCommand(
    Command("log_level", "Change the log level of the running process")
      .addArgument({ "--level", "-l" }, &m_level, "Level: trace, debug, info, warn, err (or error), critical or off"),
    LogLevel);
}

const std::string &AdminCliComponent::requiredOf(const std::optional<std::string> &value, const char *flag)
{
  if (!value.has_value()) { throw std::runtime_error(std::string(flag) + " is required"); }
  return value.value();
}

void AdminCliComponent::onCommand(const UserDto & /*userDto*/,
  const std::string &command,
  const std::vector<std::string> &parameters)
{
  try {
    const Commands cmd = m_commandParser.parse(command, parameters);
    if (cmd == ReloadTls) {
      const std::optional<AdminTlsReloadResponse> response = m_adminApi->reloadTlsOf();
      if (!response.has_value()) {
        std::cout << "ERROR: TLS certificate could not be reloaded\n";
        return;
      }
      printReloadResults(response.value());
    } else if (cmd == LogLevel) {
      const std::string &level = requiredOf(m_level, "--level");
      const std::optional<AdminLogLevelResponse> response = m_adminApi->logLevelOf(level);
      if (!response.has_value()) {
        std::cout << "ERROR: log level " << level << " could not be changed\n";
        return;
      }
      if (response->status != "changed") {
        std::cout << "ERROR: log level " << (response->level.empty() ? level : response->level) << " was rejected ("
                  << response->status << ")\n";
        return;
      }
      std::cout << "Log level changed to " << (response->level.empty() ? level : response->level) << "\n";
    }
  } catch (const std::exception &exception) {
    std::cerr << "ERROR: " << exception.what() << "\n";
  }
}

void AdminCliComponent::printReloadResults(const AdminTlsReloadResponse &response)
{
  for (const auto &result : response.results) { std::cout << result.name << ": " << result.detail << "\n"; }
  if (response.status == "reloaded") {
    std::cout << "Reloaded the TLS certificate of " << response.results.size() << " resource(s)\n";
    return;
  }
  if (response.status == "nothing_to_reload") {
    // no TLS server registered a handler, so the process serves clear text or
    // is not the process that owns the server
    std::cout << "ERROR: no TLS server is registered, nothing to reload\n";
    return;
  }
  std::cout << "ERROR: TLS certificate could not be reloaded on " << failedCountOf(response) << " of "
            << response.results.size() << " resource(s)\n";
}

void AdminCliComponent::onHelp() { m_commandParser.printHelp(getName(), getAlias(), m_description); }

bool AdminCliComponent::onMenu(const std::string & /*component*/) { return false; }

void AdminCliComponent::onShowMenu() { std::cout << "No submenu available!\n"; }

bool AdminCliComponent::onExit() { return true; }

void AdminCliComponent::printCommandList(std::set<std::string> menuAlias)
{
  m_commandParser.printCommandList(menuAlias);
}

std::vector<std::string> AdminCliComponent::allCommandsOf() { return m_commandParser.allCommandsOf(); }
}// namespace lintel
