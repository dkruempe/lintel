#ifndef LINTEL_SHAREDMEMORYREPOSITORYREGISTRY_H
#define LINTEL_SHAREDMEMORYREPOSITORYREGISTRY_H

#include <memory>
#include <string>
#include <unordered_map>

namespace lintel {

class SharedMemoryRepositoryRegistry
{
public:
  static SharedMemoryRepositoryRegistry &instance();

  void registerRepository(const std::string &name, std::shared_ptr<void> repo);
  std::shared_ptr<void> get(const std::string &name) const;

private:
  std::unordered_map<std::string, std::shared_ptr<void>> m_repos;
};

}// namespace lintel
#endif
