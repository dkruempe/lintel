#include "base_library/features/http/HttpFeature.h"

#include "base_library/features/http/ClientProvider.h"
#include "base_library/features/http/Controller.h"
#include "base_library/features/http/ExampleController.h"
#include "base_library/features/http/configuration/HttpComponent.h"

HttpFeature::HttpFeature() : Feature(type_name<HttpFeature>()) {}

void HttpFeature::registerTypes(Hypodermic::ContainerBuilder& builder) {
  builder.registerType<ServerProvider>().singleInstance();
  builder.registerType<ClientProvider>().singleInstance();
  builder.registerType<ExampleController>()
      .as<Controller>()
      .asSelf()
      .singleInstance();
  builder.registerType<HttpComponent>()
      .as<Component>()
      .asSelf()
      .singleInstance();
}

void HttpFeature::initialize(std::shared_ptr<Hypodermic::Container> container) {
  m_ServerProvider = container->resolve<ServerProvider>();
}
