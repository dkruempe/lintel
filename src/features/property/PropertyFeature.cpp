#include "lintel/features/property/PropertyFeature.h"

#include "Hypodermic/Container.h"
#include "Hypodermic/ContainerBuilder.h"

#include "lintel/core/services/PersistableBean.h"
#include "lintel/features/property/command_line/PropertyCliComponent.h"
#include "lintel/features/property/controller/PropertyApi.h"
#include "lintel/features/property/controller/PropertyController.h"
#include "lintel/features/property/repositories/DatabasePropertyRepository.h"
#include "lintel/features/property/repositories/FilePropertyRepository.h"
#include "lintel/features/property/repositories/SharedMemoryPropertyRepository.h"
#include "lintel/features/property/strategies/XMLConfigSerializationStrategy.h"

PropertyFeature::PropertyFeature(std::shared_ptr<Features> features) : Feature(Features::Property,
                                                                               std::move(features)) {}

void PropertyFeature::registerTypes(Hypodermic::ContainerBuilder &builder) {
    builder.registerType<FilePropertyRepository>()
            .as<PropertyRepository>()
            .asSelf()
            .singleInstance();
    builder.registerType<DatabasePropertyRepository>()
            .as<PropertyRepository>()
            .asSelf()
            .singleInstance();
    builder.registerType<SharedMemoryPropertyRepository>()
            .as<PropertyRepository>()
            .as<SharedMemoryRepository>()
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
    builder.registerType<PropertyController>()
            .as<Controller>()
            .as<GroupProvider>()
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
