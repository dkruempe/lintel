#ifndef LINTEL_MOCKHISTORYSERVICE_H
#define LINTEL_MOCKHISTORYSERVICE_H

#include <catch2/trompeloeil.hpp>

#include <string>
#include <vector>

#include "lintel/features/base/services/IHistoryService.h"

namespace lintel {
class MockHistoryService : public IHistoryService
{
public:
  std::string processName{ "MockProcess" };

  MAKE_MOCK1(historizeOf, void(std::vector<HistoryEntry>), override);

  [[nodiscard]] const std::string &getProcessName() const override { return processName; }

  MAKE_CONST_MOCK0(allOf, std::vector<HistoryEntry>(), override);
  MAKE_CONST_MOCK1(allOf, std::vector<HistoryEntry>(const std::string &), override);
  MAKE_CONST_MOCK2(allOf, std::vector<HistoryEntry>(const std::string &, const std::string &), override);
  MAKE_CONST_MOCK1(allOfProcess, std::vector<HistoryEntry>(const std::string &), override);
  MAKE_CONST_MOCK1(allOfService, std::vector<HistoryEntry>(const std::string &), override);
};

}// namespace lintel
#endif// LINTEL_MOCKHISTORYSERVICE_H
