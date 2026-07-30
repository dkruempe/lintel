#include "base_library/features/http/HttpFeature.h"

#include "base_library/features/http/ExampleController.h"
#include "base_library/features/http/controllers/HistoryApi.h"
#include "base_library/features/http/controllers/HistoryController.h"
#include "base_library/features/http/controllers/MessageQueueApi.h"
#include "base_library/features/http/controllers/MessageQueueController.h"
#include "base_library/features/http/controllers/ProcessApi.h"
#include "base_library/features/http/controllers/ProcessController.h"
#include "base_library/features/http/controllers/SharedMemoryApi.h"
#include "base_library/features/http/controllers/SharedMemoryController.h"
#include "base_library/features/http/controllers/UserApi.h"
#include "base_library/features/http/controllers/UserController.h"
#include "base_library/features/http/provider/ClientProvider.h"
#include "base_library/features/http/service/Controller.h"

HttpFeature::HttpFeature(std::shared_ptr<Features> features) : Feature(Features::Http, std::move(features)) {}

void HttpFeature::registerTypes(Hypodermic::ContainerBuilder &builder) {
    builder.registerType<ServerProvider>().singleInstance();
    builder.registerType<ClientProvider>().singleInstance();
    builder.registerType<ExampleController>()
            .as<Controller>()
            .as<GroupProvider>()
            .asSelf()
            .singleInstance();
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
