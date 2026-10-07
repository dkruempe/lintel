#include "lintel/core/plugins/SharedMemoryBootstrapPlugin.h"

#include <filesystem>
#include <map>
#include <stdexcept>
#include <string>
#include <system_error>
#include <tuple>
#include <utility>

#include <boost/interprocess/managed_mapped_file.hpp>

#include "lintel/core/models/BootstrapSequence.h"
#include "lintel/core/persistence/Connection.h"
#include "lintel/core/persistence/ParameterBuilder.h"
#include "lintel/core/persistence/PreparedStatement.h"
#include "lintel/core/persistence/Statement.h"
#include "lintel/core/persistence/Transaction.h"
#include "lintel/core/services/LoggerService.h"
#include "lintel/features/base/configuration/EventBusComponent.h"

namespace {
    /** Smallest file size a valid (even fully shrunk) boost managed mapped
     * file can have. Any file smaller than this cannot contain a valid boost
     * header, so dereferencing its structures (e.g. during a sanity check)
     * could crash instead of throwing. */
    constexpr std::size_t MIN_SEGMENT_FILE_SIZE = 288;
}  // namespace

SharedMemoryBootstrapPlugin::SharedMemoryBootstrapPlugin(
        const std::shared_ptr<DatabaseConnectionConfigurations> &
        connectionConfigurations,
        std::vector<std::shared_ptr<SharedMemoryRepository>>
        sharedMemoryRepositories,
        std::shared_ptr<SharedMemorySegmentManager> sharedMemorySegmentManager,
        std::shared_ptr<Configuration> configuration)
        : m_connectionEntry(connectionConfigurations->ofDefault()),
          m_sharedMemoryRepositories(std::move(sharedMemoryRepositories)),
          m_sharedMemorySegmentManager(
                  std::move(sharedMemorySegmentManager)),
          m_configuration(std::move(configuration)) {}

BootstrapSequence SharedMemoryBootstrapPlugin::getPriority() {
    return BootstrapSequence::SharedMemory;
}

void SharedMemoryBootstrapPlugin::validateSegments() {
    if (m_sharedMemorySegmentManager == nullptr) {
        return;
    }
    for (const auto &segment: m_sharedMemorySegmentManager->allOf()) {
        const std::filesystem::path &path = segment->getPath();
        std::error_code error;
        if (!std::filesystem::exists(path, error) || error) {
            continue;
        }
        const std::size_t fileSize = std::filesystem::file_size(path, error);
        if (fileSize < MIN_SEGMENT_FILE_SIZE) {
            const std::string message =
                    "shared memory segment '" + segment->getName()
                    + "' is corrupted: file '" + path.string()
                    + "' has only " + std::to_string(fileSize)
                    + " bytes, too small for a managed shared memory segment";
            LOG_ERROR("{}", message);
            throw std::runtime_error(message);
        }
        boost::interprocess::managed_mapped_file mapping(
                boost::interprocess::open_only, path.c_str());
        if (mapping.get_size() > fileSize) {
            const std::string message =
                    "shared memory segment '" + segment->getName()
                    + "' is truncated: file '" + path.string()
                    + "' has " + std::to_string(fileSize)
                    + " bytes but the header requires "
                    + std::to_string(mapping.get_size()) + " bytes";
            LOG_ERROR("{}", message);
            throw std::runtime_error(message);
        }
        if (!mapping.check_sanity()) {
            const std::string message =
                    "shared memory segment '" + segment->getName()
                    + "' is corrupted: sanity check failed on file '"
                    + path.string() + "'";
            LOG_ERROR("{}", message);
            throw std::runtime_error(message);
        }
    }
}

EventBusConfig SharedMemoryBootstrapPlugin::configOf(
        const std::shared_ptr<EventBusEntry> &entry) {
    EventBusConfig config;
    config.busCapacity = entry->getBusCapacity();
    config.subscriberCapacity = entry->getSubscriberCapacity();
    config.maxTopics = entry->getMaxTopics();
    config.maxSubscribers = entry->getMaxSubscribers();
    if (!config.isValid()) {
        const std::string message =
                "invalid event bus configuration for '" + entry->getName() + "'";
        LOG_ERROR("{}", message);
        throw std::runtime_error(message);
    }
    return config;
}

void SharedMemoryBootstrapPlugin::validateEventBuses() {
    if (m_sharedMemorySegmentManager == nullptr ||
        m_configuration == nullptr) {
        return;
    }
    const auto entries = m_configuration->configurationOf<EventBusComponent>();
    for (const auto &entry: entries) {
        auto eventBusEntry = std::static_pointer_cast<EventBusEntry>(entry);
        const EventBusConfig config = configOf(eventBusEntry);
        const std::string &segmentName = eventBusEntry->getSegment();
        const auto segment = m_sharedMemorySegmentManager->of(segmentName);
        if (segment == nullptr) {
            const std::string message =
                    "segment '" + segmentName + "' not found for event bus '"
                            + eventBusEntry->getName() + "'";
            LOG_ERROR("{}", message);
            throw std::runtime_error(message);
        }
        const std::filesystem::path &path = segment->getPath();
        std::error_code error;
        if (!std::filesystem::exists(path, error) || error) {
            continue;
        }
        boost::interprocess::managed_mapped_file mapping(
                boost::interprocess::open_only, path.c_str());
        const auto bus =
                mapping.find<EventBus>(eventBusEntry->getName().c_str());
        if (bus.first == nullptr) {
            continue;
        }
        const EventBusConfig persisted = bus.first->getConfig();
        if (persisted.busCapacity != config.busCapacity ||
            persisted.subscriberCapacity != config.subscriberCapacity ||
            persisted.maxTopics != config.maxTopics ||
            persisted.maxSubscribers != config.maxSubscribers) {
            const std::string message =
                    "event bus '" + eventBusEntry->getName()
                            + "' in segment '" + segmentName
                            + "' was created with a different configuration "
                              "(bus capacity "
                            + std::to_string(persisted.busCapacity)
                            + ", subscriber capacity "
                            + std::to_string(persisted.subscriberCapacity)
                            + ", max topics "
                            + std::to_string(persisted.maxTopics)
                            + ", max subscribers "
                            + std::to_string(persisted.maxSubscribers) + ")";
            LOG_ERROR("{}", message);
            throw std::runtime_error(message);
        }
    }
}

void SharedMemoryBootstrapPlugin::onStart() {
    validateSegments();
    validateEventBuses();
    if (m_connectionEntry == nullptr) {
        LOG_INFO("no default database connection - skip shared memory database "
                 "bookmark");
        return;
    }
    std::map<std::string,
            std::shared_ptr<SharedMemoryRepository>>
            temp;

    for (const auto &sharedMemoryRepository: m_sharedMemoryRepositories) {
        temp.insert(
                {sharedMemoryRepository->getUuid(),
                 sharedMemoryRepository});
    }
    db::Connection const connection(m_connectionEntry);
    db::Transaction transaction(connection);
    db::Statement statement(connection);
    auto result = statement.execute(R"(
  select uuid,
         shared_memory_segment,
         shared_memory_type,
         shared_memory_repository,
         current_version,
         current_data_size
  from shared_memory_repositories)");
    std::map<std::string, int32_t> resTemp;
    for (const auto &iter: result) {
        auto uuid = iter.of(0).getValue<std::string>();
        resTemp.insert({uuid, -1});
        try {
            auto currentVersion = iter.of(4).getValue<int32_t>();
            auto currentDataSize = iter.of(5).getValue<std::size_t>();
            const auto &repository = temp.at({uuid});
            if (currentVersion != repository->getCodeVersion()) {
                // ERROR
                throw std::runtime_error(
                        uuid
                                .append(": currentVersion(")
                                .append(std::to_string(currentVersion))
                                .append(") != ")
                                .append("codeVersion(")
                                .append(std::to_string(repository->getCodeVersion()))
                                .append(")"));
            }
            if (currentDataSize != repository->getSizeOfData()) {
                // ERROR
                throw std::runtime_error(
                        uuid
                                .append(": currentDataSize(")
                                .append(std::to_string(currentDataSize))
                                .append(") != ")
                                .append("sizeOfData(")
                                .append(std::to_string(repository->getSizeOfData()))
                                .append(")"));
            }
        } catch (std::out_of_range &exception) {
            LOG_WARN("{} found with not SharedMemoryRepository => delete ?", uuid);
        }
    }
    for (const auto &sharedMemoryRepository: m_sharedMemoryRepositories) {
        auto found = resTemp.find(sharedMemoryRepository->getUuid());
        if (found == resTemp.end()) {
            db::ParameterBuilder builder(m_connectionEntry);
            builder.add<std::string>(sharedMemoryRepository->getUuid());
            builder.add<std::string>(
                    sharedMemoryRepository->getSharedMemorySegment()->getName());
            builder.add<std::string>(std::string(sharedMemoryRepository->getTypeName()));
            builder.add<std::string>(
                    std::string(sharedMemoryRepository->getSharedMemoryRepository()));
            builder.add<int32_t>(sharedMemoryRepository->getCodeVersion());
            builder.add<std::size_t>(sharedMemoryRepository->getSizeOfData());
            statement.execute(R"(
      insert into shared_memory_repositories (
        uuid,
        shared_memory_segment,
        shared_memory_type,
        shared_memory_repository,
        current_version,
        current_data_size)
      values(?, ?, ?, ?, ?, ?)
      )",
                              builder);
        }
    }
    transaction.commit();
}
