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
if (NOT TARGET lintel::Hypodermic)
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
    include/lintel/config.h
    # core - exceptions
    include/lintel/core/exceptions/ConfigShmSegmentNotFound.h
    include/lintel/core/exceptions/FileServiceFileExists.h
    include/lintel/core/exceptions/FileServiceIsNotFileException.h
    include/lintel/core/exceptions/LoggerServiceNotInitialized.h
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
    include/lintel/core/plugins/AdminUserBootstrapPlugin.h
    include/lintel/core/plugins/BootstrapPlugin.h
    include/lintel/core/plugins/DatabaseBootstrapPlugin.h
    include/lintel/core/plugins/MessageQueueBootstrapPlugin.h
    include/lintel/core/plugins/SharedMemoryBootstrapPlugin.h
    include/lintel/core/plugins/SingleInstanceBootstrapPlugin.h
    include/lintel/core/plugins/VirtualGroupBootstrapPlugin.h
    # core - services
    include/lintel/core/services/AbstractService.h
    include/lintel/core/services/BootstrapService.h
    include/lintel/core/services/DirectoryService.h
    include/lintel/core/services/FileService.h
    include/lintel/core/services/LoggerService.h
    include/lintel/core/services/PersistableBean.h
    include/lintel/core/services/PersistableService.h
    include/lintel/core/services/ProcessService.h
    include/lintel/core/services/ISharedMemoryService.h
    include/lintel/core/services/PropertyRegistration.h
    include/lintel/core/services/SharedMemoryService.h
    include/lintel/core/services/ShmConstructors.h
    include/lintel/core/services/SignalService.h
    include/lintel/core/services/StopWatchService.h
    include/lintel/core/services/StringifyService.h
    include/lintel/core/StartupBuilder.h
    # core - utils
    include/lintel/core/utils/Cryption.h
    include/lintel/core/utils/MemorySize.h
    include/lintel/core/utils/ProcessResourceReader.h
    include/lintel/core/utils/RegexUtils.h
    include/lintel/core/utils/StringUtils.h
    include/lintel/core/utils/TableBuilder.h
    include/lintel/core/utils/TypeName.h
    include/lintel/core/utils/UUID.h
    include/lintel/core/utils/Argument.h
    # features - base
    include/lintel/features/base/BaseFeature.h
    # features - base - configuration
    include/lintel/features/base/configuration/Component.h
    include/lintel/features/base/configuration/Configuration.h
    include/lintel/features/base/configuration/ConfigurationComponentBuilder.h
    include/lintel/features/base/configuration/ConfigurationException.h
    include/lintel/features/base/configuration/DatabaseConnectionComponent.h
    include/lintel/features/base/configuration/DatabaseConnectionEntry.h
    include/lintel/features/base/configuration/Entry.h
    include/lintel/features/base/configuration/EnvironmentConfiguration.h
    include/lintel/features/base/configuration/LoggerComponent.h
    include/lintel/features/base/configuration/LoggerConfiguration.h
    include/lintel/features/base/configuration/LoggerEntry.h
    include/lintel/features/base/configuration/LoggerPathConfiguration.h
    include/lintel/features/base/configuration/LoggerSinkConfiguration.h
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
    include/lintel/features/base/models/ProcessName.h
    include/lintel/features/base/models/SharedMemorySegmentInfo.h
    include/lintel/features/base/models/User.h
    # features - base - msg
    include/lintel/features/base/msg/Message.h
    include/lintel/features/base/msg/MessageQueue.h
    include/lintel/features/base/msg/MessageQueueCore.h
    include/lintel/features/base/msg/MessageQueues.h
    # features - base - events
    include/lintel/features/base/events/Event.h
    include/lintel/features/base/events/EventBus.h
    include/lintel/features/base/events/BoostSegmentAllocator.h
    include/lintel/features/base/events/ShmSegmentAccessor.h
    # features - base - provider
    include/lintel/features/base/provider/GroupProvider.h
    # features - base - repositories
    include/lintel/features/base/repositories/GroupRepository.h
    include/lintel/features/base/repositories/HistoryRepository.h
    include/lintel/features/base/repositories/IMessageQueueRepository.h
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
    include/lintel/features/base/services/SharedMemorySegmentManager.h
    # features - cli
    include/lintel/features/cli/CommandLineFeature.h
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
    include/lintel/features/cli/components/HistoryCliComponent.h
    include/lintel/features/cli/components/MessageQueueCliComponent.h
    include/lintel/features/cli/components/ProcessCliComponent.h
    include/lintel/features/cli/components/SharedMemoryCliComponent.h
    include/lintel/features/cli/components/UserManagementCliComponent.h
    # features - common
    include/lintel/features/Feature.h
    include/lintel/features/Features.h
    # features - http
    include/lintel/features/http/HttpFeature.h
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
    include/lintel/features/http/service/HttpBadRequestException.h
    include/lintel/features/http/service/HttpUnauthorizedException.h
    include/lintel/features/http/service/HttpClientHelper.h
    include/lintel/features/http/service/Server.h
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
    include/lintel/features/property/PropertyFeature.h
    include/lintel/features/property/command_line/PropertyCliComponent.h
    include/lintel/features/property/configuration/PropertyComponent.h
    include/lintel/features/property/configuration/PropertyEntry.h
    include/lintel/features/property/configuration/PropertyRepositoryComponent.h
    include/lintel/features/property/configuration/PropertyRepositoryEntry.h
    include/lintel/features/property/controller/PropertiesDto.h
    include/lintel/features/property/controller/PropertyApi.h
    include/lintel/features/property/controller/PropertyController.h
    include/lintel/features/property/controller/PropertyDto.h
    include/lintel/features/property/controller/PropertyValueDto.h
    include/lintel/features/property/events/PropertyChange.h
    include/lintel/features/property/exceptions/PropertyNoRuntimeChangeSupported.h
    include/lintel/features/property/exceptions/PropertyNotFoundException.h
    include/lintel/features/property/factories/PropertyFactory.h
    include/lintel/features/property/models/DataStorage.h
    include/lintel/features/property/models/Property.h
    include/lintel/features/property/models/PropertyBase.h
    include/lintel/features/property/models/PropertyRepositoryType.h
    include/lintel/features/property/models/PropertyValueStorage.h
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
    core/StartupBuilder.cpp
    # core - models
    core/models/SharedMemorySegment.cpp
    # core - persistence
    core/persistence/Arguments.cpp
    core/persistence/Connection.cpp
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
    core/utils/ProcessResourceReader.cpp
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
    features/base/msg/MessageQueueCore.cpp
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
    features/http/service/HttpClientHelper.cpp
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

add_library(lintel ${headers} ${sources})
# Hypodermic includes: propagated via target_link_libraries for vendored target,
# explicit variable for find_package fallback
if (NOT TARGET lintel::Hypodermic)
    set(HYPODERMIC_INCLUDE_DIRS_VAR ${Hypodermic_INCLUDE_DIRS})
else ()
    set(HYPODERMIC_INCLUDE_DIRS_VAR "")
endif ()

target_compile_features(lintel PUBLIC cxx_std_17)
# Include directories: everything needed permanently is passed on as a linked target in
# target_link_libraries() further down. So only the include paths that are really needed
# just for building are listed here (Hypodermic from find_package, own headers). The
# *_INCLUDE_DIRS variables point into the Conan cache of the build machine and would be
# dead weight in the installed export - exactly what the old
# $<INSTALL_INTERFACE:${OpenSSL_INCLUDES};...> warns about. The include paths of the
# dependencies arrive via their targets in the consumer tree instead, which keeps the
# export relocatable. The include directory of the installed library itself is
# prefix-relative.
target_include_directories(lintel
        SYSTEM PUBLIC
        "$<BUILD_INTERFACE:${OpenSSL_INCLUDES};${magic_enum_INCLUDE_DIRS};${RapidJSON_INCLUDE_DIRS};${HYPODERMIC_INCLUDE_DIRS_VAR};${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

# magic_enum and rapidjson are part of the interface through public headers and are
# therefore linked as targets (rapidjson thereby replaces the previous
# ${rapidjson_LIBRARIES_DEBUG} that was resolved in Debug only and was empty in the
# release export).
# project_warnings is a pure build target of this project (CompilerWarnings.cmake). As a
# PRIVATE linked dependency of a static library it ends up as $<LINK_ONLY:project_warnings>
# in the export and would have to be exported along with it. The $<BUILD_INTERFACE:...>
# removes it from the install export but keeps the flags in the build tree - so
# project_warnings does not belong into the export.
target_link_libraries(lintel PUBLIC fmt::fmt-header-only tinyxml2::tinyxml2 PostgreSQL::PostgreSQL SQLite::SQLite3
        Threads::Threads OpenSSL::SSL OpenSSL::Crypto httplib::httplib rapidjson magic_enum::magic_enum date::date
        spdlog::spdlog_header_only lintel::Hypodermic Boost::boost Boost::headers Boost::filesystem
        PRIVATE $<BUILD_INTERFACE:project_warnings>)

# Boost.UUID formats UUIDs via SSE2 intrinsics. Without optimization (-O0) GCC 13 no longer
# inlines the _mm_slli_si128 intrinsic, but the backend call with a variable shift then
# produces a hard error ("the last argument must be an 8-bit immediate"). Use the generic
# Boost path in debug builds.
target_compile_definitions(lintel PRIVATE $<$<CONFIG:Debug>:BOOST_UUID_NO_SIMD>)

option(ENABLE_PCH "Enable precompiled headers" ON)
if(ENABLE_PCH)
    target_precompile_headers(lintel PRIVATE
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
        <fmt/base.h>
    )
endif()

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

# Only lintel is exported, project_warnings is not (see the comment at
# target_link_libraries). The include directory is already part of the
# $<INSTALL_INTERFACE:...> of the include directories, so no INCLUDES DESTINATION here.
install(TARGETS lintel
        EXPORT lintelTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/lintel
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# NAMESPACE lintel:: matches the alias the library is used with in the build
# tree - so the imported name is lintel::lintel.
install(EXPORT lintelTargets
        DESTINATION ${LINTEL_CMAKE_INSTALL_DIR}
        FILE lintelTargets.cmake
        NAMESPACE lintel::)

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

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lintelConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/lintelConfigVersion.cmake
        DESTINATION ${LINTEL_CMAKE_INSTALL_DIR})

# License and docs belong into the package as well - otherwise the license obligation does not travel with it.
install(FILES ${LINTEL_SOURCE_ROOT}/LICENSE
        ${LINTEL_SOURCE_ROOT}/README.md
        ${LINTEL_SOURCE_ROOT}/swagger.yaml
        DESTINATION ${LINTEL_DOCS_INSTALL_DIR})
