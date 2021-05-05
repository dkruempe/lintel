#include "base_library/features/websocket/WebsocketFeature.h"

#include "base_library/features/websocket/configuration/WebsocketComponent.h"
#include "base_library/features/websocket/controller/EchoController.h"
#include "base_library/features/websocket/services/Controller.h"
#include "base_library/features/websocket/services/Server.h"

WebsocketFeature::WebsocketFeature() : Feature(type_name<WebsocketFeature>()) {}
void WebsocketFeature::registerTypes(Hypodermic::ContainerBuilder& builder) {
  builder.registerType<WebsocketComponent>()
      .as<Component>()
      .asSelf()
      .singleInstance();
  builder.registerType<Server>().singleInstance();
  builder.registerType<EchoController>()
      .as<Controller>()
      .asSelf()
      .singleInstance();
}
void WebsocketFeature::initialize(
    std::shared_ptr<Hypodermic::Container> container) {
  m_server = container->resolve<Server>();
}
