#include "base_library/core/plugins/SharedMemoryBootstrapPlugin.h"
#include "base_library/core/persistence/ParameterBuilder.h"
#include "base_library/core/models/BootstrapSequence.h"

#include <tuple>
#include <map>
#include <string>
#include <utility>

#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/PreparedStatement.h"
#include "base_library/core/persistence/Statement.h"
#include "base_library/core/persistence/Transaction.h"
#include "base_library/core/services/LoggerService.h"

SharedMemoryBootstrapPlugin::SharedMemoryBootstrapPlugin(
        const std::shared_ptr<DatabaseConnectionConfigurations> &
        connectionConfigurations,
        std::vector<std::shared_ptr<SharedMemoryRepository>>
        sharedMemoryRepositories)
        : m_connectionEntry(connectionConfigurations->ofDefault()),
          m_sharedMemoryRepositories(std::move(sharedMemoryRepositories)) {}

BootstrapSequence SharedMemoryBootstrapPlugin::getPriority() {
    return BootstrapSequence::SharedMemory;
}

void SharedMemoryBootstrapPlugin::onStart() {
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
            builder.add<std::string>(sharedMemoryRepository->getTypeName());
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
