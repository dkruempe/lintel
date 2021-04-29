#ifndef CPP_BASE_LIBRARY_WEBSOCKETFEATURE_H
#define CPP_BASE_LIBRARY_WEBSOCKETFEATURE_H

#include <memory>

#include "base_library/features/Feature.h"

class Server;

class WebsocketFeature : public Feature {
 private:
  std::shared_ptr<Server> m_server;

 public:
  WebsocketFeature();

  void registerTypes(Hypodermic::ContainerBuilder &builder) override;

  void initialize(std::shared_ptr<Hypodermic::Container> container) override;
};

#endif  // CPP_BASE_LIBRARY_WEBSOCKETFEATURE_H
