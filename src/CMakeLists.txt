cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0025 NEW)
if(NOT DEFINED LINTEL_VERSION)
    set(LINTEL_VERSION "0.1.0")
endif()
project(lintel VERSION ${LINTEL_VERSION})
set(CMAKE_MACOSX_RPATH OFF)
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

# configuration of default path
if (NOT DEFINED PROJECT_PATH)
    set(PROJECT_PATH "${PROJECT_SOURCE_DIR}")
endif ()
if (NOT DEFINED CONFIG_DIRECTORY)
    set(CONFIG_DIRECTORY "${PROJECT_SOURCE_DIR}/../cfg")
endif ()

if (NOT DEFINED BOOTSTRAP_CONFIG_NAME)
    set(BOOTSTRAP_CONFIG_NAME "bootstrap")
endif ()

IF (NOT DEFINED MSG_QUEUE_NAME_SIZE)
    set(MSG_QUEUE_NAME_SIZE 100)
endif ()
IF (NOT DEFINED MSG_QUEUE_CONTENT_SIZE)
    set(MSG_QUEUE_CONTENT_SIZE 1000)
endif ()

set(VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(VERSION_PATCH ${PROJECT_VERSION_PATCH})

configure_file(
        ${PROJECT_SOURCE_DIR}/include/lintel/config.h.in
        ${PROJECT_SOURCE_DIR}/include/lintel/config.h)

# dependencies
find_package(tinyxml2 REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(magic_enum REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(httplib REQUIRED)
find_package(OpenSSL REQUIRED)
# Boost.Process v1 (boost/process/v1/*) was introduced in Boost 1.86 and is used
# by ProcessService; the CMake floor therefore has to match, otherwise a system
# Boost < 1.86 (Homebrew/vcpkg on macOS, default on some Windows images) is
# accepted here and fails at compile time instead.
find_package(Boost 1.86.0 COMPONENTS system filesystem REQUIRED)
find_package(RapidJSON REQUIRED)
find_package(spdlog REQUIRED)
find_package(Threads REQUIRED)

# build
set(headers
    include/lintel/config.h
    # core - platform
    include/lintel/core/platform/Platform.h
    # core - dependency injection
    include/lintel/core/di/Container.h
    # core - exceptions
    include/lintel/core/exceptions/ConfigShmSegmentNotFound.h
    include/lintel/core/exceptions/FileServiceFileExists.h
    include/lintel/core/exceptions/FileServiceIsNotFileException.h
    include/lintel/core/exceptions/LoggerServiceNotInitialized.h
    include/lintel/core/exceptions/PoolNotFoundException.h
    include/lintel/core/exceptions/PoolTimeoutException.h
    include/lintel/core/exceptions/ShmSegmentNotFound.h
    include/lintel/core/exceptions/SQLException.h
    # core - models
    include/lintel/core/models/BootstrapSequence.h
    include/lintel/core/models/JsonSerializable.h
    include/lintel/core/models/SharedMemorySegment.h
    include/lintel/core/models/ShutdownPriority.h
    # core - persistence
    include/lintel/core/persistence/Arguments.h
    include/lintel/core/persistence/Connection.h
    include/lintel/core/persistence/ConnectionPool.h
    include/lintel/core/persistence/ConnectionPoolRegistry.h
    include/lintel/core/persistence/ScopedConnection.h
    include/lintel/core/persistence/ConnectionType.h
    include/lintel/core/persistence/Cursor.h
    include/lintel/core/persistence/DatabaseConnectionConfigurations.h
    include/lintel/core/persistence/Identifier.h
    include/lintel/core/persistence/Notify.h
    include/lintel/core/persistence/Parameter.h
    include/lintel/core/persistence/ParameterBuilder.h
    include/lintel/core/persistence/PreparedStatement.h
    include/lintel/core/persistence/Result.h
    include/lintel/core/persistence/Serialization.h
    include/lintel/core/persistence/Statement.h
    include/lintel/core/persistence/Transaction.h
    # core - persistence - postgresql
    include/lintel/core/persistence/postgresql/Connection.h
    include/lintel/core/persistence/postgresql/Cursor.h
    include/lintel/core/persistence/postgresql/Notify.h
    include/lintel/core/persistence/postgresql/PreparedStatement.h
    include/lintel/core/persistence/postgresql/Result.h
    include/lintel/core/persistence/postgresql/Serialization.h
    include/lintel/core/persistence/postgresql/Statement.h
    include/lintel/core/persistence/postgresql/Transaction.h
    # core - persistence - sqlite3
    include/lintel/core/persistence/sqlite3/Connection.h
    include/lintel/core/persistence/sqlite3/Cursor.h
    include/lintel/core/persistence/sqlite3/Notify.h
    include/lintel/core/persistence/sqlite3/PreparedStatement.h
    include/lintel/core/persistence/sqlite3/Result.h
    include/lintel/core/persistence/sqlite3/Serialization.h
    include/lintel/core/persistence/sqlite3/Statement.h
    include/lintel/core/persistence/sqlite3/Transaction.h
    # core - plugins
    include/lintel/app/plugins/AdminUserBootstrapPlugin.h
    include/lintel/core/plugins/BootstrapPlugin.h
    include/lintel/app/plugins/DatabaseBootstrapPlugin.h
    include/lintel/app/plugins/MessageQueueBootstrapPlugin.h
    include/lintel/app/plugins/SharedMemoryBootstrapPlugin.h
    include/lintel/app/plugins/SingleInstanceBootstrapPlugin.h
    include/lintel/app/plugins/VirtualGroupBootstrapPlugin.h
    # core - services
    include/lintel/core/services/AbstractService.h
    include/lintel/core/services/BootstrapService.h
    include/lintel/core/services/DirectoryService.h
    include/lintel/core/services/FileService.h
    include/lintel/core/services/LoggerService.h
    include/lintel/core/services/PersistableBean.h
    include/lintel/core/services/PersistableService.h
    include/lintel/core/services/ReadinessRegistry.h
    include/lintel/features/base/services/ProcessService.h
    include/lintel/core/services/ISharedMemoryService.h
    include/lintel/core/services/PropertyRegistration.h
    include/lintel/features/base/services/SharedMemoryService.h
    include/lintel/features/base/services/ShmConstructors.h
    include/lintel/core/services/SignalService.h
    include/lintel/core/services/StopWatchService.h
    include/lintel/core/services/StringifyService.h
    include/lintel/app/StartupBuilder.h
    # core - utils
    include/lintel/core/utils/Cryption.h
    include/lintel/core/utils/DateTimeFormat.h
    include/lintel/core/utils/JsonUtils.h
    include/lintel/core/utils/MemorySize.h
    include/lintel/core/utils/ProcessResourceReader.h
    include/lintel/core/utils/RegexUtils.h
    include/lintel/core/utils/StringUtils.h
    include/lintel/core/utils/TableBuilder.h
    include/lintel/core/utils/TypeName.h
    include/lintel/core/utils/UUID.h
    include/lintel/core/utils/Argument.h
    # features - base
    include/lintel/app/BaseFeature.h
    # features - base - configuration
    include/lintel/core/configuration/Component.h
    include/lintel/core/configuration/Configuration.h
    include/lintel/app/ConfigurationComponentBuilder.h
    include/lintel/core/configuration/ConfigurationException.h
    include/lintel/core/configuration/DatabaseConnectionComponent.h
    include/lintel/core/configuration/DatabaseConnectionEntry.h
    include/lintel/core/configuration/Entry.h
    include/lintel/core/configuration/EnvironmentConfiguration.h
    include/lintel/core/configuration/LoggerComponent.h
    include/lintel/core/configuration/LoggerConfiguration.h
    include/lintel/core/configuration/LoggerEntry.h
    include/lintel/core/configuration/LoggerPathConfiguration.h
    include/lintel/core/configuration/LoggerSinkConfiguration.h
    include/lintel/features/base/configuration/MessageQueueComponent.h
    include/lintel/features/base/configuration/MessageQueueEntry.h
    include/lintel/features/base/configuration/HistoryComponent.h
    include/lintel/features/base/configuration/HistoryServiceEntry.h
    include/lintel/features/base/configuration/EventBusComponent.h
    include/lintel/features/base/configuration/EventBusEntry.h
    include/lintel/features/base/configuration/ProcessComponent.h
    include/lintel/features/base/configuration/ProcessEntry.h
    include/lintel/features/base/configuration/SharedMemorySegmentComponent.h
    include/lintel/features/base/configuration/SharedMemorySegmentEntry.h
    # features - base - controller
    include/lintel/features/base/controller/EventBusDtos.h
    include/lintel/features/base/controller/GroupDto.h
    include/lintel/features/base/controller/HistoryDto.h
    include/lintel/features/base/controller/HistoryDtos.h
    include/lintel/features/base/controller/MessageQueueDto.h
    include/lintel/features/base/controller/MessageQueueDtos.h
    include/lintel/features/base/controller/ProcessGroupDto.h
    include/lintel/features/base/controller/ProcessGroupsDto.h
    include/lintel/features/base/controller/ProcessInfoDto.h
    include/lintel/features/base/controller/ProcessInfosDto.h
    include/lintel/features/base/controller/SharedMemoryRepositoryDto.h
    include/lintel/features/base/controller/SharedMemoryRepositoriesDto.h
    include/lintel/features/base/controller/SharedMemorySegmentDto.h
    include/lintel/features/base/controller/SharedMemorySegmentsDto.h
    include/lintel/features/base/controller/UserDto.h
    include/lintel/features/base/controller/UserGroupDto.h
    include/lintel/features/base/controller/UserLoginDto.h
    include/lintel/features/base/controller/UserNameDto.h
    include/lintel/features/base/controller/UserPasswordChangeDto.h
    include/lintel/features/base/controller/UserSessionDto.h
    include/lintel/features/base/controller/UserSessionsDto.h
    include/lintel/features/base/controller/UserTokenDto.h
    # features - base - models
    include/lintel/features/base/models/Group.h
    include/lintel/features/base/models/HistoryEntry.h
    include/lintel/features/base/models/Process.h
    include/lintel/features/base/models/ProcessGroup.h
    include/lintel/features/base/models/ProcessInfo.h
    include/lintel/features/base/models/MessageQueueStats.h
    include/lintel/core/models/ProcessName.h
    include/lintel/core/models/SharedMemorySegmentInfo.h
    include/lintel/features/base/models/User.h
    # features - base - msg
    include/lintel/features/base/msg/CorrelationId.h
    include/lintel/features/base/msg/Message.h
    include/lintel/features/base/msg/MessageQueue.h
    include/lintel/features/base/msg/MessageQueueBackend.h
    include/lintel/features/base/msg/MessageQueueCore.h
    include/lintel/features/base/msg/MessageQueueCoreFactory.h
    include/lintel/features/base/msg/MessageQueueRequestReply.h
    include/lintel/features/base/msg/MessageQueueShmCore.h
    include/lintel/features/base/msg/MessageQueues.h
    # features - base - events
    include/lintel/features/base/events/Event.h
    include/lintel/features/base/events/EventBus.h
    include/lintel/core/shm/BoostSegmentAllocator.h
    include/lintel/core/shm/ShmSegmentAccessor.h
    include/lintel/core/shm/ShmView.h
    include/lintel/core/shm/SharedMemoryRepository.h
    include/lintel/core/shm/SharedMemoryRepositoryRegistry.h
    include/lintel/core/shm/containers/ArrayShm.h
    include/lintel/core/shm/containers/QueueShm.h
    include/lintel/core/shm/containers/RingBufferShm.h
    # features - base - provider
    include/lintel/features/base/provider/GroupProvider.h
    # features - base - repositories
    include/lintel/features/base/repositories/GroupRepository.h
    include/lintel/features/base/repositories/HistoryRepository.h
    include/lintel/features/base/repositories/IMessageQueueRepository.h
    include/lintel/features/base/repositories/MessageQueueQuery.h
    include/lintel/features/base/repositories/MessageQueueRepository.h
    include/lintel/features/base/repositories/SharedMemoryRepository.h
    include/lintel/features/base/repositories/UserRepository.h
    # features - base - services
    include/lintel/features/base/services/AuthService.h
    include/lintel/features/base/services/ExecutorService.h
    include/lintel/features/base/services/HistoryService.h
    include/lintel/features/base/services/IAuthService.h
    include/lintel/features/base/services/IHistoryService.h
    include/lintel/features/base/services/InitializeService.h
    include/lintel/features/base/services/ISharedMemorySegmentManager.h
    include/lintel/features/base/services/MessageQueueService.h
    include/lintel/features/base/services/EventBusService.h
    include/lintel/features/base/services/NoopHistoryService.h
    include/lintel/features/base/services/ProcessArgumentService.h
    include/lintel/features/base/services/SchedulerService.h
    include/lintel/features/base/services/SharedMemoryCleanupService.h
    include/lintel/features/base/services/SharedMemorySegmentManager.h
    # features - cli
    include/lintel/app/CommandLineFeature.h
    include/lintel/features/cli/CliTypes.h
    include/lintel/features/cli/models/AbstractCommandLineMenu.h
    include/lintel/features/cli/models/Command.h
    include/lintel/features/cli/models/CommandHistoryEntry.h
    include/lintel/features/cli/models/CommandLineComponent.h
    include/lintel/features/cli/models/CommandParser.h
    include/lintel/features/cli/providers/AuthArgumentProvider.h
    include/lintel/features/cli/services/AuthCliService.h
    include/lintel/features/cli/services/CommandLineHistoryService.h
    include/lintel/features/cli/services/CommandLineService.h
    include/lintel/features/cli/services/IInputService.h
    include/lintel/features/cli/services/ITerminalService.h
    include/lintel/features/cli/services/InputService.h
    include/lintel/features/cli/services/TerminalService.h
    include/lintel/features/cli/utils/CommandLineUtils.h
    include/lintel/features/cli/components/AdminCliComponent.h
    include/lintel/features/cli/components/EventBusCliComponent.h
    include/lintel/features/cli/components/HistoryCliComponent.h
    include/lintel/features/cli/components/MessageQueueCliComponent.h
    include/lintel/features/cli/components/ProcessCliComponent.h
    include/lintel/features/cli/components/SharedMemoryCliComponent.h
    include/lintel/features/cli/components/UserManagementCliComponent.h
    # features - common
    include/lintel/core/Feature.h
    include/lintel/core/Features.h
    # features - http
    include/lintel/app/HttpFeature.h
    include/lintel/features/http/ExampleController.h
    include/lintel/features/http/configuration/ClientConfiguration.h
    include/lintel/features/http/configuration/HttpComponent.h
    include/lintel/features/http/configuration/HttpEntry.h
    include/lintel/features/http/configuration/ServerConfiguration.h
    include/lintel/features/http/provider/ClientProvider.h
    include/lintel/features/http/provider/ServerProvider.h
    include/lintel/features/http/service/Client.h
    include/lintel/features/http/service/ClientIpResolver.h
    include/lintel/features/http/service/ContentType.h
    include/lintel/features/http/service/Controller.h
    include/lintel/features/http/service/HttpStatusCodes.h
    include/lintel/core/exceptions/HttpBadRequestException.h
    include/lintel/features/http/service/HttpUnauthorizedException.h
    include/lintel/features/http/service/HttpClientHelper.h
    include/lintel/features/http/service/Server.h
    include/lintel/features/http/service/TlsReloadRegistry.h
    include/lintel/features/http/controllers/AdminApi.h
    include/lintel/features/http/controllers/AdminController.h
    include/lintel/features/http/controllers/EventBusApi.h
    include/lintel/features/http/controllers/EventBusController.h
    include/lintel/features/http/controllers/HistoryApi.h
    include/lintel/features/http/controllers/HistoryController.h
    include/lintel/features/http/controllers/MessageQueueApi.h
    include/lintel/features/http/controllers/MessageQueueController.h
    include/lintel/features/http/controllers/ProcessApi.h
    include/lintel/features/http/controllers/ProcessController.h
    include/lintel/features/http/controllers/SharedMemoryApi.h
    include/lintel/features/http/controllers/SharedMemoryController.h
    include/lintel/features/http/controllers/UserApi.h
    include/lintel/features/http/controllers/UserController.h
    # features - property
    include/lintel/app/PropertyFeature.h
    include/lintel/features/cli/components/PropertyCliComponent.h
    include/lintel/features/property/configuration/PropertyComponent.h
    include/lintel/features/property/configuration/PropertyEntry.h
    include/lintel/features/property/configuration/PropertyRepositoryComponent.h
    include/lintel/features/property/configuration/PropertyRepositoryEntry.h
    include/lintel/features/property/controller/PropertiesDto.h
    include/lintel/features/http/controllers/PropertyApi.h
    include/lintel/features/http/controllers/PropertyController.h
    include/lintel/features/property/controller/PropertyDto.h
    include/lintel/features/property/controller/PropertyValueDto.h
    include/lintel/features/property/events/PropertyChange.h
    include/lintel/features/property/exceptions/PropertyNoRuntimeChangeSupported.h
    include/lintel/features/property/exceptions/PropertyNotFoundException.h
    include/lintel/features/property/factories/PropertyFactory.h
    include/lintel/core/models/DataStorage.h
    include/lintel/core/models/Property.h
    include/lintel/core/models/PropertyBase.h
    include/lintel/core/models/PropertyRepositoryType.h
    include/lintel/core/models/PropertyValueStorage.h
    include/lintel/features/property/repositories/DatabasePropertyRepository.h
    include/lintel/features/property/repositories/FilePropertyRepository.h
    include/lintel/features/property/repositories/PropertyRepository.h
    include/lintel/features/property/repositories/SharedMemoryPropertyRepository.h
    include/lintel/features/property/services/PropertyService.h
    include/lintel/features/property/strategies/ConfigSerializationStrategy.h
    include/lintel/features/property/strategies/XMLConfigSerializationStrategy.h
)

set(sources
    # core
    app/StartupBuilder.cpp
    # core - models
    core/models/SharedMemorySegment.cpp
    # core - shm
    core/shm/ShmView.cpp
    # core - persistence
    core/persistence/Arguments.cpp
    core/persistence/Connection.cpp
    core/persistence/ConnectionPool.cpp
    core/persistence/ConnectionPoolRegistry.cpp
    core/persistence/ScopedConnection.cpp
    core/persistence/Cursor.cpp
    core/persistence/DatabaseConnectionConfigurations.cpp
    core/persistence/Notify.cpp
    core/persistence/Parameter.cpp
    core/persistence/ParameterBuilder.cpp
    core/persistence/PreparedStatement.cpp
    core/persistence/Result.cpp
    core/persistence/Statement.cpp
    core/persistence/Transaction.cpp
    # core - persistence - postgresql
    core/persistence/postgresql/Connection.cpp
    core/persistence/postgresql/Cursor.cpp
    core/persistence/postgresql/Notify.cpp
    core/persistence/postgresql/PreparedStatement.cpp
    core/persistence/postgresql/Result.cpp
    core/persistence/postgresql/Statement.cpp
    core/persistence/postgresql/Transaction.cpp
    # core - persistence - sqlite3
    core/persistence/sqlite3/Connection.cpp
    core/persistence/sqlite3/Cursor.cpp
    core/persistence/sqlite3/Notify.cpp
    core/persistence/sqlite3/PreparedStatement.cpp
    core/persistence/sqlite3/Result.cpp
    core/persistence/sqlite3/Statement.cpp
    core/persistence/sqlite3/Transaction.cpp
    # core - platform
    core/platform/Platform.cpp
    # core - plugins
    app/plugins/AdminUserBootstrapPlugin.cpp
    app/plugins/DatabaseBootstrapPlugin.cpp
    app/plugins/MessageQueueBootstrapPlugin.cpp
    app/plugins/SharedMemoryBootstrapPlugin.cpp
    app/plugins/SingleInstanceBootstrapPlugin.cpp
    app/plugins/VirtualGroupBootstrapPlugin.cpp
    # core - services
    core/services/BootstrapService.cpp
    core/services/DirectoryService.cpp
    core/services/FileService.cpp
    core/services/LoggerService.cpp
    core/services/PersistableService.cpp
    core/services/ReadinessRegistry.cpp
    features/base/services/ProcessService.cpp
    features/base/services/SharedMemoryService.cpp
    features/base/services/SharedMemoryRepository.cpp
    features/base/services/SharedMemoryRepositoryRegistry.cpp
    core/services/StopWatchService.cpp
    # core - utils
    core/utils/Cryption.cpp
    core/utils/MemorySize.cpp
    core/utils/ProcessResourceReader.cpp
    core/utils/RegexUtils.cpp
    core/utils/StringUtils.cpp
    core/utils/UUID.cpp
    core/utils/Argument.cpp
    # features - base
    app/BaseFeature.cpp
    # features - base - configuration
    core/configuration/Component.cpp
    core/configuration/Configuration.cpp
    app/ConfigurationComponentBuilder.cpp
    core/configuration/DatabaseConnectionComponent.cpp
    core/configuration/DatabaseConnectionEntry.cpp
    core/configuration/Entry.cpp
    core/configuration/EnvironmentConfiguration.cpp
    core/configuration/LoggerComponent.cpp
    core/configuration/LoggerConfiguration.cpp
    core/configuration/LoggerEntry.cpp
    features/base/configuration/HistoryComponent.cpp
    features/base/configuration/HistoryServiceEntry.cpp
    core/configuration/LoggerPathConfiguration.cpp
    core/configuration/LoggerSinkConfiguration.cpp
    features/base/configuration/MessageQueueComponent.cpp
    features/base/configuration/MessageQueueEntry.cpp
    features/base/configuration/ProcessComponent.cpp
    features/base/configuration/ProcessEntry.cpp
    features/base/configuration/SharedMemorySegmentComponent.cpp
    features/base/configuration/SharedMemorySegmentEntry.cpp
    # features - base - controller
    features/base/controller/EventBusDtos.cpp
    features/base/controller/GroupDto.cpp
    features/base/controller/HistoryDto.cpp
    features/base/controller/HistoryDtos.cpp
    features/base/controller/MessageQueueDto.cpp
    features/base/controller/MessageQueueDtos.cpp
    features/base/controller/ProcessGroupDto.cpp
    features/base/controller/ProcessGroupsDto.cpp
    features/base/controller/ProcessInfoDto.cpp
    features/base/controller/ProcessInfosDto.cpp
    features/base/controller/SharedMemoryRepositoriesDto.cpp
    features/base/controller/SharedMemoryRepositoryDto.cpp
    features/base/controller/SharedMemorySegmentDto.cpp
    features/base/controller/SharedMemorySegmentsDto.cpp
    features/base/controller/UserDto.cpp
    features/base/controller/UserGroupDto.cpp
    features/base/controller/UserLoginDto.cpp
    features/base/controller/UserNameDto.cpp
    features/base/controller/UserPasswordChangeDto.cpp
    features/base/controller/UserSessionDto.cpp
    features/base/controller/UserSessionsDto.cpp
    features/base/controller/UserTokenDto.cpp
    # features - base - models
    features/base/models/Group.cpp
    features/base/models/HistoryEntry.cpp
    features/base/models/Process.cpp
    features/base/models/ProcessGroup.cpp
    features/base/models/ProcessInfo.cpp
    features/base/models/MessageQueueStats.cpp
    core/models/SharedMemorySegmentInfo.cpp
    features/base/models/User.cpp
    # features - base - msg
    features/base/msg/Message.cpp
    features/base/msg/MessageQueueCore.cpp
    features/base/msg/MessageQueueShmCore.cpp
    # features - base - repositories
    features/base/repositories/GroupRepository.cpp
    features/base/repositories/HistoryRepository.cpp
    features/base/repositories/MessageQueueRepository.cpp
    features/base/repositories/SharedMemoryRepository.cpp
    features/base/repositories/UserRepository.cpp
    # features - base - services
    features/base/services/AuthService.cpp
    features/base/services/ExecutorService.cpp
    features/base/services/HistoryService.cpp
    features/base/services/InitializeService.cpp
    features/base/services/MessageQueueService.cpp
    features/base/services/ProcessArgumentService.cpp
    features/base/services/SchedulerService.cpp
    features/base/services/SharedMemoryCleanupService.cpp
    features/base/services/SharedMemorySegmentManager.cpp
    # features - cli
    app/CommandLineFeature.cpp
    features/cli/models/AbstractCommandLineMenu.cpp
    features/cli/models/Command.cpp
    features/cli/models/CommandLineComponent.cpp
    features/cli/models/CommandParser.cpp
    features/cli/providers/AuthArgumentProvider.cpp
    features/cli/services/AuthCliService.cpp
    features/cli/services/CommandLineHistoryService.cpp
    features/cli/services/CommandLineService.cpp
    features/cli/services/InputService.cpp
    features/cli/services/TerminalService.cpp
    features/cli/utils/CommandLineUtils.cpp
    features/cli/components/AdminCliComponent.cpp
    features/cli/components/EventBusCliComponent.cpp
    features/cli/components/HistoryCliComponent.cpp
    features/cli/components/MessageQueueCliComponent.cpp
    features/cli/components/ProcessCliComponent.cpp
    features/cli/components/SharedMemoryCliComponent.cpp
    features/cli/components/UserManagementCliComponent.cpp
    # features - http
    app/HttpFeature.cpp
    features/http/ExampleController.cpp
    features/http/configuration/ClientConfiguration.cpp
    features/http/configuration/HttpComponent.cpp
    features/http/configuration/HttpEntry.cpp
    features/http/configuration/ServerConfiguration.cpp
    features/http/provider/ClientProvider.cpp
    features/http/provider/ServerProvider.cpp
    features/http/service/Client.cpp
    features/http/service/ClientIpResolver.cpp
    features/http/service/ContentType.cpp
    features/http/service/HttpClientHelper.cpp
    features/http/service/Controller.cpp
    features/http/service/HttpStatusCode.cpp
    features/http/service/Server.cpp
    features/http/service/TlsReloadRegistry.cpp
    features/http/controllers/AdminApi.cpp
    features/http/controllers/AdminController.cpp
    features/http/controllers/EventBusApi.cpp
    features/http/controllers/EventBusController.cpp
    features/http/controllers/HistoryApi.cpp
    features/http/controllers/HistoryController.cpp
    features/http/controllers/MessageQueueApi.cpp
    features/http/controllers/MessageQueueController.cpp
    features/http/controllers/ProcessApi.cpp
    features/http/controllers/ProcessController.cpp
    features/http/controllers/SharedMemoryApi.cpp
    features/http/controllers/SharedMemoryController.cpp
    features/http/controllers/UserApi.cpp
    features/http/controllers/UserController.cpp
    # features - property
    app/PropertyFeature.cpp
    features/cli/components/PropertyCliComponent.cpp
    features/property/configuration/PropertyComponent.cpp
    features/property/configuration/PropertyEntry.cpp
    features/property/configuration/PropertyRepositoryComponent.cpp
    features/property/configuration/PropertyRepositoryEntry.cpp
    features/property/controller/PropertiesDto.cpp
    features/http/controllers/PropertyApi.cpp
    features/http/controllers/PropertyController.cpp
    features/property/controller/PropertyDto.cpp
    features/property/controller/PropertyValueDto.cpp
    features/property/factories/PropertyFactory.cpp
    core/models/Property.cpp
    features/property/repositories/DatabasePropertyRepository.cpp
    features/property/repositories/FilePropertyRepository.cpp
    features/property/repositories/SharedMemoryPropertyRepository.cpp
    features/property/services/PropertyService.cpp
    features/property/strategies/XMLConfigSerializationStrategy.cpp
    # features - base - events
    features/base/events/EventBus.cpp
    features/base/events/TopicMatcher.cpp
    # features - base - configuration
    features/base/configuration/EventBusComponent.cpp
    features/base/configuration/EventBusEntry.cpp
    # features - base - services
    features/base/services/EventBusService.cpp
)

# ---- per-module targets -----------------------------------------------------
# Dependency graph (a DAG): core <- base <- property <- http <- cli <- app.
# lintel::lintel is the INTERFACE aggregate that links every module - it is what
# this repository's tests and examples link. A consumer that only needs a subset
# links the corresponding module (e.g. lintel::property) and pulls in only its
# prerequisites (linked PUBLIC below, so includes/link flags stay transitive).
set(LINTEL_PCH_HEADERS
    <algorithm>
    <atomic>
    <chrono>
    <condition_variable>
    <cstddef>
    <cstdint>
    <cstring>
    <exception>
    <filesystem>
    <functional>
    <fstream>
    <iostream>
    <map>
    <memory>
    <mutex>
    <new>
    <optional>
    <ostream>
    <set>
    <shared_mutex>
    <source_location>
    <sstream>
    <stdexcept>
    <string>
    <string_view>
    <thread>
    <typeindex>
    <type_traits>
    <utility>
    <vector>
)

# Partition the explicit header/source lists by module (path prefix). The lists
# above are kept flat and single-sourced; the filters below only route them.
set(LINTEL_CORE_HEADERS ${headers})
list(FILTER LINTEL_CORE_HEADERS INCLUDE REGEX "^include/lintel/(config\\.h|core/)")
set(LINTEL_BASE_HEADERS ${headers})
list(FILTER LINTEL_BASE_HEADERS INCLUDE REGEX "^include/lintel/features/base/")
set(LINTEL_PROPERTY_HEADERS ${headers})
list(FILTER LINTEL_PROPERTY_HEADERS INCLUDE REGEX "^include/lintel/features/property/")
set(LINTEL_HTTP_HEADERS ${headers})
list(FILTER LINTEL_HTTP_HEADERS INCLUDE REGEX "^include/lintel/features/http/")
set(LINTEL_CLI_HEADERS ${headers})
list(FILTER LINTEL_CLI_HEADERS INCLUDE REGEX "^include/lintel/features/cli/")
set(LINTEL_APP_HEADERS ${headers})
list(FILTER LINTEL_APP_HEADERS INCLUDE REGEX "^include/lintel/app/")

set(LINTEL_CORE_SOURCES ${sources})
list(FILTER LINTEL_CORE_SOURCES INCLUDE REGEX "^core/")
set(LINTEL_BASE_SOURCES ${sources})
list(FILTER LINTEL_BASE_SOURCES INCLUDE REGEX "^features/base/")
set(LINTEL_PROPERTY_SOURCES ${sources})
list(FILTER LINTEL_PROPERTY_SOURCES INCLUDE REGEX "^features/property/")
set(LINTEL_HTTP_SOURCES ${sources})
list(FILTER LINTEL_HTTP_SOURCES INCLUDE REGEX "^features/http/")
set(LINTEL_CLI_SOURCES ${sources})
list(FILTER LINTEL_CLI_SOURCES INCLUDE REGEX "^features/cli/")
set(LINTEL_APP_SOURCES ${sources})
list(FILTER LINTEL_APP_SOURCES INCLUDE REGEX "^app/")

# Common settings applied to every module target. Include directories: everything
# needed permanently is passed on via the PUBLIC link targets below; the *_INCLUDE_DIRS
# variables point into the Conan cache of the build machine and would be dead weight
# in the installed export, so they are wrapped in $<BUILD_INTERFACE:...>. The include
# directory of the installed library itself is prefix-relative.
# project_warnings is a pure build target (CompilerWarnings.cmake). Wrapped in
# $<BUILD_INTERFACE:...> so it stays out of the install export while keeping the
# flags in the build tree.
function(lintel_setup_module target)
    target_compile_features(${target} PUBLIC cxx_std_20)
    if(MSVC)
        # Defined on the command line (PUBLIC, so it also reaches consumers of the
        # installed package): <Windows.h> must never define the min/max macros,
        # they would break every std::min/std::max in the public headers.
        target_compile_definitions(${target} PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
    endif()
    target_include_directories(${target} SYSTEM PUBLIC
            "$<BUILD_INTERFACE:${OpenSSL_INCLUDES};${magic_enum_INCLUDE_DIRS};${RapidJSON_INCLUDE_DIRS};${CMAKE_CURRENT_SOURCE_DIR}/include>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
    target_link_libraries(${target} PRIVATE $<BUILD_INTERFACE:project_warnings>)
    # Boost.UUID formats UUIDs via SSE2 intrinsics. Without optimization (-O0) GCC 13 no
    # longer inlines the _mm_slli_si128 intrinsic and the backend call with a variable
    # shift produces a hard error ("the last argument must be an 8-bit immediate").
    target_compile_definitions(${target} PRIVATE $<$<CONFIG:Debug>:BOOST_UUID_NO_SIMD>)
    if(ENABLE_PCH)
        target_precompile_headers(${target} PRIVATE ${LINTEL_PCH_HEADERS})
    endif()
endfunction()

add_library(lintel_core ${LINTEL_CORE_HEADERS} ${LINTEL_CORE_SOURCES})
lintel_setup_module(lintel_core)
# magic_enum and rapidjson are part of the interface through public headers and are
# therefore linked as targets (rapidjson thereby replaces the previous
# ${rapidjson_LIBRARIES_DEBUG} that was resolved in Debug only and was empty in the
# release export).
target_link_libraries(lintel_core PUBLIC tinyxml2::tinyxml2 PostgreSQL::PostgreSQL SQLite::SQLite3
        Threads::Threads OpenSSL::SSL OpenSSL::Crypto rapidjson magic_enum::magic_enum
        spdlog::spdlog_header_only Boost::boost Boost::headers Boost::filesystem)
if(APPLE)
  # processStartTicksOf() reads the start time through proc_pidinfo().
  target_link_libraries(lintel_core PRIVATE proc)
endif()
set_target_properties(lintel_core PROPERTIES EXPORT_NAME core)
add_library(lintel::core ALIAS lintel_core)

add_library(lintel_base ${LINTEL_BASE_HEADERS} ${LINTEL_BASE_SOURCES})
lintel_setup_module(lintel_base)
target_link_libraries(lintel_base PUBLIC lintel_core)
set_target_properties(lintel_base PROPERTIES EXPORT_NAME base)
add_library(lintel::base ALIAS lintel_base)

add_library(lintel_property ${LINTEL_PROPERTY_HEADERS} ${LINTEL_PROPERTY_SOURCES})
lintel_setup_module(lintel_property)
target_link_libraries(lintel_property PUBLIC lintel_base)
set_target_properties(lintel_property PROPERTIES EXPORT_NAME property)
add_library(lintel::property ALIAS lintel_property)

add_library(lintel_http ${LINTEL_HTTP_HEADERS} ${LINTEL_HTTP_SOURCES})
lintel_setup_module(lintel_http)
target_link_libraries(lintel_http PUBLIC lintel_property httplib::httplib)
set_target_properties(lintel_http PROPERTIES EXPORT_NAME http)
add_library(lintel::http ALIAS lintel_http)

add_library(lintel_cli ${LINTEL_CLI_HEADERS} ${LINTEL_CLI_SOURCES})
lintel_setup_module(lintel_cli)
target_link_libraries(lintel_cli PUBLIC lintel_http)
set_target_properties(lintel_cli PROPERTIES EXPORT_NAME cli)
add_library(lintel::cli ALIAS lintel_cli)

add_library(lintel_app ${LINTEL_APP_HEADERS} ${LINTEL_APP_SOURCES})
lintel_setup_module(lintel_app)
# app composes every feature; linking cli pulls in http -> property -> base -> core.
target_link_libraries(lintel_app PUBLIC lintel_cli)
set_target_properties(lintel_app PROPERTIES EXPORT_NAME app)
add_library(lintel::app ALIAS lintel_app)

# Aggregate target: preserves the historical lintel::lintel entry point and links
# every module. Existing consumers (and this repo's tests/examples) keep working.
add_library(lintel INTERFACE)
target_link_libraries(lintel INTERFACE lintel_app)
set_target_properties(lintel PROPERTIES EXPORT_NAME lintel)
add_library(lintel::lintel ALIAS lintel)

# --- Installation and CMake package ---------------------------------------------
# In this directory PROJECT_SOURCE_DIR points to src/ (the project() call above
# resets it), the repo root is one level up - same as for CONFIG_DIRECTORY above.
set(LINTEL_SOURCE_ROOT "${PROJECT_SOURCE_DIR}/..")

# Target folder from GNUInstallDirs (included at top level). Config, version and
# targets file must live in the same directory: lintelConfig.cmake loads the
# targets file via ${CMAKE_CURRENT_LIST_DIR}, and CMake resolves relative
# INSTALL_INTERFACE entries against it.
set(LINTEL_CMAKE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/lintel")
set(LINTEL_DOCS_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/doc/lintel")

install(TARGETS lintel_core lintel_base lintel_property lintel_http lintel_cli lintel_app
        EXPORT lintelTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS lintel EXPORT lintelTargets)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/lintel DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Package configuration. @PACKAGE_INIT@ in the template makes it relocatable, and the
# find_dependency() calls for the dependencies from the public export live there as well.
# The version file has to be named lintelConfigVersion.cmake - only then
# find_package(lintel <version>) finds the version information.
configure_package_config_file(
        ${LINTEL_SOURCE_ROOT}/cmake/lintelConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/lintelConfig.cmake
        INSTALL_DESTINATION ${LINTEL_CMAKE_INSTALL_DIR})
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/lintelConfigVersion.cmake
        COMPATIBILITY SameMajorVersion)

install(EXPORT lintelTargets
        NAMESPACE lintel::
        DESTINATION ${LINTEL_CMAKE_INSTALL_DIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lintelConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/lintelConfigVersion.cmake
        DESTINATION ${LINTEL_CMAKE_INSTALL_DIR})

# License and docs belong into the package as well - otherwise the license obligation does not travel with it.
install(FILES ${LINTEL_SOURCE_ROOT}/LICENSE
        ${LINTEL_SOURCE_ROOT}/README.md
        ${LINTEL_SOURCE_ROOT}/swagger.yaml
        DESTINATION ${LINTEL_DOCS_INSTALL_DIR})
