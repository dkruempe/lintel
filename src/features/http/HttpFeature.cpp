#include "lintel/features/http/HttpFeature.h"

#include "Hypodermic/Container.h"
#include "Hypodermic/ContainerBuilder.h"

#include "lintel/features/http/controllers/HistoryApi.h"
#include "lintel/features/http/controllers/HistoryController.h"
#include "lintel/features/http/controllers/MessageQueueApi.h"
#include "lintel/features/http/controllers/MessageQueueController.h"
#include "lintel/features/http/controllers/ProcessApi.h"
#include "lintel/features/http/controllers/ProcessController.h"
#include "lintel/features/http/controllers/SharedMemoryApi.h"
#include "lintel/features/http/controllers/SharedMemoryController.h"
#include "lintel/features/http/controllers/UserApi.h"
#include "lintel/features/http/controllers/UserController.h"
#include "lintel/features/http/provider/ClientProvider.h"
#include "lintel/features/http/service/Controller.h"

HttpFeature::HttpFeature(std::shared_ptr<Features> features) : Feature(Features::Http, std::move(features)) {}

void HttpFeature::registerTypes(Hypodermic::ContainerBuilder &builder) {
    builder.registerType<ServerProvider>().singleInstance();
    builder.registerType<ClientProvider>().singleInstance();
    builder.registerType<ProcessApi>().singleInstance();
    builder.registerType<ProcessController>()
            .as<Controller>()
            .as<GroupProvider>()
            .asSelf()
            .singleInstance();
    builder.registerType<UserApi>().singleInstance();
    builder.registerType<UserController>()
            .as<Controller>()
            .as<GroupProvider>()
            .asSelf()
            .singleInstance();
    builder.registerType<SharedMemoryApi>().singleInstance();
    builder.registerType<SharedMemoryController>()
            .as<Controller>()
            .as<GroupProvider>()
            .asSelf()
            .singleInstance();
    builder.registerType<HistoryApi>().singleInstance();
    builder.registerType<HistoryController>()
            .as<Controller>()
            .as<GroupProvider>()
            .asSelf()
            .singleInstance();
    builder.registerType<MesssageQueueApi>().singleInstance();
    builder.registerType<MessageQueueController>()
            .as<Controller>()
            .as<GroupProvider>()
            .asSelf()
            .singleInstance();
}

void HttpFeature::initialize(std::shared_ptr<Hypodermic::Container> container) {
    m_ServerProvider = container->resolve<ServerProvider>();
}
