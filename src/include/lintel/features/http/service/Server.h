#ifndef HTTP_LIBRARY_SERVER_H
#define HTTP_LIBRARY_SERVER_H

#include <memory>
#include <vector>

#include "lintel/features/http/configuration/ServerConfiguration.h"

class Controller;

/** HTTP server that manages controller registration and runs in a background thread */
class Server
{
private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;

public:
  /** @param serverConfiguration server settings
   *  @param controller list of controllers to register */
  Server(const ServerConfiguration &serverConfiguration, std::vector<std::shared_ptr<Controller>> controller);

  ~Server();
};

#endif// HTTP_LIBRARY_SERVER_H