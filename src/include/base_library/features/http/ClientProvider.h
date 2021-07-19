#ifndef CPP_BASE_LIBRARY_CLIENTPROVIDER_H
#define CPP_BASE_LIBRARY_CLIENTPROVIDER_H

#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/http/Client.h"

class ClientProvider {
 private:
  std::shared_ptr<Client> m_client;

  static std::shared_ptr<Client> build(
      const std::shared_ptr<Configuration>& configuration);

 public:
  explicit ClientProvider(const std::shared_ptr<Configuration>& configuration);

  const std::shared_ptr<Client>& provide();
};

#endif  // CPP_BASE_LIBRARY_CLIENTPROVIDER_H
