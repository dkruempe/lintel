#include "base_library/core/plugins/SharedMemoryBootstrapPlugin.h"

#include <tuple>

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
    std::map<std::tuple<std::string, std::string, std::string>,
            std::shared_ptr<SharedMemoryRepository>>
            temp;

    for (const auto &sharedMemoryRepository: m_sharedMemoryRepositories) {
        temp.insert(
                {{sharedMemoryRepository->getSharedMemorySegment()->getName(),
                  sharedMemoryRepository->getTypeName(),
                  std::string(sharedMemoryRepository->getSharedMemoryRepository())},
                 sharedMemoryRepository});
    }
    db::Connection connection(m_connectionEntry);
    db::Transaction transaction(connection);
    db::Statement statement(connection);
    auto result = statement.execute(R"(
  select shared_memory_segment,
         shared_memory_type,
         shared_memory_repository,
         current_version,
         current_data_size
  from shared_memory_repositories)");
    std::map<std::tuple<std::string, std::string, std::string>, int32_t> resTemp;
    for (const auto &iter: result) {
        std::string segmentName = iter.of(0).getValue();
        std::string type = iter.of(1).getValue();
        std::string repositoryName = iter.of(2).getValue();
        resTemp.insert({{segmentName, type, repositoryName}, -1});
        try {
            auto currentVersion = iter.of(3).getValue<int32_t>();
            auto currentDataSize = iter.of(4).getValue<std::size_t>();
            const auto &repository = temp.at({segmentName, type, repositoryName});
            if (currentVersion != repository->getCodeVersion()) {
                // ERROR
                throw std::runtime_error(
                        segmentName.append("/")
                                .append(segmentName)
                                .append("/")
                                .append(type)
                                .append("/")
                                .append(repositoryName)
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
                        segmentName.append("/")
                                .append(segmentName)
                                .append("/")
                                .append(type)
                                .append("/")
                                .append(repositoryName)
                                .append(": currentDataSize(")
                                .append(std::to_string(currentDataSize))
                                .append(") != ")
                                .append("sizeOfData(")
                                .append(std::to_string(repository->getSizeOfData()))
                                .append(")"));
            }
        } catch (std::out_of_range &exception) {
            LOG_ERROR("{}/{}/{} found with not SharedMemoryRepository => delete",
                      segmentName, type, repositoryName);
            db::ParameterBuilder builder(m_connectionEntry);
            builder.add(segmentName);
            builder.add(type);
            builder.add(repositoryName);
            statement.execute(R"(
      delete from shared_memory_repositories
      where shared_memory_segment = ?
        and shared_memory_repository = ?
        and shared_memory_type = ?)",
                              builder);
        };
    }
    for (const auto &sharedMemoryRepository: m_sharedMemoryRepositories) {
        auto found = resTemp.find(
                {sharedMemoryRepository->getSharedMemorySegment()->getName(),
                 sharedMemoryRepository->getTypeName(),
                 std::string(sharedMemoryRepository->getSharedMemoryRepository())});
        if (found == resTemp.end()) {
            db::ParameterBuilder builder(m_connectionEntry);
            builder.add<std::string>(
                    sharedMemoryRepository->getSharedMemorySegment()->getName());
            builder.add<std::string>(sharedMemoryRepository->getTypeName());
            builder.add<std::string>(
                    std::string(sharedMemoryRepository->getSharedMemoryRepository()));
            builder.add<int32_t>(sharedMemoryRepository->getCodeVersion());
            builder.add<std::size_t>(sharedMemoryRepository->getSizeOfData());
            statement.execute(R"(
      insert into shared_memory_repositories (
        shared_memory_segment,
        shared_memory_type,
        shared_memory_repository,
        current_version,
        current_data_size)
      values(?, ?, ?, ?, ?)
      )",
                              builder);
        }
    }
}
