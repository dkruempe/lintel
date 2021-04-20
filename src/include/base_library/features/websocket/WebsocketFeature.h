#ifndef CPP_BASE_LIBRARY_WEBSOCKETFEATURE_H
#define CPP_BASE_LIBRARY_WEBSOCKETFEATURE_H

#include "base_library/features/Feature.h"

class WebsocketFeature : public Feature {
 public:
  WebsocketFeature();

  void registerTypes(Hypodermic::ContainerBuilder &builder) override;

  void initialize(std::shared_ptr<Hypodermic::Container> container) override;
};

#endif  // CPP_BASE_LIBRARY_WEBSOCKETFEATURE_H
