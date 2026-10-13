#ifndef LINTEL_SERVERPROVIDER_H
#define LINTEL_SERVERPROVIDER_H

#include "lintel/core/configuration/Configuration.h"
#include "lintel/features/http/service/Server.h"

namespace lintel {
/** Provides a shared HTTP server instance built from configuration and controllers */
class ServerProvider
{
private:
  std::shared_ptr<Server> m_server;

  /** Build a Server from configuration and controller list */
  static std::shared_ptr<Server> build(const std::shared_ptr<Configuration> &configuration,
    const std::vector<std::shared_ptr<Controller>> &controllers);

public:
  explicit ServerProvider(const std::shared_ptr<Configuration> &configuration,
    const std::vector<std::shared_ptr<Controller>> &controllers);

  /** @return the shared Server instance */
  const std::shared_ptr<Server> &provide();
};

}// namespace lintel
#endif// LINTEL_SERVERPROVIDER_H
