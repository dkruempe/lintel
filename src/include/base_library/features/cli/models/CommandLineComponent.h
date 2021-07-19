#ifndef CPP_BASE_LIBRARY_COMMANDLINECOMPONENT_H
#define CPP_BASE_LIBRARY_COMMANDLINECOMPONENT_H

#include <memory>
#include <string>

#include "base_library/features/http/Client.h"

class CommandLineComponent {
 private:
  std::string_view m_name;
  std::string_view m_alias;

 protected:
  std::shared_ptr<Client> m_client;

 public:
  CommandLineComponent(std::string_view name, std::string_view alias);

  virtual ~CommandLineComponent() = default;

  std::string_view getName();

  std::string_view getAlias();

  virtual void onCommand(const std::string &input) = 0;

  virtual void onHelp() = 0;

  virtual void onShowMenu() = 0;

  virtual bool onMenu(const std::string &component) = 0;

  virtual bool onExit() = 0;

  void setClient(const std::shared_ptr<Client> &client) { m_client = client; }
};

#endif  // CPP_BASE_LIBRARY_COMMANDLINECOMPONENT_H
