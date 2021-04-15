#include "base_library/features/property/PropertyFeature.h"
#include "base_library/features/property/repositories/FilePropertyRepository.h"
#include "base_library/features/property/strategies/XMLConfigSerializationStrategy.h"

PropertyFeature::PropertyFeature() : Feature(type_name<PropertyFeature>()) {}
void PropertyFeature::registerTypes(Hypodermic::ContainerBuilder &builder) {
  builder.registerType<FilePropertyRepository>()
      .as<PropertyRepository>()
      .asSelf()
      .singleInstance();
  builder.registerType<PropertyService>().singleInstance();
  builder.registerType<XMLConfigSerializationStrategy>()
      .as<ConfigSerializationStrategy>()
      .asSelf()
      .singleInstance();
}

void PropertyFeature::initialize(
    std::shared_ptr<Hypodermic::Container> container) {
  propertyService = container->resolve<PropertyService>();
}
