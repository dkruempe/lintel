#include "lintel/features/base/services/SharedMemoryRepositoryRegistry.h"

namespace lintel {

SharedMemoryRepositoryRegistry &SharedMemoryRepositoryRegistry::instance()
{
  static SharedMemoryRepositoryRegistry instance;
  return instance;
}

void SharedMemoryRepositoryRegistry::registerRepository(const std::string &name, std::shared_ptr<void> repo)
{
  m_repos[name] = std::move(repo);
}

std::shared_ptr<void> SharedMemoryRepositoryRegistry::get(const std::string &name) const
{
  auto it = m_repos.find(name);
  if (it == m_repos.end()) { return nullptr; }
  return it->second;
}

}// namespace lintel
