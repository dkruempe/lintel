#ifndef LINTEL_SHAREDMEMORYREPOSITORYREGISTRY_H
#define LINTEL_SHAREDMEMORYREPOSITORYREGISTRY_H

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace lintel {

/**
 * Process wide name -> typed repository registry for the shared memory
 * repositories (lintel::shm::SharedMemoryRepository<T> and the containers it
 * stores).
 *
 * The registry holds shared_ptrs, so a registered repository stays alive even
 * when the wiring that created it goes out of scope; removing it is the only
 * way to release it again. Registration and lookup are typed: an entry is
 * remembered with the std::type_index of its T, and asking for another type
 * throws instead of handing out a reinterpret_cast'ed pointer that would fail
 * somewhere inside get()/set() instead of here.
 *
 * The registry is not internally synchronized: register at wiring time (single
 * threaded startup) and hand the shared_ptr out to the threads that use it.
 */
class SharedMemoryRepositoryRegistry
{
public:
  static SharedMemoryRepositoryRegistry &instance();

  /** Register a repository under a name, replacing an existing entry of the
   *  same name.
   *  @tparam T    payload type of the repository
   *  @param name  the name to register under
   *  @param repo  the repository to keep alive */
  template<typename T> void registerRepository(const std::string &name, std::shared_ptr<T> repo)
  {
    // Aggregate init, not member assignment: std::type_index has no default
    // constructor, so Entry can only be built with both members at once.
    const Entry entry{ .repository = std::static_pointer_cast<void>(repo), .type = std::type_index(typeid(T)) };
    m_repos.insert_or_assign(name, entry);
  }

  /** Look up a repository by name.
   *  @tparam T    expected payload type of the repository
   *  @param name  the registered name
   *  @return the repository, or nullptr when the name is not registered
   *  @throws std::invalid_argument when the name holds a repository of another type */
  template<typename T> [[nodiscard]] std::shared_ptr<T> getAs(const std::string &name) const
  {
    auto found = m_repos.find(name);
    if (found == m_repos.end()) { return nullptr; }
    if (found->second.type != std::type_index(typeid(T))) {
      throw std::invalid_argument("shared memory repository '" + name + "' is not of the requested type");
    }
    return std::static_pointer_cast<T>(found->second.repository);
  }

  /** @param name the registered name
   *  @return true when a repository is registered under that name */
  [[nodiscard]] bool contains(const std::string &name) const { return m_repos.find(name) != m_repos.end(); }

  /** Remove a repository and release it.
   *  @param name the registered name
   *  @return true when an entry was removed */
  bool remove(const std::string &name) { return m_repos.erase(name) > 0; }

  /** @return number of registered repositories */
  [[nodiscard]] std::size_t size() const { return m_repos.size(); }

private:
  /** One registry entry: the kept-alive repository and the type it was
   *  registered with, so getAs() can refuse a mismatched lookup. */
  struct Entry
  {
    std::shared_ptr<void> repository;
    std::type_index type;
  };

  std::unordered_map<std::string, Entry> m_repos;
};

}// namespace lintel
#endif
