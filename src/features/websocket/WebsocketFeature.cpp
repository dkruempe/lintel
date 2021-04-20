#include "base_library/features/websocket/WebsocketFeature.h"
WebsocketFeature::WebsocketFeature() : Feature(type_name<WebsocketFeature>()) {}
void WebsocketFeature::registerTypes(Hypodermic::ContainerBuilder& builder) {}
void WebsocketFeature::initialize(
    std::shared_ptr<Hypodermic::Container> container) {}
