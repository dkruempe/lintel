#include "lintel/features/base/services/SharedMemoryCleanupService.h"

#include <bit>
#include <cstdint>
#include <cstring>
#include <exception>
#include <stdexcept>

#include "lintel/core/persistence/Connection.h"
#include "lintel/core/persistence/ParameterBuilder.h"
#include "lintel/core/persistence/Statement.h"
#include "lintel/core/shm/ShmObjectHeader.h"
#include "lintel/features/base/services/SchedulerService.h"
#include "lintel/features/base/services/SharedMemoryService.h"
#include "lintel/features/base/services/ShmConstructors.h"

namespace lintel {
namespace shm {

  bool destroyNamedObject(SharedMemoryService &service,
    const std::shared_ptr<SharedMemorySegment> &segment,
    const std::string &name)
  {
    if (segment == nullptr) { return false; }
    std::lock_guard<std::mutex> lock(service.segmentsMutexOf());
    auto &handle = service.segmentOf(segment->getName());
    SegmentSemaphoreGuard gate(handle);
    // The byte-wise lookup works for every object size. An object with a
    // compatible header keeps its payload in a separate allocation: release it,
    // then destroy the named header. trackObject<T>() restricts the tracking to
    // trivially destructible types, for which running the payload destructor is
    // a no-op, so none is run here.
    const auto named = handle.m_managedMappedFile->find<char>(name.c_str());
    if (named.first == nullptr) { return true; }// already gone: idempotent
    if (named.second >= sizeof(shm::ShmObjectHeader)) {
      // Read the header as bytes first: a pre-header object is not guaranteed to
      // be aligned like a ShmObjectHeader. Only after the magic proved that the
      // block is a header does the address become one.
      shm::ShmObjectHeader probe{};
      std::memcpy(&probe, named.first, sizeof(probe));
      if (probe.isCompatible()) {
        auto *header = std::bit_cast<shm::ShmObjectHeader *>(named.first);
        if (header->isReady() && header->payloadOffset != 0 && header->payloadBytes != 0) {
          void *payload = shm::payloadAddressOf(header);
          // Stop readers before the payload disappears.
          header->state = shm::ShmObjectHeader::kStateConstructing;
          handle.m_managedMappedFile->get_segment_manager()->deallocate(payload);
        }
        // The header is trivially destructible, so destroying it runs no payload
        // destructor; the segment manager releases the block and the name.
        handle.m_managedMappedFile->destroy<shm::ShmObjectHeader>(name.c_str());
        return true;
      }
    }
    // Pre-header object: destroy_ptr() on the char pointer keeps working for any
    // size, and the segment manager releases the block itself.
    handle.m_managedMappedFile->destroy_ptr(named.first);
    return true;
  }

}// namespace shm

SharedMemoryCleanupService::SharedMemoryCleanupService(const std::shared_ptr<SchedulerService> &scheduler,
  std::shared_ptr<SharedMemoryService> sharedMemoryService,
  std::shared_ptr<DatabaseConnectionEntry> databaseConnection,
  std::chrono::seconds interval)
  : PropertyRegistration(
      sharedMemoryService == nullptr ? std::string("shared-memory-cleanup") : sharedMemoryService->getProcessName()),
    m_scheduler(scheduler), m_sharedMemoryService(std::move(sharedMemoryService)),
    m_databaseConnection(std::move(databaseConnection)), m_interval(interval)
{}

void SharedMemoryCleanupService::onInitialize()
{
  if (m_scheduler == nullptr || m_interval.count() <= 0) { return; }
  std::weak_ptr<SharedMemoryCleanupService> weakSelf = weak_from_this();
  m_scheduler->schedule_at_fixed_rate(m_interval, m_interval, [weakSelf]() {
    if (const std::shared_ptr<SharedMemoryCleanupService> self = weakSelf.lock()) {
      if (self->m_applyScheduled) {
        static_cast<void>(self->apply());
      } else {
        static_cast<void>(self->scan());
      }
    }
  });
}

SharedMemoryCleanupReport SharedMemoryCleanupService::scan() { return run(false); }

SharedMemoryCleanupReport SharedMemoryCleanupService::apply() { return run(true); }

std::chrono::seconds SharedMemoryCleanupService::interval() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_interval;
}

void SharedMemoryCleanupService::setInterval(std::chrono::seconds interval)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_interval = interval;
}

bool SharedMemoryCleanupService::applyScheduled() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_applyScheduled;
}

void SharedMemoryCleanupService::setApplyScheduled(bool apply)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_applyScheduled = apply;
}

std::string SharedMemoryCleanupService::keyOf(const std::string &segmentName, const std::string &objectName)
{
  return segmentName + '\0' + objectName;
}

bool SharedMemoryCleanupService::trackObject(const std::string &segmentName,
  const std::string &objectName,
  const ProcessIdentity &owner,
  RemoveFunction remove,
  std::chrono::system_clock::time_point lastHeartbeat)
{
  if (segmentName.empty() || objectName.empty() || !remove) { return false; }
  Entry entry;
  entry.segmentName = segmentName;
  entry.objectName = objectName;
  entry.owner = owner;
  entry.lastHeartbeat = lastHeartbeat;
  entry.remove = std::move(remove);
  std::lock_guard<std::mutex> lock(m_mutex);
  m_entries.insert_or_assign(keyOf(segmentName, objectName), std::move(entry));
  return true;
}

bool SharedMemoryCleanupService::touchObject(const std::string &segmentName,
  const std::string &objectName,
  std::chrono::system_clock::time_point lastHeartbeat)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  auto found = m_entries.find(keyOf(segmentName, objectName));
  if (found == m_entries.end()) { return false; }
  found->second.lastHeartbeat = lastHeartbeat;
  return true;
}

bool SharedMemoryCleanupService::untrackObject(const std::string &segmentName, const std::string &objectName)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_entries.erase(keyOf(segmentName, objectName)) > 0;
}

std::size_t SharedMemoryCleanupService::size() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_entries.size();
}

SharedMemoryCleanupStats SharedMemoryCleanupService::stats() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_stats;
}

bool SharedMemoryCleanupService::isOrphaned(const Entry &entry, SharedMemoryCleanupObject &orphan)
{
  // A recorded process identity is authoritative. A dead owner is an orphan; a
  // live one is never touched, no matter how old its heartbeat looks. The start
  // ticks make a recycled pid count as dead instead of alive.
  if (entry.owner.processId > 0 && processIdentityIsAlive(entry.owner)) { return false; }
  orphan.segmentName = entry.segmentName;
  orphan.objectName = entry.objectName;
  orphan.owner = entry.owner;
  orphan.lastHeartbeat = entry.lastHeartbeat;
  if (entry.owner.processId <= 0) {
    orphan.reason = "no owner process recorded";
  } else {
    orphan.reason = "owner process " + std::to_string(entry.owner.processId) + " is gone";
  }
  return true;
}

SharedMemoryCleanupReport SharedMemoryCleanupService::run(bool apply)
{
  std::vector<Entry> entries;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    entries.reserve(m_entries.size());
    for (const auto &entry : m_entries) { entries.push_back(entry.second); }
    if (apply) {
      ++m_stats.applies;
    } else {
      ++m_stats.scans;
    }
    m_stats.lastRun = std::chrono::system_clock::now();
  }

  SharedMemoryCleanupReport report;
  report.dryRun = !apply;
  report.scanned = entries.size();
  for (const Entry &entry : entries) {
    SharedMemoryCleanupObject orphan;
    if (!isOrphaned(entry, orphan)) { continue; }
    report.orphans.push_back(orphan);
    if (!apply) { continue; }

    bool removed = false;
    try {
      removed = entry.remove();
    } catch (const std::exception &error) {
      LOG_WARN("shared memory cleanup: removing orphan '{}' from segment '{}' failed: {}",
        orphan.objectName,
        orphan.segmentName,
        error.what());
    } catch (...) {
      LOG_WARN(
        "shared memory cleanup: removing orphan '{}' from segment '{}' failed", orphan.objectName, orphan.segmentName);
    }
    if (!removed) {
      ++report.failed;
      continue;
    }
    report.removed.push_back(orphan);
    // Drop the entry before the database reconciliation: the segment no longer
    // contains the object, so a retry could only report a phantom orphan.
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_entries.erase(keyOf(orphan.segmentName, orphan.objectName));
      ++m_stats.removed;
    }
    reconcileDatabase(orphan);
  }

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats.orphans += report.orphans.size();
    m_stats.failures += report.failed;
  }
  return report;
}

void SharedMemoryCleanupService::reconcileDatabase(const SharedMemoryCleanupObject &object) noexcept
{
  if (m_databaseConnection == nullptr) { return; }
  try {
    db::Connection connection(m_databaseConnection);
    db::Statement statement(connection);
    db::ParameterBuilder builder(m_databaseConnection);
    builder.add<std::string>(object.objectName).add<std::string>(object.objectName);
    statement.execute(R"(
      delete from shared_memory_repositories
      where uuid = ? or shared_memory_repository = ?
      )",
      builder);
  } catch (const std::exception &error) {
    // The segment object is already gone; a stale database row is a reporting
    // problem, not a reason to report the removal as failed.
    LOG_WARN("shared memory cleanup: reconciling the database row of '{}' failed: {}", object.objectName, error.what());
  }
}

}// namespace lintel
