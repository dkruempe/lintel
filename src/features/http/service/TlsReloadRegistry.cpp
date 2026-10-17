#include "lintel/features/http/service/TlsReloadRegistry.h"

#include <exception>

namespace lintel {
TlsReloadRegistry &TlsReloadRegistry::instance()
{
  static TlsReloadRegistry registry;
  return registry;
}

TlsReloadRegistry::ReloadId TlsReloadRegistry::registerHandler(std::string name, Handler handler)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  // a fresh id on every call, even when the name is replaced: the previous
  // owner of the name keeps its old id and can no longer unregister the entry
  const ReloadId id = m_nextId++;
  for (auto &entry : m_entries) {
    if (entry.m_name == name) {
      entry.m_id = id;
      entry.m_handler = std::move(handler);
      return id;
    }
  }
  m_entries.push_back(Entry{ id, std::move(name), std::move(handler) });
  return id;
}

void TlsReloadRegistry::unregisterHandler(ReloadId id)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto iter = m_entries.begin(); iter != m_entries.end(); ++iter) {
    if (iter->m_id == id) {
      m_entries.erase(iter);
      return;
    }
  }
}

void TlsReloadRegistry::clear()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_entries.clear();
}

std::vector<TlsReloadRegistry::ReloadResult> TlsReloadRegistry::reload() const
{
  // snapshot under the lock, execute outside of it: a handler re-reads files
  // and must not serialize registration of other handlers
  std::vector<Entry> entries;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    entries = m_entries;
  }
  std::vector<ReloadResult> results;
  results.reserve(entries.size());
  for (const auto &entry : entries) {
    ReloadResult result;
    result.m_name = entry.m_name;
    try {
      if (entry.m_handler == nullptr) {
        result.m_reloaded = false;
        result.m_detail = "handler is empty";
      } else {
        result.m_detail = entry.m_handler();
        result.m_reloaded = result.m_detail.empty();
      }
    } catch (const std::exception &exception) {
      result.m_reloaded = false;
      result.m_detail = exception.what();
    } catch (...) {
      result.m_reloaded = false;
      result.m_detail = "unknown exception";
    }
    results.push_back(std::move(result));
  }
  return results;
}
}// namespace lintel
