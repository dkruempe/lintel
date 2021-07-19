#ifndef CPP_BASE_LIBRARY_HTTPENTRY_H
#define CPP_BASE_LIBRARY_HTTPENTRY_H

#include <memory>
#include <ostream>

#include "base_library/features/base/configuration/Entry.h"
#include "base_library/features/http/ClientConfiguration.h"
#include "base_library/features/http/ServerConfiguration.h"

class HttpEntry : public Entry {
 private:
  std::shared_ptr<ServerConfiguration> m_serverConfiguration;
  std::shared_ptr<ClientConfiguration> m_clientConfiguration;

 public:
  HttpEntry(std::string_view component,
            std::shared_ptr<ServerConfiguration> serverConfiguration);
  HttpEntry(std::string_view component,
            std::shared_ptr<ClientConfiguration> clientConfiguration);

  [[nodiscard]] const std::shared_ptr<ServerConfiguration> &getServerConfiguration() const;
  [[nodiscard]] const std::shared_ptr<ClientConfiguration> &getClientConfiguration() const;
  bool isServer();
  bool isClient();

  friend std::ostream &operator<<(std::ostream &os, const HttpEntry &entry);
};

#endif  // CPP_BASE_LIBRARY_HTTPENTRY_H
