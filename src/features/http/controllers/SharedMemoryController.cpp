#include "lintel/features/http/controllers/SharedMemoryController.h"

#include <utility>

#include "lintel/core/utils/MemorySize.h"
#include "lintel/core/utils/RegexUtils.h"
#include "lintel/features/base/controller/SharedMemoryRepositoriesDto.h"
#include "lintel/features/base/controller/SharedMemorySegmentDto.h"
#include "lintel/features/base/controller/SharedMemorySegmentsDto.h"

SharedMemoryController::SharedMemoryController(const std::shared_ptr<IAuthService> &authServicie,
  std::shared_ptr<ISharedMemoryService> sharedMemoryService,
  std::shared_ptr<ISharedMemorySegmentManager> sharedMemorySegmentManager,
  std::vector<std::shared_ptr<SharedMemoryRepository>> sharedMemoryRepositories)
  : Controller(authServicie), m_sharedMemoryService(std::move(sharedMemoryService)),
    m_sharedMemorySegmentManager(std::move(sharedMemorySegmentManager)),
    m_sharedMemoryRepositories(std::move(sharedMemoryRepositories)),
    m_uuidSharedMemoryRepositories(build(m_sharedMemoryRepositories)), m_adminGroup("Admin-Shm", {}, true),
    m_userGroup("User-Shm", {}, true)
{
  add(m_adminGroup);
  add(m_userGroup);
}

std::map<std::string, std::shared_ptr<SharedMemoryRepository>> SharedMemoryController::build(
  const std::vector<std::shared_ptr<SharedMemoryRepository>> &repositories)
{
  std::map<std::string, std::shared_ptr<SharedMemoryRepository>> map;
  for (const auto &item : repositories) { map.insert({ item->getUuid(), item }); }
  return map;
}

void SharedMemoryController::exportRepositoryOfGet(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_userGroup) && !user->m_user.has(m_adminGroup)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  LOG_TRACE("Called export");
  const std::string uuid = request.matches[1];
  if (contentType == ContentType::ApplicationJson) {
    LOG_TRACE("start searching repository {}", uuid);
    auto found = m_uuidSharedMemoryRepositories.find(uuid);
    if (found != m_uuidSharedMemoryRepositories.end()) {
      response.set_content(found->second->serialize(), contentType.getName());
    } else {
      LOG_WARN("exportRepositoryOfGet: repository {} not found", uuid);
      response.status = HttpStatusCodes::NotFound;
      response.set_content("", contentType.getName());
    }
  } else {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
  }
}

void SharedMemoryController::allSegmentsOfGet(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  // no user loggedin => Unauthorized
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_userGroup) && !user->m_user.has(m_adminGroup)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  const std::string segmentName = request.matches[1];
  if (contentType == ContentType::ApplicationJson) {
    auto segments = m_sharedMemorySegmentManager->allOf(segmentName);
    std::vector<SharedMemorySegmentDto> vec;
    for (const auto &segment : segments) {
      SharedMemorySegmentDto dto(m_sharedMemoryService->showStateOf(segment));
      vec.push_back(dto);
    }
    LOG_TRACE("segments {}", segments.size());
    SharedMemorySegmentsDto segmentsDto(vec);
    response.set_content(segmentsDto.JsonSerializable::serialize(), contentType.getName());
  } else {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
  }
}

void SharedMemoryController::allRepositoriesOfGet(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  // no user loggedin => Unauthorized
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_userGroup) && !user->m_user.has(m_adminGroup)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  const std::string repositoryName = request.matches[1];
  const std::string segmentName = request.matches[2];
  if (contentType == ContentType::ApplicationJson) {
    std::string errorMessage;
    if (!RegexUtils::validatePattern(repositoryName, errorMessage)
        || !RegexUtils::validatePattern(segmentName, errorMessage)) {
      LOG_WARN("allRepositoriesOfGet: {}", errorMessage);
      response.status = HttpStatusCodes::BadRequest;
      response.set_content("", contentType.getName());
      return;
    }
    const std::regex repositoryRegex(repositoryName);
    const std::regex segmentRegex(segmentName);
    LOG_TRACE("segment {} repository {}", segmentName, repositoryName);
    std::vector<SharedMemoryRepositoryDto> tmp;
    for (const auto &iter : m_sharedMemoryRepositories) {
      if (!std::regex_match(iter->getSharedMemorySegment()->getName(), segmentRegex)) { continue; }
      if (!std::regex_match(iter->getSharedMemoryRepository(), repositoryRegex)) { continue; }
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
    response.set_content(repositoriesDto.JsonSerializable::serialize(), contentType.getName());
  } else {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
  }
}

void SharedMemoryController::shrinkSegmentOfPut(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  // no user loggedin => Unauthorized
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_userGroup) && !user->m_user.has(m_adminGroup)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  const std::string segmentName = request.matches[1];
  if (contentType == ContentType::ApplicationJson) {
    auto segment = m_sharedMemorySegmentManager->of(segmentName);
    if (segment == nullptr) {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName());
      return;
    }
    m_sharedMemoryService->shrinkOf(segment);
  } else {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
  }
}

void SharedMemoryController::growSegmentOfPut(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  // no user loggedin => Unauthorized
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (!user->m_user.has(m_userGroup) && !user->m_user.has(m_adminGroup)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  const std::string segmentName = request.matches[1];
  const std::string sizeStr = request.matches[2];
  if (contentType == ContentType::ApplicationJson) {
    std::size_t growSize = 0;
    try {
      growSize = MemorySize::deserialize(sizeStr);
    } catch (const std::invalid_argument &exception) {
      LOG_WARN("growSegmentOfPut: {}", exception.what());
      response.status = HttpStatusCodes::BadRequest;
      response.set_content("", contentType.getName());
      return;
    }
    auto segment = m_sharedMemorySegmentManager->of(segmentName);
    if (segment == nullptr) {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName());
      return;
    }
    m_sharedMemoryService->growOf(segment, growSize);
  } else {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
  }
}
