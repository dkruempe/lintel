#include "lintel/core/shm/ShmObject.h"

#include "lintel/core/models/SharedMemorySegment.h"
#include "lintel/core/services/LoggerService.h"
#include "lintel/core/shm/BoostSegmentAllocator.h"
#include "lintel/core/shm/ShmSegmentAccessor.h"
#include "lintel/features/base/services/SharedMemoryService.h"
#include "lintel/features/base/services/ShmConstructors.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace lintel {
namespace shm {
  namespace {

    /** Process wide migration registry state. */
    struct MigrationRegistry
    {
      std::mutex mutex;
      std::map<std::uint32_t, std::vector<ShmPayloadMigrationStep>> steps;
    };

    /** @return the registry of this process */
    MigrationRegistry &migrationRegistry()
    {
      static MigrationRegistry instance;
      return instance;
    }

    /** @param typeId payload type
     *  @return a copy of the registered steps, so the callback cannot deadlock
     *          on the registry mutex if it registers another migration */
    std::vector<ShmPayloadMigrationStep> stepsOf(std::uint32_t typeId)
    {
      MigrationRegistry &registry = migrationRegistry();
      std::lock_guard<std::mutex> lock(registry.mutex);
      const auto found = registry.steps.find(typeId);
      return found == registry.steps.end() ? std::vector<ShmPayloadMigrationStep>{} : found->second;
    }

    /** @param steps registered chain
     *  @param version current version
     *  @return the step that advances from version, or nullptr */
    const ShmPayloadMigrationStep *stepFrom(const std::vector<ShmPayloadMigrationStep> &steps, std::uint16_t version)
    {
      const auto found = std::find_if(steps.begin(), steps.end(), [version](const ShmPayloadMigrationStep &step) {
        return step.fromVersion == version;
      });
      return found == steps.end() ? nullptr : &*found;
    }

    /** Allocate a payload block and align it inside the block.
     *  @param segment allocator of the segment
     *  @param layout  requested payload layout
     *  @return pointer to the aligned payload; the raw block is released on
     *          failure */
    void *allocatePayload(ShmSegmentAccessor &segment, ShmObject::Layout layout)
    {
      const std::size_t bytes = layout.size + layout.alignment - 1u;
      void *raw = segment.allocate(bytes);
      if (raw == nullptr) { throw std::bad_alloc(); }
      void *aligned = raw;
      std::size_t space = bytes;
      if (std::align(layout.alignment, layout.size, aligned, space) == nullptr) {
        segment.deallocate(raw);
        throw std::bad_alloc();
      }
      return aligned;
    }

    /** Release the payload a header points at, if any.
     *  @param header  object header
     *  @param segment allocator of the segment */
    void releasePayload(ShmObjectHeader &header, ShmSegmentAccessor &segment) noexcept
    {
      if (header.payloadOffset == 0 || header.payloadBytes == 0) {
        header.payloadOffset = 0;
        header.payloadBytes = 0;
        header.state = ShmObjectHeader::kStateConstructing;
        return;
      }
      void *payload = payloadAddressOf(&header);
      try {
        segment.deallocate(payload);
      } catch (...) {
        // The segment is being torn down or the block was already released;
        // leaking is the only safe option here and the segment owns the bytes
        // anyway.
      }
      header.payloadOffset = 0;
      header.payloadBytes = 0;
      header.state = ShmObjectHeader::kStateConstructing;
    }

    /** Publish payload metadata into a header and run the payload constructor.
     *  The caller holds the segment's construct lock.
     *  @param header      header inside the segment
     *  @param segment     allocator of the segment
     *  @param layout      payload layout
     *  @param typeId      payload type id
     *  @param options     registration options
     *  @param init        payload constructor
     *  @param initArgument constructor arguments */
    void initializePayload(ShmObjectHeader &header,
      ShmSegmentAccessor &segment,
      ShmObject::Layout layout,
      std::uint32_t typeId,
      const ShmObjectOptions &options,
      ShmObject::InitFunction init,
      ShmObject::InitArgument initArgument)
    {
      releasePayload(header, segment);
      void *payload = allocatePayload(segment, layout);
      const std::intptr_t offset = reinterpret_cast<std::intptr_t>(payload) - reinterpret_cast<std::intptr_t>(&header);
      try {
        init(payload, initArgument);
      } catch (...) {
        segment.deallocate(payload);
        header.payloadOffset = 0;
        header.payloadBytes = 0;
        throw;
      }
      header.magic = ShmObjectHeader::kMagic;
      header.headerVersion = ShmObjectHeader::kHeaderVersion;
      header.flags = 0;
      header.payloadTypeId = typeId;
      header.payloadVersion = options.payloadVersion;
      header.persistence = static_cast<std::uint8_t>(options.persistence);
      header.reservedByte = 0;
      header.payloadBytes = layout.size;
      header.payloadAlignment = layout.alignment;
      header.payloadOffset = offset;
      header.state = ShmObjectHeader::kStateReady;
    }

    /** Why an existing object cannot be used by this build. */
    std::string incompatibilityOf(const ShmObjectHeader &header, const ShmObject::Layout &layout, std::uint32_t typeId)
    {
      if (!header.isCompatible()) { return "its header layout is not compatible with this build"; }
      if (header.payloadTypeId != typeId) {
        return "it stores payload type " + std::to_string(header.payloadTypeId) + " but this build expects "
               + std::to_string(typeId);
      }
      if (header.payloadBytes != layout.size) {
        return "its payload is " + std::to_string(header.payloadBytes) + " bytes but this build expects "
               + std::to_string(layout.size);
      }
      return "its payload is not initialised";
    }

    /**
     * Locate the named object header, creating or migrating it as needed.
     *
     * Everything runs under the segment's construct lock, exactly like the
     * former ShmCellRecord path: the process wide mutex serializes against a
     * remap of this process, the cross-process semaphore against every other
     * process.
     */
    ShmObjectHeader *findRecordOf(SharedMemoryService &service,
      const std::shared_ptr<SharedMemorySegment> &segment,
      const std::string &name,
      ShmObject::Layout layout,
      std::uint32_t typeId,
      const ShmObjectOptions &options,
      ShmObject::InitFunction init,
      ShmObject::InitArgument initArgument)
    {
      std::lock_guard<std::mutex> lock(service.segmentsMutexOf());
      auto &handle = service.segmentOf(segment->getName());
      SegmentSemaphoreGuard gate(handle);
      if (handle.m_segmentAllocator == nullptr) {
        handle.m_segmentAllocator =
          std::make_shared<BoostSegmentAllocator>(*handle.m_managedMappedFile->get_segment_manager());
      }
      ShmSegmentAccessor accessor(*handle.m_segmentAllocator);
      // A pre-header object can be smaller than the header; never read past its
      // allocation. The layout marker normally catches such segments before
      // this point, but the check keeps a hand-stamped segment from crashing.
      const auto existing = handle.m_managedMappedFile->find<char>(name.c_str());
      if (existing.first != nullptr && existing.second < sizeof(ShmObjectHeader)) {
        throw std::runtime_error("shared memory object '" + name + "' of segment '" + segment->getName()
                                 + "' is too small to carry an object header");
      }
      auto *header = handle.m_managedMappedFile->find_or_construct<ShmObjectHeader>(name.c_str())();

      // A header in the constructing state is either brand new (default
      // constructed by find_or_construct) or left over by a process that died
      // while creating/migrating. The semaphore serializes us against every
      // live process, so taking it over is safe - but only when the bytes
      // actually carry our header magic; otherwise this is a pre-header object.
      if (header->state == ShmObjectHeader::kStateConstructing && header->isCompatible()) {
        initializePayload(*header, accessor, layout, typeId, options, init, initArgument);
        return header;
      }
      if (!header->isCompatible()) {
        // An object written before the header existed, or by a build whose
        // header layout this one cannot read. Its persistence class is unknown,
        // so it must be treated as authoritative.
        throw std::runtime_error("shared memory object '" + name + "' of segment '" + segment->getName()
                                 + "' has no compatible object header (layout mismatch); "
                                   "register a migrator or recreate the segment");
      }
      if (!header->isReady()) {
        throw std::runtime_error("shared memory object '" + name + "' of segment '" + segment->getName()
                                 + "' is still being initialised by another process");
      }
      if (header->payloadVersion == options.payloadVersion) {
        if (header->payloadTypeId != typeId || header->payloadBytes != layout.size) {
          const std::string reason = incompatibilityOf(*header, layout, typeId);
          if (header->persistenceClass() == ShmPersistence::Persistent) {
            throw std::invalid_argument(
              "shared memory object '" + name + "' of segment '" + segment->getName() + "' is incompatible: " + reason);
          }
          LOG_WARN("recreating shared memory object '{}' of segment '{}': {} (persistence allows it)",
            name,
            segment->getName(),
            reason);
          initializePayload(*header, accessor, layout, typeId, options, init, initArgument);
          return header;
        }
        return header;
      }
      if (header->payloadVersion > options.payloadVersion) {
        throw std::runtime_error("shared memory object '" + name + "' of segment '" + segment->getName()
                                 + "' was written with payload version " + std::to_string(header->payloadVersion)
                                 + ", but this build only knows version " + std::to_string(options.payloadVersion));
      }

      // Older payload. A registered chain wins; otherwise the persistence class
      // decides whether the object may be recreated.
      if (ShmPayloadMigrations::canMigrate(typeId, header->payloadVersion, options.payloadVersion)) {
        LOG_WARN("migrating shared memory object '{}' of segment '{}' from payload version {} to {}",
          name,
          segment->getName(),
          header->payloadVersion,
          options.payloadVersion);
        const std::vector<std::byte> migrated = ShmPayloadMigrations::migrate(typeId,
          header->payloadVersion,
          options.payloadVersion,
          payloadAddressOf(header),
          static_cast<std::size_t>(header->payloadBytes),
          layout.size);
        void *payload = allocatePayload(accessor, layout);
        std::memcpy(payload, migrated.data(), layout.size);
        // Publish the new payload first, then release the old block: a process
        // that dies in between leaks the old block, but never sees a header
        // that points at freed memory.
        void *oldPayload = payloadAddressOf(header);
        header->payloadOffset = reinterpret_cast<std::intptr_t>(payload) - reinterpret_cast<std::intptr_t>(header);
        header->payloadBytes = layout.size;
        header->payloadAlignment = layout.alignment;
        header->payloadVersion = options.payloadVersion;
        header->payloadTypeId = typeId;
        header->state = ShmObjectHeader::kStateReady;
        try {
          accessor.deallocate(oldPayload);
        } catch (...) {
          // See releasePayload(): the segment owns the bytes.
        }
        return header;
      }

      if (header->persistenceClass() == ShmPersistence::Persistent) {
        throw std::runtime_error("shared memory object '" + name + "' of segment '" + segment->getName()
                                 + "' is persistent and was written with payload version "
                                 + std::to_string(header->payloadVersion) + " (current "
                                 + std::to_string(options.payloadVersion)
                                 + "); register a payload migration or remove the segment");
      }
      LOG_WARN(
        "recreating shared memory object '{}' of segment '{}': payload version {} is not migratable, "
        "persistence is {}",
        name,
        segment->getName(),
        header->payloadVersion,
        header->persistenceClass() == ShmPersistence::Volatile ? "volatile" : "derived");
      initializePayload(*header, accessor, layout, typeId, options, init, initArgument);
      return header;
    }

  }// namespace

  void ShmPayloadMigrations::registerStep(std::uint32_t typeId, ShmPayloadMigrationStep step)
  {
    if (step.fromVersion >= step.toVersion) {
      throw std::invalid_argument("a payload migration must advance the version");
    }
    if (!step.apply) { throw std::invalid_argument("a payload migration needs a callback"); }
    MigrationRegistry &registry = migrationRegistry();
    std::lock_guard<std::mutex> lock(registry.mutex);
    std::vector<ShmPayloadMigrationStep> &steps = registry.steps[typeId];
    const auto found = std::find_if(steps.begin(), steps.end(), [&step](const ShmPayloadMigrationStep &existing) {
      return existing.fromVersion == step.fromVersion;
    });
    if (found == steps.end()) {
      steps.push_back(std::move(step));
    } else {
      *found = std::move(step);
    }
  }

  bool ShmPayloadMigrations::canMigrate(std::uint32_t typeId, std::uint16_t fromVersion, std::uint16_t toVersion)
  {
    if (fromVersion >= toVersion) { return fromVersion == toVersion; }
    const std::vector<ShmPayloadMigrationStep> steps = stepsOf(typeId);
    std::uint16_t version = fromVersion;
    while (version < toVersion) {
      const ShmPayloadMigrationStep *step = stepFrom(steps, version);
      if (step == nullptr || step->toVersion <= version) { return false; }
      version = step->toVersion;
    }
    return true;
  }

  std::vector<std::byte> ShmPayloadMigrations::migrate(std::uint32_t typeId,
    std::uint16_t fromVersion,
    std::uint16_t toVersion,
    const void *source,
    std::size_t sourceBytes,
    std::size_t finalBytes)
  {
    if (fromVersion >= toVersion) {
      std::vector<std::byte> unchanged(sourceBytes);
      if (sourceBytes > 0 && source != nullptr) { std::memcpy(unchanged.data(), source, sourceBytes); }
      return unchanged;
    }
    const std::vector<ShmPayloadMigrationStep> steps = stepsOf(typeId);
    std::vector<std::byte> current(sourceBytes);
    if (sourceBytes > 0 && source != nullptr) { std::memcpy(current.data(), source, sourceBytes); }
    std::uint16_t version = fromVersion;
    while (version < toVersion) {
      const ShmPayloadMigrationStep *step = stepFrom(steps, version);
      if (step == nullptr || step->toVersion <= version) {
        throw std::runtime_error(
          "no payload migration from version " + std::to_string(version) + " for type " + std::to_string(typeId));
      }
      if (step->sourceBytes != 0 && step->sourceBytes != current.size()) {
        throw std::runtime_error("payload migration from version " + std::to_string(version) + " expects "
                                 + std::to_string(step->sourceBytes) + " bytes but got "
                                 + std::to_string(current.size()));
      }
      const std::size_t targetBytes =
        step->targetBytes != 0 ? step->targetBytes : (step->toVersion == toVersion ? finalBytes : 0);
      if (targetBytes == 0) {
        throw std::runtime_error(
          "payload migration from version " + std::to_string(version) + " does not name an intermediate size");
      }
      std::vector<std::byte> next(targetBytes);
      step->apply(current.data(), current.size(), next.data(), next.size());
      current = std::move(next);
      version = step->toVersion;
    }
    if (current.size() != finalBytes) {
      throw std::runtime_error("payload migration ended with " + std::to_string(current.size()) + " bytes but "
                               + std::to_string(finalBytes) + " were expected");
    }
    return current;
  }

  void ShmPayloadMigrations::reset() noexcept
  {
    MigrationRegistry &registry = migrationRegistry();
    std::lock_guard<std::mutex> lock(registry.mutex);
    registry.steps.clear();
  }

  ShmObject::ShmObject(std::shared_ptr<SharedMemoryService> service,
    std::shared_ptr<SharedMemorySegment> segment,
    std::string name,
    Layout layout,
    std::uint32_t typeId,
    ShmObjectOptions options,
    InitFunction init,
    std::shared_ptr<void> initArgument)
    : m_service(std::move(service)), m_segment(std::move(segment)), m_name(std::move(name)), m_payloadSize(layout.size),
      m_alignment(layout.alignment), m_typeId(typeId), m_options(options), m_init(init),
      m_initArgument(std::move(initArgument))
  {
    if (m_service == nullptr) { throw std::invalid_argument("ShmObject needs a shared memory service"); }
    if (m_segment == nullptr) {
      throw std::invalid_argument("ShmObject '" + m_name + "' needs a shared memory segment");
    }
    if (m_name.empty()) { throw std::invalid_argument("ShmObject name must not be empty"); }
    if (m_payloadSize == 0) { throw std::invalid_argument("ShmObject '" + m_name + "' payload must not be empty"); }
    if (m_init == nullptr) { throw std::invalid_argument("ShmObject '" + m_name + "' needs a payload constructor"); }
    if (m_initArgument == nullptr) {
      throw std::invalid_argument("ShmObject '" + m_name + "' needs constructor arguments");
    }
    if (m_alignment == 0 || m_alignment > kMaxAlignment || (m_alignment & (m_alignment - 1u)) != 0) {
      throw std::invalid_argument("ShmObject '" + m_name + "' alignment must be a power of two of at most "
                                  + std::to_string(kMaxAlignment) + " bytes");
    }
  }

  void *ShmObject::resolve() const
  {
    const std::size_t observed = m_service->getGeneration();
    if (m_record != nullptr && observed == m_generation) {
      auto *header = static_cast<ShmObjectHeader *>(m_record);
      return payloadAddressOf(header);
    }
    auto *header = findRecordOf(*m_service,
      m_segment,
      m_name,
      Layout{ m_payloadSize, m_alignment },
      m_typeId,
      m_options,
      m_init,
      ShmObject::InitArgument{ m_initArgument.get() });
    m_record = header;
    m_generation = observed;
    return payloadAddressOf(header);
  }

  const ShmObjectHeader &ShmObject::header() const
  {
    static_cast<void>(resolve());
    return *static_cast<const ShmObjectHeader *>(m_record);
  }

  bool ShmObject::isStale() const { return m_record == nullptr || m_service->getGeneration() != m_generation; }

  std::size_t ShmObject::generation() const { return m_generation; }

}// namespace shm
}// namespace lintel
