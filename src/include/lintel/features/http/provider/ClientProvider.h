#ifndef LINTEL_CLIENTPROVIDER_H
#define LINTEL_CLIENTPROVIDER_H

#include "lintel/features/base/configuration/Configuration.h"
#include "lintel/features/http/service/Client.h"

namespace lintel {
/** Provides a shared HTTP client instance built from configuration */
class ClientProvider
{
private:
  std::shared_ptr<Client> m_client;

  /** Build a Client from the given configuration */
  static std::shared_ptr<Client> build(const std::shared_ptr<Configuration> &configuration);

public:
  explicit ClientProvider(const std::shared_ptr<Configuration> &configuration);

  /** @return the shared Client instance */
  const std::shared_ptr<Client> &provide();
};

}// namespace lintel
#endif// LINTEL_CLIENTPROVIDER_H
