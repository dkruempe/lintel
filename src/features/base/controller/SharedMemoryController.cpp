#include "base_library/features/base/controller/SharedMemoryController.h"

#include <utility>

#include "base_library/features/base/controller/SharedMemoryRepsoitoriesDto.h"
#include "base_library/features/base/controller/SharedMemorySegmentDto.h"
#include "base_library/features/base/controller/SharedMemorySegmentsDto.h"

SharedMemoryController::SharedMemoryController(
    const std::shared_ptr<AuthService>& authServicie,
    std::shared_ptr<SharedMemoryService> sharedMemoryService,
    std::shared_ptr<SharedMemorySegmentManager> sharedMemorySegmentManager,
    std::vector<std::shared_ptr<SharedMemoryRepository>>
        sharedMemoryRepositories)
    : Controller(authServicie),
      m_sharedMemoryService(std::move(sharedMemoryService)),
      m_sharedMemorySegmentManager(std::move(sharedMemorySegmentManager)),
      m_sharedMemoryRepositories(std::move(sharedMemoryRepositories)),
      m_adminGroup("Admin-Shm", {}, true),
      m_userGroup("User-Shm", {}, true) {
  add(m_adminGroup);
  add(m_userGroup);
}

void SharedMemoryController::allSegmentsOfGet(
    const httplib::Request& request, httplib::Response& response,
    const ContentType& contentType, const std::optional<UserToken>& user) {
  // no user loggedin => Unauthorized
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  if (!user->m_user.has(m_userGroup) && !user->m_user.has(m_adminGroup)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  const std::string segmentName = request.matches[1];
  switch (contentType) {
    case ContentType::ApplicationJson: {
      auto segments = m_sharedMemorySegmentManager->allOf(segmentName);
      std::vector<SharedMemorySegmentDto> vec;
      for (const auto& segment : segments) {
        SharedMemorySegmentDto dto(m_sharedMemoryService->showStateOf(segment));
        vec.push_back(dto);
      }
      LOG_TRACE("segments {}", segments.size());
      SharedMemorySegmentsDto segmentsDto(vec);
      response.set_content(segmentsDto.JsonSerializable::serialize(),
                           contentType.getName().c_str());
      break;
    }
    default: {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      break;
    }
  }
}
void SharedMemoryController::allRepositoriesOfGet(
    const httplib::Request& request, httplib::Response& response,
    const ContentType& contentType, const std::optional<UserToken>& user) {
  // no user loggedin => Unauthorized
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  if (!user->m_user.has(m_userGroup) && !user->m_user.has(m_adminGroup)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  const std::string repositoryName = request.matches[1];
  const std::string segmentName = request.matches[2];
  switch (contentType) {
    case ContentType::ApplicationJson: {
      std::regex repositoryRegex(repositoryName);
      std::regex segmentRegex(segmentName);
      LOG_TRACE("segment {} repository {}", segmentName, repositoryName);
      std::vector<SharedMemoryRepositoryDto> tmp;
      for (const auto& iter : m_sharedMemoryRepositories) {
        if (!std::regex_match(iter->getSharedMemorySegment()->getName(),
                              segmentRegex)) {
          continue;
        }
        if (!std::regex_match(iter->getSharedMemoryRepository(),
                              repositoryRegex)) {
          continue;
        }
        LOG_TRACE("repository {}", iter->getSharedMemoryRepository());
        SharedMemoryRepositoryDto dto(*iter);
        tmp.push_back(dto);
      }
      /*
       * SharedMemoryRepository:
       * - Type of SharedMemoryRepository (Map, Array, Vector, Set, Object)
       * - Name of Repository
       * - Size of Repository (in map case size of value)
       * - Current Version of Repository
       * - Used Segment of Repository
       */
      LOG_TRACE("segments {}", tmp.size());
      SharedMemoryRepositoriesDto repositoriesDto(tmp);
      response.set_content(repositoriesDto.JsonSerializable::serialize(),
                           contentType.getName().c_str());
      break;
    }
    default: {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      break;
    }
  }
}
void SharedMemoryController::shrinkSegmentOfPut(
    const httplib::Request& request, httplib::Response& response,
    const ContentType& contentType, const std::optional<UserToken>& user) {
  // no user loggedin => Unauthorized
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  if (!user->m_user.has(m_userGroup) && !user->m_user.has(m_adminGroup)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName().c_str());
    return;
  }
  const std::string segmentName = request.matches[1];
  switch (contentType) {
    case ContentType::ApplicationJson: {
      auto segment = m_sharedMemorySegmentManager->of(segmentName);
      if (segment == nullptr) {
        response.status = HttpStatusCodes::Forbidden;
        response.set_content("", contentType.getName().c_str());
        return;
      }
      m_sharedMemoryService->shrinkOf(segment);
      break;
    }
    default: {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      break;
    }
  }
}
