#include "base_library/features/base/controller/SharedMemoryRepsoitoriesDto.h"

SharedMemoryRepositoriesDto::SharedMemoryRepositoriesDto(
        std::vector<SharedMemoryRepositoryDto> repositories)
        : m_repositories(std::move(repositories)) {}

const std::vector<SharedMemoryRepositoryDto> &
SharedMemoryRepositoriesDto::getRepositories() const {
    return m_repositories;
}

void SharedMemoryRepositoriesDto::serialize(
        rapidjson::Writer<rapidjson::StringBuffer> *writer) const {
    writer->StartArray();
    for (const auto &repository: m_repositories) {
        repository.serialize(writer);
    }
    writer->EndArray();
}

void SharedMemoryRepositoriesDto::deserialize(const std::string &json) {
    rapidjson::Document document;
    document.Parse(json.c_str());
    if (!document.IsArray()) {
        return;
    }
    for (const auto &item: document.GetArray()) {
        SharedMemoryRepositoryDto sharedMemoryRepositoryDto;
        sharedMemoryRepositoryDto.deserialize(item);
        m_repositories.push_back(sharedMemoryRepositoryDto);
    }
}