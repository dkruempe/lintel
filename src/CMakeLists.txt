cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0025 NEW)
project(base_library VERSION 0.0.1)
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
IF (NOT DEFINED MSG_QUUEUE_CONTENT_SIZE)
    set(MSG_QUUEUE_CONTENT_SIZE 1000)
endif ()

configure_file(
        ${PROJECT_SOURCE_DIR}/include/base_library/config.h.in
        ${PROJECT_SOURCE_DIR}/include/base_library/config.h)

# dependencies
if (NOT TARGET kruempelmann::Hypodermic)
    find_package(Hypodermic REQUIRED)
endif ()

find_package(fmt REQUIRED)
find_package(tinyxml2 REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(magic_enum REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(httplib REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost 1.75.0 COMPONENTS system filesystem REQUIRED)
find_package(RapidJSON REQUIRED)
find_package(date REQUIRED)
find_package(spdlog REQUIRED)
find_package(Threads REQUIRED)

# build
set(headers
    include/base_library/config.h
    # core - exceptions
    include/base_library/core/exceptions/ConfigShmSegmentNotFound.h
    include/base_library/core/exceptions/FileServiceFileExists.h
    include/base_library/core/exceptions/FileServiceIsNotFileException.h
    include/base_library/core/exceptions/LoggerServiceNotInitialized.h
    include/base_library/core/exceptions/ShmSegmentNotFound.h
    include/base_library/core/exceptions/SQLException.h
    # core - models
    include/base_library/core/models/BootstrapSequence.h
    include/base_library/core/models/JsonSerializable.h
    include/base_library/core/models/SharedMemorySegment.h
    include/base_library/core/models/ShutdownPriority.h
    # core - persistence
    include/base_library/core/persistence/Arguments.h
    include/base_library/core/persistence/Connection.h
    include/base_library/core/persistence/ConnectionType.h
    include/base_library/core/persistence/DatabaseConnectionConfigurations.h
    include/base_library/core/persistence/Identifier.h
    include/base_library/core/persistence/Notify.h
    include/base_library/core/persistence/Parameter.h
    include/base_library/core/persistence/ParameterBuilder.h
    include/base_library/core/persistence/PreparedStatement.h
    include/base_library/core/persistence/Result.h
    include/base_library/core/persistence/Serialization.h
    include/base_library/core/persistence/Statement.h
    include/base_library/core/persistence/Transaction.h
    # core - persistence - postgresql
    include/base_library/core/persistence/postgresql/Connection.h
    include/base_library/core/persistence/postgresql/Notify.h
    include/base_library/core/persistence/postgresql/PreparedStatement.h
    include/base_library/core/persistence/postgresql/Result.h
    include/base_library/core/persistence/postgresql/Serialization.h
    include/base_library/core/persistence/postgresql/Statement.h
    include/base_library/core/persistence/postgresql/Transaction.h
    # core - persistence - sqlite3
    include/base_library/core/persistence/sqlite3/Connection.h
    include/base_library/core/persistence/sqlite3/Notify.h
    include/base_library/core/persistence/sqlite3/PreparedStatement.h
    include/base_library/core/persistence/sqlite3/Result.h
    include/base_library/core/persistence/sqlite3/Serialization.h
    include/base_library/core/persistence/sqlite3/Statement.h
    include/base_library/core/persistence/sqlite3/Transaction.h
    # core - plugins
    include/base_library/core/plugins/AdminUserBootstrapPlugin.h
    include/base_library/core/plugins/BootstrapPlugin.h
    include/base_library/core/plugins/DatabaseBootstrapPlugin.h
    include/base_library/core/plugins/MessageQueueBootstrapPlugin.h
    include/base_library/core/plugins/SharedMemoryBootstrapPlugin.h
    include/base_library/core/plugins/SingleInstanceBootstrapPlugin.h
    include/base_library/core/plugins/VirtualGroupBootstrapPlugin.h
    # core - services
    include/base_library/core/services/AbstractService.h
    include/base_library/core/services/BootstrapService.h
    include/base_library/core/services/DirectoryService.h
    include/base_library/core/services/FileService.h
    include/base_library/core/services/LoggerService.h
    include/base_library/core/services/PersistableBean.h
    include/base_library/core/services/PersistableService.h
    include/base_library/core/services/ProcessService.h
    include/base_library/core/services/ISharedMemoryService.h
    include/base_library/core/services/PropertyRegistration.h
    include/base_library/core/services/SharedMemoryService.h
    include/base_library/core/services/SignalService.h
    include/base_library/core/services/StopWatchService.h
    include/base_library/core/services/StringifyService.h
    include/base_library/core/StartupBuilder.h
    # core - utils
    include/base_library/core/utils/Cryption.h
    include/base_library/core/utils/MemorySize.h
    include/base_library/core/utils/RegexUtils.h
    include/base_library/core/utils/StringUtils.h
    include/base_library/core/utils/TableBuilder.h
    include/base_library/core/utils/TypeName.h
    include/base_library/core/utils/UUID.h
    include/base_library/core/utils/Argument.h
    # features - base
    include/base_library/features/base/BaseFeature.h
    # features - base - configuration
    include/base_library/features/base/configuration/Component.h
    include/base_library/features/base/configuration/Configuration.h
    include/base_library/features/base/configuration/ConfigurationComponentBuilder.h
    include/base_library/features/base/configuration/ConfigurationException.h
    include/base_library/features/base/configuration/DatabaseConnectionComponent.h
    include/base_library/features/base/configuration/DatabaseConnectionEntry.h
    include/base_library/features/base/configuration/Entry.h
    include/base_library/features/base/configuration/EnvironmentConfiguration.h
    include/base_library/features/base/configuration/LoggerComponent.h
    include/base_library/features/base/configuration/LoggerConfiguration.h
    include/base_library/features/base/configuration/LoggerEnrty.h
    include/base_library/features/base/configuration/LoggerPathConfiguration.h
    include/base_library/features/base/configuration/LoggerSinkConfiguration.h
    include/base_library/features/base/configuration/MessageQueueComponent.h
    include/base_library/features/base/configuration/MessageQueueEntry.h
    include/base_library/features/base/configuration/HistoryComponent.h
    include/base_library/features/base/configuration/HistoryServiceEntry.h
    include/base_library/features/base/configuration/EventBusComponent.h
    include/base_library/features/base/configuration/EventBusEntry.h
    include/base_library/features/base/configuration/ProcessComponent.h
    include/base_library/features/base/configuration/ProcessEntry.h
    include/base_library/features/base/configuration/SharedMemorySegmentComponent.h
    include/base_library/features/base/configuration/SharedMemorySegmentEntry.h
    # features - base - controller
    include/base_library/features/base/controller/GroupDto.h
    include/base_library/features/base/controller/HistoryDto.h
    include/base_library/features/base/controller/HistoryDtos.h
    include/base_library/features/base/controller/MessageQueueDto.h
    include/base_library/features/base/controller/MessageQueueDtos.h
    include/base_library/features/base/controller/ProcessGroupDto.h
    include/base_library/features/base/controller/ProcessGroupsDto.h
    include/base_library/features/base/controller/ProcessInfoDto.h
    include/base_library/features/base/controller/ProcessInfosDto.h
    include/base_library/features/base/controller/SharedMemoryRepositoryDto.h
    include/base_library/features/base/controller/SharedMemoryRepsoitoriesDto.h
    include/base_library/features/base/controller/SharedMemorySegmentDto.h
    include/base_library/features/base/controller/SharedMemorySegmentsDto.h
    include/base_library/features/base/controller/UserDto.h
    include/base_library/features/base/controller/UserGroupDto.h
    include/base_library/features/base/controller/UserLoginDto.h
    include/base_library/features/base/controller/UserNameDto.h
    include/base_library/features/base/controller/UserPasswordChangeDto.h
    include/base_library/features/base/controller/UserSessionDto.h
    include/base_library/features/base/controller/UserSessionsDto.h
    include/base_library/features/base/controller/UserTokenDto.h
    # features - base - models
    include/base_library/features/base/models/Group.h
    include/base_library/features/base/models/HistoryEntry.h
    include/base_library/features/base/models/Process.h
    include/base_library/features/base/models/ProcessGroup.h
    include/base_library/features/base/models/ProcessInfo.h
    include/base_library/features/base/models/ProcessName.h
    include/base_library/features/base/models/SharedMemorySegmentInfo.h
    include/base_library/features/base/models/User.h
    # features - base - msg
    include/base_library/features/base/msg/Message.h
    include/base_library/features/base/msg/MessageQueue.h
    include/base_library/features/base/msg/MessageQueues.h
    # features - base - events
    include/base_library/features/base/events/Event.h
    include/base_library/features/base/events/EventBus.h
    # features - base - provider
    include/base_library/features/base/provider/GroupProvider.h
    # features - base - repositories
    include/base_library/features/base/repositories/GroupRepository.h
    include/base_library/features/base/repositories/HistoryRepository.h
    include/base_library/features/base/repositories/IMessageQueueRepository.h
    include/base_library/features/base/repositories/MessageQueueRepository.h
    include/base_library/features/base/repositories/SharedMemoryRepository.h
    include/base_library/features/base/repositories/UserRepository.h
    # features - base - services
    include/base_library/features/base/services/AuthService.h
    include/base_library/features/base/services/ExecutorService.h
    include/base_library/features/base/services/HistoryService.h
    include/base_library/features/base/services/IAuthService.h
    include/base_library/features/base/services/IHistoryService.h
    include/base_library/features/base/services/InitializeService.h
    include/base_library/features/base/services/ISharedMemorySegmentManager.h
    include/base_library/features/base/services/MessageQueueService.h
    include/base_library/features/base/services/EventBusService.h
    include/base_library/features/base/services/NoopHistoryService.h
    include/base_library/features/base/services/ProcessArgumentService.h
    include/base_library/features/base/services/SchedulerService.h
    include/base_library/features/base/services/SharedMemorySegmentManager.h
    # features - cli
    include/base_library/features/cli/CommandLineFeature.h
    include/base_library/features/cli/CliTypes.h
    include/base_library/features/cli/models/AbstractCommandLineMenu.h
    include/base_library/features/cli/models/Command.h
    include/base_library/features/cli/models/CommandHistoryEntry.h
    include/base_library/features/cli/models/CommandLineComponent.h
    include/base_library/features/cli/models/CommandParser.h
    include/base_library/features/cli/providers/AuthArgumentProvider.h
    include/base_library/features/cli/services/AuthCliService.h
    include/base_library/features/cli/services/CommandLineHistoryService.h
    include/base_library/features/cli/services/CommandLineService.h
    include/base_library/features/cli/services/IInputService.h
    include/base_library/features/cli/services/ITerminalService.h
    include/base_library/features/cli/services/InputService.h
    include/base_library/features/cli/services/TerminalService.h
    include/base_library/features/cli/utils/CommandLineUtils.h
    include/base_library/features/cli/components/HistoryCliComponent.h
    include/base_library/features/cli/components/MessageQueueCliComponent.h
    include/base_library/features/cli/components/ProcessCliComponent.h
    include/base_library/features/cli/components/SharedMemoryCliComponent.h
    include/base_library/features/cli/components/UserManagementCliComponent.h
    # features - common
    include/base_library/features/Feature.h
    include/base_library/features/Features.h
    # features - http
    include/base_library/features/http/HttpFeature.h
    include/base_library/features/http/ExampleController.h
    include/base_library/features/http/configuration/ClientConfiguration.h
    include/base_library/features/http/configuration/HttpComponent.h
    include/base_library/features/http/configuration/HttpEntry.h
    include/base_library/features/http/configuration/ServerConfiguration.h
    include/base_library/features/http/provider/ClientProvider.h
    include/base_library/features/http/provider/ServerProvider.h
    include/base_library/features/http/service/Client.h
    include/base_library/features/http/service/ClientIpResolver.h
    include/base_library/features/http/service/ContentType.h
    include/base_library/features/http/service/Controller.h
    include/base_library/features/http/service/HttpStatusCodes.h
    include/base_library/features/http/service/HttpBadRequestException.h
    include/base_library/features/http/service/HttpUnauthorizedException.h
    include/base_library/features/http/service/HttpClientHelper.h
    include/base_library/features/http/service/Server.h
    include/base_library/features/http/controllers/HistoryApi.h
    include/base_library/features/http/controllers/HistoryController.h
    include/base_library/features/http/controllers/MessageQueueApi.h
    include/base_library/features/http/controllers/MessageQueueController.h
    include/base_library/features/http/controllers/ProcessApi.h
    include/base_library/features/http/controllers/ProcessController.h
    include/base_library/features/http/controllers/SharedMemoryApi.h
    include/base_library/features/http/controllers/SharedMemoryController.h
    include/base_library/features/http/controllers/UserApi.h
    include/base_library/features/http/controllers/UserController.h
    # features - property
    include/base_library/features/property/PropertyFeature.h
    include/base_library/features/property/command_line/PropertyCliComponent.h
    include/base_library/features/property/configuration/PropertyComponent.h
    include/base_library/features/property/configuration/PropertyEntry.h
    include/base_library/features/property/configuration/PropertyRepositoryComponent.h
    include/base_library/features/property/configuration/PropertyRepositoryEntry.h
    include/base_library/features/property/controller/PropertiesDto.h
    include/base_library/features/property/controller/PropertyApi.h
    include/base_library/features/property/controller/PropertyController.h
    include/base_library/features/property/controller/PropertyDto.h
    include/base_library/features/property/controller/PropertyValueDto.h
    include/base_library/features/property/events/PropertyChange.h
    include/base_library/features/property/exceptions/PropertyNoRuntimeChangeSupported.h
    include/base_library/features/property/exceptions/PropertyNotFoundException.h
    include/base_library/features/property/factories/PropertyFactory.h
    include/base_library/features/property/models/DataStorage.h
    include/base_library/features/property/models/Property.h
    include/base_library/features/property/models/PropertyBase.h
    include/base_library/features/property/models/PropertyRepositoryType.h
    include/base_library/features/property/models/PropertyValueStorage.h
    include/base_library/features/property/repositories/DatabasePropertyRepository.h
    include/base_library/features/property/repositories/FilePropertyRepository.h
    include/base_library/features/property/repositories/PropertyRepository.h
    include/base_library/features/property/repositories/SharedMemoryPropertyRepository.h
    include/base_library/features/property/services/PropertyService.h
    include/base_library/features/property/strategies/ConfigSerializationStrategy.h
    include/base_library/features/property/strategies/XMLConfigSerializationStrategy.h
)

set(sources
    # core
    core/StartupBuilder.cpp
    # core - models
    core/models/SharedMemorySegment.cpp
    # core - persistence
    core/persistence/Arguments.cpp
    core/persistence/Connection.cpp
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
    core/persistence/postgresql/Notify.cpp
    core/persistence/postgresql/PreparedStatement.cpp
    core/persistence/postgresql/Result.cpp
    core/persistence/postgresql/Statement.cpp
    core/persistence/postgresql/Transaction.cpp
    # core - persistence - sqlite3
    core/persistence/sqlite3/Connection.cpp
    core/persistence/sqlite3/Notify.cpp
    core/persistence/sqlite3/PreparedStatement.cpp
    core/persistence/sqlite3/Result.cpp
    core/persistence/sqlite3/Statement.cpp
    core/persistence/sqlite3/Transaction.cpp
    # core - plugins
    core/plugins/AdminUserBootstrapPlugin.cpp
    core/plugins/DatabaseBootstrapPlugin.cpp
    core/plugins/MessageQueueBootstrapPlugin.cpp
    core/plugins/SharedMemoryBootstrapPlugin.cpp
    core/plugins/SingleInstanceBootstrapPlugin.cpp
    core/plugins/VirtualGroupBootstrapPlugin.cpp
    # core - services
    core/services/BootstrapService.cpp
    core/services/DirectoryService.cpp
    core/services/FileService.cpp
    core/services/LoggerService.cpp
    core/services/PersistableService.cpp
    core/services/ProcessService.cpp
    core/services/SharedMemoryService.cpp
    core/services/StopWatchService.cpp
    # core - utils
    core/utils/Cryption.cpp
    core/utils/MemorySize.cpp
    core/utils/RegexUtils.cpp
    core/utils/StringUtils.cpp
    core/utils/UUID.cpp
    core/utils/Argument.cpp
    # features - base
    features/base/BaseFeature.cpp
    # features - base - configuration
    features/base/configuration/Component.cpp
    features/base/configuration/Configuration.cpp
    features/base/configuration/ConfigurationComponentBuilder.cpp
    features/base/configuration/DatabaseConnectionComponent.cpp
    features/base/configuration/DatabaseConnectionEntry.cpp
    features/base/configuration/Entry.cpp
    features/base/configuration/EnvironmentConfiguration.cpp
    features/base/configuration/LoggerComponent.cpp
    features/base/configuration/LoggerConfiguration.cpp
    features/base/configuration/LoggerEntry.cpp
    features/base/configuration/HistoryComponent.cpp
    features/base/configuration/HistoryServiceEntry.cpp
    features/base/configuration/LoggerPathConfiguration.cpp
    features/base/configuration/LoggerSinkConfiguration.cpp
    features/base/configuration/MessageQueueComponent.cpp
    features/base/configuration/MessageQueueEntry.cpp
    features/base/configuration/ProcessComponent.cpp
    features/base/configuration/ProcessEntry.cpp
    features/base/configuration/SharedMemorySegmentComponent.cpp
    features/base/configuration/SharedMemorySegmentEntry.cpp
    # features - base - controller
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
    features/base/models/SharedMemorySegmentInfo.cpp
    features/base/models/User.cpp
    # features - base - msg
    features/base/msg/Message.cpp
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
    features/base/services/SharedMemorySegmentManager.cpp
    # features - cli
    features/cli/CommandLineFeature.cpp
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
    features/cli/components/HistoryCliComponent.cpp
    features/cli/components/MessageQueueCliComponent.cpp
    features/cli/components/ProcessCliComponent.cpp
    features/cli/components/SharedMemoryCliComponent.cpp
    features/cli/components/UserManagementCliComponent.cpp
    # features - http
    features/http/HttpFeature.cpp
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
    features/http/service/Controller.cpp
    features/http/service/HttpStatusCode.cpp
    features/http/service/Server.cpp
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
    features/property/PropertyFeature.cpp
    features/property/command_line/PropertyCliComponent.cpp
    features/property/configuration/PropertyComponent.cpp
    features/property/configuration/PropertyEntry.cpp
    features/property/configuration/PropertyRepositoryComponent.cpp
    features/property/configuration/PropertyRepositoryEntry.cpp
    features/property/controller/PropertiesDto.cpp
    features/property/controller/PropertyApi.cpp
    features/property/controller/PropertyController.cpp
    features/property/controller/PropertyDto.cpp
    features/property/controller/PropertyValueDto.cpp
    features/property/factories/PropertyFactory.cpp
    features/property/models/Property.cpp
    features/property/repositories/DatabasePropertyRepository.cpp
    features/property/repositories/FilePropertyRepository.cpp
    features/property/repositories/SharedMemoryPropertyRepository.cpp
    features/property/services/PropertyService.cpp
    features/property/strategies/XMLConfigSerializationStrategy.cpp
    # features - base - events
    features/base/events/EventBus.cpp
    # features - base - configuration
    features/base/configuration/EventBusComponent.cpp
    features/base/configuration/EventBusEntry.cpp
    # features - base - services
    features/base/services/EventBusService.cpp
)

add_library(base_library ${headers} ${sources})
# Hypodermic includes: propagated via target_link_libraries for vendored target,
# explicit variable for find_package fallback
if (NOT TARGET kruempelmann::Hypodermic)
    set(HYPODERMIC_INCLUDE_DIRS_VAR ${Hypodermic_INCLUDE_DIRS})
else ()
    set(HYPODERMIC_INCLUDE_DIRS_VAR "")
endif ()

target_compile_features(base_library PUBLIC cxx_std_17)
target_include_directories(base_library
        SYSTEM PUBLIC
        "$<BUILD_INTERFACE:${OpenSSL_INCLUDES};${magic_enum_INCLUDE_DIRS};${RapidJSON_INCLUDE_DIRS};${HYPODERMIC_INCLUDE_DIRS_VAR};${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${OpenSSL_INCLUDES};${magic_enum_INCLUDE_DIRS};${RapidJSON_INCLUDE_DIRS};include>"
)
target_link_libraries(base_library PUBLIC fmt::fmt-header-only tinyxml2::tinyxml2 PostgreSQL::PostgreSQL SQLite::SQLite3
        Threads::Threads OpenSSL::SSL OpenSSL::Crypto httplib::httplib ${rapidjson_LIBRARIES_DEBUG} date::date spdlog::spdlog_header_only
        kruempelmann::Hypodermic Boost::boost Boost::headers Boost::filesystem PRIVATE project_warnings)

option(ENABLE_PCH "Enable precompiled headers" ON)
if(ENABLE_PCH)
    target_precompile_headers(base_library PRIVATE
        <algorithm>
        <atomic>
        <chrono>
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
        <optional>
        <ostream>
        <set>
        <shared_mutex>
        <sstream>
        <stdexcept>
        <string>
        <string_view>
        <thread>
        <type_traits>
        <utility>
        <vector>
        <spdlog/common.h>
        <spdlog/fwd.h>
        <base_library/core/services/LoggerService.h>
        <base_library/features/base/configuration/Configuration.h>
        <base_library/core/utils/StringUtils.h>
        <base_library/core/utils/TypeName.h>
    )
endif()

add_library(kruempelmann::base_library ALIAS base_library)
INSTALL(TARGETS base_library project_warnings
        EXPORT base_library_targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION lib
        INCLUDES DESTINATION include)
INSTALL(EXPORT base_library_targets
        DESTINATION lib/cmake/base_library
        FILE base_library_targets.cmake
        NAMESPACE kruempelmann::)
INSTALL(DIRECTORY include/base_library
        DESTINATION include)

# library package version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(base_library_config_version.cmake
        COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/base_library_config_version.cmake ${CMAKE_CURRENT_BINARY_DIR}/base_library_config_version.cmake
        DESTINATION lib/cmake/base_library)