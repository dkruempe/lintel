#include "lintel/core/persistence/ConnectionType.h"
#include <vector>
#include <memory>
#include <string>
#include "lintel/core/services/LoggerService.h"
#include "lintel/core/persistence/Connection.h"
#include "lintel/core/persistence/ParameterBuilder.h"
#include "lintel/core/persistence/PreparedStatement.h"
#include "lintel/core/persistence/Result.h"
#include "lintel/features/base/models/HistoryEntry.h"

int main(int argc, char *argv[])
{
  // Password placeholder: enter your own password here.
  // The old password is in the git history and must be rotated.
  std::shared_ptr<DatabaseConnectionEntry> databaseConnectionEntry = std::make_shared<DatabaseConnectionEntry>("test",
    "127.0.0.1",
    "example_user",
    "${ADMIN_PASSWORD}",
    db::ConnectionType::PostgreSQL,
    "DEFAULT_PSQL",
    -1,
    "temp",
    true);
  db::Connection const connection(databaseConnectionEntry);
  std::string stmt = R"(
            select process_name,
                   service_name,
                   label,
                   text,
                   created_timestamp
            from history
            where process_name ~* ?
              and service_name ~* ?
              and label ~* ?)";

  db::PreparedStatement preparedStatement(connection, stmt, "history_all_process_service_label_of");
  std::vector<HistoryEntry> entries;
  db::ParameterBuilder builder(databaseConnectionEntry);
  std::string param = ".*";
  builder.add(param);
  builder.add(param);
  builder.add(param);
  db::Result result = preparedStatement.execute(builder);
  for (auto &item : result) {
    const std::string processNameTemp = item.of(0).getValue();
    const std::string serviceNameTemp = item.of(1).getValue();
    const std::string labelTemp = item.of(2).getValue();
    const std::string text = item.of(3).getValue();
    auto createdTimestamp = item.of(4).getValue<date::sys_time<std::chrono::microseconds>>();
    const HistoryEntry entry(processNameTemp, serviceNameTemp, labelTemp, text, createdTimestamp);
    entries.push_back(entry);
  }
  LOG_INFO("{} entries", entries.size());
  return 0;
}
