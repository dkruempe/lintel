#ifndef CPP_BASE_LIBRARY_SERVERPROVIDER_H
#define CPP_BASE_LIBRARY_SERVERPROVIDER_H

#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/http/service/Server.h"

class ServerProvider {
 private:
  std::shared_ptr<Server> m_server;

  static std::shared_ptr<Server> build(
      const std::shared_ptr<Configuration>& configuration,
      const std::vector<std::shared_ptr<Controller>>& controllers);

 public:
  explicit ServerProvider(
      const std::shared_ptr<Configuration>& configuration,
      const std::vector<std::shared_ptr<Controller>>& controllers);

  const std::shared_ptr<Server>& provide();
};

#endif  // CPP_BASE_LIBRARY_SERVERPROVIDER_H
