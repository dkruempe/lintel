#include "base_library/features/base/controller/SharedMemoryController.h"

#include <base_library/features/base/controller/SharedMemorySegmentsDto.h>

#include <utility>

#include "base_library/features/base/controller/SharedMemorySegmentDto.h"

SharedMemoryController::SharedMemoryController(
    const std::shared_ptr<AuthService>& authServicie,
    std::shared_ptr<SharedMemoryService> sharedMemoryService,
    std::shared_ptr<SharedMemorySegmentManager> sharedMemorySegmentManager)
    : Controller(authServicie),
      m_sharedMemoryService(std::move(sharedMemoryService)),
      m_sharedMemorySegmentManager(std::move(sharedMemorySegmentManager)),
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
  // TODO variables
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
