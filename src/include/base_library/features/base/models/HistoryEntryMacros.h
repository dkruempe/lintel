#ifndef CPP_BASE_LIBRARY_HISTORYENTRYMACROS_H
#define CPP_BASE_LIBRARY_HISTORYENTRYMACROS_H

/**
 * Plain (non-module) header containing the history entry convenience macros.
 * Macros cannot be exported from C++20 modules, so they live in a separate
 * header that both module interface units and module consumers can include.
 */
#define DEFINE_HISTORY_ENTRY(name, label, text) HistoryEntry name = HistoryEntry(*this, label, text)

#define DEFINE_HISTORY_ENTRY2(name, label, text, processName, serviceName) \
  HistoryEntry name = HistoryEntry(processName, serviceName, label, text, std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now()))

#endif// CPP_BASE_LIBRARY_HISTORYENTRYMACROS_H
