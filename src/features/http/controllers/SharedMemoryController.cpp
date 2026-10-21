#include "lintel/features/http/controllers/SharedMemoryController.h"

#include <utility>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "lintel/core/exceptions/ShmSegmentNotFound.h"
#include "lintel/core/utils/MemorySize.h"
#include "lintel/core/utils/RegexUtils.h"
#include "lintel/features/base/controller/SharedMemoryRepositoriesDto.h"
#include "lintel/features/base/controller/SharedMemorySegmentDto.h"
#include "lintel/features/base/controller/SharedMemorySegmentsDto.h"
#include "lintel/features/base/models/SharedMemorySegmentStats.h"

namespace lintel {
namespace {
  /** Serialize the statistics of one segment.
   *  @param stats statistics to write
   *  @return JSON object with the uppercase field names used by the other DTOs */
  std::string shmStatsJsonOf(const SharedMemorySegmentStats &stats)
  {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer writer(buffer);
    writer.StartObject();
    writer.String("NAME");
    writer.String(stats.name.c_str());
    writer.String("ALLOCATED_BYTES");
    writer.Uint64(stats.allocatedBytes);
    writer.String("CAPACITY_BYTES");
    writer.Uint64(stats.capacityBytes);
    writer.String("FREE_BYTES");
    writer.Uint64(stats.freeBytes);
    writer.String("GROW_COUNT");
    writer.Uint64(stats.growCount);
    writer.String("SHRINK_COUNT");
    writer.Uint64(stats.shrinkCount);
    writer.String("RECOVERY_COUNT");
    writer.Uint64(stats.recoveryCount);
    writer.String("GENERATION");
    writer.Uint64(stats.generation);
    writer.EndObject();
    return std::string(buffer.GetString());
  }

  /** Serialize the counts of one cleanup run.
   *  @param report report of scan() or apply()
   *  @return JSON object with the uppercase field names used by the other DTOs */
  std::string shmCleanupJsonOf(const SharedMemoryCleanupReport &report)
  {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer writer(buffer);
    writer.StartObject();
    writer.String("DRY_RUN");
    writer.Bool(report.dryRun);
    writer.String("SCANNED");
    writer.Uint64(report.scanned);
    writer.String("ORPHANS");
    writer.Uint64(report.orphanCount());
    writer.String("REMOVED");
    writer.Uint64(report.removed.size());
    writer.String("FAILED");
    writer.Uint64(report.failed);
    writer.EndObject();
    return std::string(buffer.GetString());
  }
}// namespace

SharedMemoryController::SharedMemoryController(const std::shared_ptr<IAuthService> &authServicie,
  std::shared_ptr<ISharedMemoryService> sharedMemoryService,
  std::shared_ptr<ISharedMemorySegmentManager> sharedMemorySegmentManager,
  std::vector<std::shared_ptr<SharedMemoryRepository>> sharedMemoryRepositories,
  std::shared_ptr<SharedMemoryCleanupService> sharedMemoryCleanupService)
  : Controller(authServicie), m_sharedMemoryService(std::move(sharedMemoryService)),
    m_sharedMemorySegmentManager(std::move(sharedMemorySegmentManager)),
    m_sharedMemoryCleanupService(std::move(sharedMemoryCleanupService)),
    m_sharedMemoryRepositories(std::move(sharedMemoryRepositories)),
    m_uuidSharedMemoryRepositories(build(m_sharedMemoryRepositories)), m_adminGroup("Admin-Shm", {}, true),
    m_userGroup("User-Shm", {}, true)
{
  add(m_adminGroup);
  add(m_userGroup);
}

bool SharedMemoryController::canRead(const std::optional<UserToken> &user) const
{
  return user.has_value() && (user->m_user.has(m_userGroup) || user->m_user.has(m_adminGroup));
}

bool SharedMemoryController::canAdminister(const std::optional<UserToken> &user) const
{
  return user.has_value() && user->m_user.has(m_adminGroup);
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

void SharedMemoryController::statsOfGet(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!canRead(user)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType != ContentType::ApplicationJson) {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    return;
  }
  const std::string segmentName = request.matches[1];
  const std::shared_ptr<SharedMemorySegment> segment = m_sharedMemorySegmentManager->of(segmentName);
  if (segment == nullptr) {
    LOG_WARN("statsOf: segment {} not found", segmentName);
    response.status = HttpStatusCodes::NotFound;
    response.set_content("", contentType.getName());
    return;
  }
  SharedMemorySegmentStats stats;
  try {
    stats = m_sharedMemoryService->statsOf(segment);
  } catch (const ShmSegmentNotFound &error) {
    // the segment manager knows the segment, but this process does not map it
    LOG_WARN("statsOf: {}", error.what());
    response.status = HttpStatusCodes::NotFound;
    response.set_content("", contentType.getName());
    return;
  }
  response.set_content(shmStatsJsonOf(stats), contentType.getName());
}

void SharedMemoryController::cleanupOfPost(const httplib::Request & /*request*/,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value()) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  // cleanup removes objects, so only administrators may run it
  if (!canAdminister(user)) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType != ContentType::ApplicationJson) {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    return;
  }
  if (m_sharedMemoryCleanupService == nullptr) {
    LOG_ERROR("cleanupOf: no cleanup service is registered");
    response.status = HttpStatusCodes::InternalServerError;
    response.set_content("", contentType.getName());
    return;
  }
  const SharedMemoryCleanupReport report = m_sharedMemoryCleanupService->apply();
  LOG_INFO("cleanupOf: scanned {} entries, removed {} orphans, {} failures",
    report.scanned,
    report.removed.size(),
    report.failed);
  response.set_content(shmCleanupJsonOf(report), contentType.getName());
}
}// namespace lintel
