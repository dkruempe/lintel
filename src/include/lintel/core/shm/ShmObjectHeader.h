#ifndef LINTEL_SHM_OBJECT_HEADER_H
#define LINTEL_SHM_OBJECT_HEADER_H

#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>
#include <type_traits>
#include <vector>

#include "lintel/core/utils/TypeName.h"

namespace lintel {
namespace shm {

  /**
   * What may happen to one object when the library cannot interpret its payload.
   *
   * The class is stored inside the segment (in ShmObjectHeader) and is the
   * library's answer to "may I discard this without asking the operator?".
   */
  enum class ShmPersistence : std::uint8_t {
    /** Ephemeral state (event bus rings, message queues): losing it is defined
     *  behaviour, a new object is created instead. */
    Volatile = 0,
    /** Reconstructable from a source of truth (database, XML): the owning
     *  service rebuilds it after the object was recreated. */
    Derived = 1,
    /** Authoritative data: it has to be migrated, or the start aborts. */
    Persistent = 2,
  };

  /** Registration options of one named shared memory object. */
  struct ShmObjectOptions
  {
    /** Version of the payload type; bumped when T's layout or meaning changes. */
    std::uint16_t payloadVersion = 1;
    /** What the library may do when the payload cannot be read. */
    ShmPersistence persistence = ShmPersistence::Persistent;
    /** Explicit type id. 0 means "derive from the payload type name" (the
     *  default in the typed helpers and shared memory repositories). */
    std::uint32_t payloadTypeId = 0;
  };

  /** Size and alignment of one payload allocation.
   *
   *  A single aggregate instead of two adjacent sizes: the two values are easy
   *  to swap at a call site (`{ sizeof(T), alignof(T) }`), and a mistake there
   *  only shows up as a misaligned payload much later. */
  struct ShmPayloadLayout
  {
    /** payload size in bytes */
    std::size_t size = 0;
    /** payload alignment in bytes (power of two) */
    std::size_t alignment = 1;
  };

  /** Payload version stored in the segment and the version this build writes. */
  struct ShmPayloadVersionRange
  {
    /** version written by the build that created the stored payload */
    std::uint16_t fromVersion = 0;
    /** version this build expects and writes */
    std::uint16_t toVersion = 0;
  };

  /** Input of one payload migration: the stored representation and the size
   *  this build expects. */
  struct ShmPayloadMigrationInput
  {
    /** id of the payload type */
    std::uint32_t typeId = 0;
    /** version currently stored and version this build writes */
    ShmPayloadVersionRange versions;
    /** old payload bytes; nullptr is allowed when sourceBytes is 0 */
    const void *source = nullptr;
    /** size of the old payload in bytes */
    std::size_t sourceBytes = 0;
    /** size the migrated payload must have (sizeof(T)) */
    std::size_t targetBytes = 0;
  };

  /**
   * Stable header of every named object in a shared memory segment.
   *
   * The header sits at the first byte of the named object and is the only part
   * whose layout the library promises to keep. The payload itself is a separate
   * allocation of the segment, addressed by payloadOffset relative to the
   * header - changing the library's bookkeeping therefore never moves the
   * payload bytes.
   *
   * headerVersion covers this struct, payloadVersion covers the type stored in
   * the payload. A header layout change is a separate, deliberate migration
   * step; new fields must go into the reserved area until then.
   *
   * The struct is a plain POD: it is created by boost::interprocess inside the
   * segment and must not contain pointers, virtual functions or non-trivial
   * members.
   */
  struct alignas(16) ShmObjectHeader
  {
    /** "LSHO" in ASCII. */
    static constexpr std::uint32_t kMagic = 0x4c53484fU;
    /** Layout version of this header. */
    static constexpr std::uint16_t kHeaderVersion = 1;
    /** The payload allocation is being initialised; not readable yet. */
    static constexpr std::uint32_t kStateConstructing = 1;
    /** Header and payload are complete and readable. */
    static constexpr std::uint32_t kStateReady = 2;

    std::uint32_t magic = kMagic;
    std::uint16_t headerVersion = kHeaderVersion;
    std::uint16_t flags = 0;
    /** FNV-1a id of the payload type name (see payloadTypeIdOf<T>()). */
    std::uint32_t payloadTypeId = 0;
    std::uint16_t payloadVersion = 0;
    std::uint8_t persistence = static_cast<std::uint8_t>(ShmPersistence::Persistent);
    std::uint8_t reservedByte = 0;
    std::uint32_t state = kStateConstructing;
    std::uint32_t reservedWord = 0;
    /** Size of the payload allocation in bytes. */
    std::uint64_t payloadBytes = 0;
    /** Alignment of the payload allocation in bytes. */
    std::uint64_t payloadAlignment = 1;
    /** Distance from this header to the payload; stable in every mapping. */
    std::int64_t payloadOffset = 0;
    std::uint64_t reserved[2]{};

    /** @return true when magic and header layout version match this build */
    [[nodiscard]] bool isCompatible() const noexcept { return magic == kMagic && headerVersion == kHeaderVersion; }

    /** @return true when the payload was completely initialised */
    [[nodiscard]] bool isReady() const noexcept { return state == kStateReady; }

    /** @return the persistence class stored in the header */
    [[nodiscard]] ShmPersistence persistenceClass() const noexcept { return static_cast<ShmPersistence>(persistence); }
  };

  static_assert(sizeof(ShmObjectHeader) == 64, "ShmObjectHeader is part of the on-disk layout");
  static_assert(alignof(ShmObjectHeader) == 16, "ShmObjectHeader alignment is part of the on-disk layout");
  static_assert(std::is_trivially_copyable_v<ShmObjectHeader>, "ShmObjectHeader must be a POD");

  /** @param header a compatible, ready header
   *  @return address of the payload in this process' mapping */
  inline void *payloadAddressOf(ShmObjectHeader *header) noexcept
  {
    if (header == nullptr) { return nullptr; }
    const std::uintptr_t address =
      reinterpret_cast<std::uintptr_t>(header) + static_cast<std::uintptr_t>(header->payloadOffset);
    return std::bit_cast<void *>(address);
  }

  /** @param header a compatible, ready header
   *  @return address of the payload in this process' mapping */
  inline const void *payloadAddressOf(const ShmObjectHeader *header) noexcept
  {
    return payloadAddressOf(const_cast<ShmObjectHeader *>(header));
  }

  /** FNV-1a hash of a type name. Stable within one build and independent of
   *  the address space, which is what a shared header can store. */
  constexpr std::uint32_t fnv1a32(std::string_view text) noexcept
  {
    std::uint32_t hash = 2166136261U;
    for (const char character : text) {
      hash ^= static_cast<std::uint32_t>(static_cast<unsigned char>(character));
      hash *= 16777619U;
    }
    return hash;
  }

  /** @tparam T payload type
   *  @return type id stored in ShmObjectHeader::payloadTypeId */
  template<typename T> constexpr std::uint32_t payloadTypeIdOf() noexcept { return fnv1a32(type_name_full<T>()); }

  /**
   * One migration step of a payload type: reads a complete object of
   * fromVersion and writes a complete object of toVersion.
   *
   * sourceBytes/targetBytes describe the exact sizes of the two intermediate
   * representations. A targetBytes of 0 means "the final payload size" (the
   * size the current build registers for T).
   */
  struct ShmPayloadMigrationStep
  {
    std::uint16_t fromVersion = 0;
    std::uint16_t toVersion = 0;
    std::size_t sourceBytes = 0;
    std::size_t targetBytes = 0;
    std::function<void(const void *source, std::size_t sourceBytes, void *target, std::size_t targetBytes)> apply;
  };

  /**
   * Process wide registry of payload migrations, keyed by type id.
   *
   * Steps are chained: a registered 1→2 and 2→3 step migrates a version 1
   * object to version 3 in one open. Registration happens during startup,
   * before the first repository is constructed; lookup and migration run under
   * the segment's construct lock.
   */
  class ShmPayloadMigrations
  {
  public:
    /** Register or replace one step.
     *  @param typeId id of the payload type
     *  @param step   the step; a step without `apply` is rejected
     *  @throws std::invalid_argument on an invalid step */
    static void registerStep(std::uint32_t typeId, ShmPayloadMigrationStep step);

    /** @return true when a complete chain from fromVersion to toVersion exists */
    [[nodiscard]] static bool canMigrate(std::uint32_t typeId, ShmPayloadVersionRange versions);

    /**
     * Run the chain and return the migrated payload bytes.
     *
     * @param input type id, version range, old payload bytes and target size
     * @return the migrated bytes, exactly input.targetBytes long
     * @throws std::runtime_error when no chain exists or a step is broken
     */
    [[nodiscard]] static std::vector<std::byte> migrate(const ShmPayloadMigrationInput &input);

    /** Drop every registered step. Intended for tests. */
    static void reset() noexcept;
  };

  /**
   * Registration of one migration step for T.
   *
   * A single aggregate instead of six positional parameters: the version pair
   * and the two byte counts are easy to swap, and a swapped pair would only
   * surface as a corrupted payload long after registration.
   */
  struct ShmPayloadMigrationRegistration
  {
    /** version stored in the segment */
    std::uint16_t fromVersion = 0;
    /** version this build writes */
    std::uint16_t toVersion = 0;
    /** places a T into the target buffer */
    std::function<void(const void *source, std::size_t sourceBytes, void *target)> apply;
    /** type id; 0 derives it from T */
    std::uint32_t typeId = 0;
    /** expected size of the old payload; 0 uses the header */
    std::size_t sourceBytes = 0;
    /** size after this step; 0 uses sizeof(T) */
    std::size_t targetBytes = 0;
  };

  /**
   * Register a v(n) → v(n+1) migration for T.
   *
   * The callback receives the old payload bytes and a target buffer of the
   * current payload size; it has to placement-construct (or copy) the current T
   * into the target.
   *
   * @tparam T payload type
   * @param registration versions, callback and optional sizes/type id
   */
  template<typename T> void registerPayloadMigration(ShmPayloadMigrationRegistration registration)
  {
    ShmPayloadMigrationStep step;
    step.fromVersion = registration.fromVersion;
    step.toVersion = registration.toVersion;
    step.sourceBytes = registration.sourceBytes;
    step.targetBytes = registration.targetBytes != 0 ? registration.targetBytes : sizeof(T);
    step.apply = [callback = std::move(registration.apply)](
                   const void *source, std::size_t sourceBytes, void *target, std::size_t /*targetBytes*/) {
      if (callback) { callback(source, sourceBytes, target); }
    };
    ShmPayloadMigrations::registerStep(
      registration.typeId != 0 ? registration.typeId : payloadTypeIdOf<T>(), std::move(step));
  }

}// namespace shm
}// namespace lintel

#endif// LINTEL_SHM_OBJECT_HEADER_H
