#include "base_library/features/property/PropertyFeature.h"

#include "base_library/core/services/PersistableBean.h"
#include "base_library/features/property/command_line/PropertyCliComponent.h"
#include "base_library/features/property/configuration/PropertyComponent.h"
#include "base_library/features/property/controller/PropertyApi.h"
#include "base_library/features/property/controller/PropertyController.h"
#include "base_library/features/property/repositories/DatabasePropertyRepository.h"
#include "base_library/features/property/repositories/FilePropertyRepository.h"
#include "base_library/features/property/strategies/XMLConfigSerializationStrategy.h"

PropertyFeature::PropertyFeature() : Feature(type_name<PropertyFeature>()) {}
void PropertyFeature::registerTypes(Hypodermic::ContainerBuilder &builder) {
  builder.registerType<FilePropertyRepository>()
      .as<PropertyRepository>()
      .asSelf()
      .singleInstance();
  builder.registerType<DatabasePropertyRepository>()
      .as<PropertyRepository>()
      .asSelf()
      .singleInstance();
  builder.registerType<PropertyService>()
      .as<PersistableBean>()
      .asSelf()
      .singleInstance();
  builder.registerType<XMLConfigSerializationStrategy>()
      .as<ConfigSerializationStrategy>()
      .asSelf()
      .singleInstance();
  builder.registerType<PropertyComponent>()
      .as<Component>()
      .asSelf()
      .singleInstance();
  builder.registerType<PropertyController>()
      .as<Controller>()
      .asSelf()
      .singleInstance();
  builder.registerType<PropertyApi>().singleInstance();
  builder.registerType<PropertyCliComponent>()
      .as<CommandLineComponent>()
      .asSelf()
      .singleInstance();
}

void PropertyFeature::initialize(
    std::shared_ptr<Hypodermic::Container> container) {
  m_propertyService = container->resolve<PropertyService>();
}
