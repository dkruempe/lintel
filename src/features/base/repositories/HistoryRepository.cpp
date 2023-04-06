#include "base_library/features/base/repositories/HistoryRepository.h"

#include "base_library/core/persistence/PreparedStatement.h"
HistoryRepository::HistoryRepository(
    const std::shared_ptr<DatabaseConnectionConfigurations>
        databaseConnectionConfigurations)
    : m_connectionConfigurations(databaseConnectionConfigurations),
      m_connectionEntry(m_connectionConfigurations->ofDefault()) {}
std::vector<HistoryEntry> HistoryRepository::allOf() const {
  db::Connection connection(m_connectionEntry);
  db::PreparedStatement preparedStatement(connection,
                                          R"(
       select process_name,
              service_name,
              label,
              text,
              created_timestamp
      from history)",
                                          "history_all_of");
  std::vector<HistoryEntry> entries;
  db::ParameterBuilder builder(m_connectionEntry);
  db::Result result = preparedStatement.execute(builder);
  for (auto &item : result) {
    std::string processName = item.of(0).getValue();
    std::string serviceName = item.of(1).getValue();
    std::string label = item.of(2).getValue();
    std::string text = item.of(3).getValue();
    auto createdTimestamp =
        item.of(4).getValue<date::sys_time<std::chrono::microseconds>>();
    HistoryEntry entry(processName, serviceName, label, text, createdTimestamp);
    entries.push_back(entry);
  }
  return entries;
}
std::vector<HistoryEntry> HistoryRepository::allOf(
    const std::string &label) const {
  db::Connection connection(m_connectionEntry);
  db::PreparedStatement preparedStatement(connection,
                                          R"(
       select process_name,
              service_name,
              label,
              text,
              created_timestamp
      from history
      where label = ?)",
                                          "history_all_label_of");
  std::vector<HistoryEntry> entries;
  db::ParameterBuilder builder(m_connectionEntry);
  builder.add(label);
  db::Result result = preparedStatement.execute(builder);
  for (auto &item : result) {
    std::string processName = item.of(0).getValue();
    std::string serviceName = item.of(1).getValue();
    std::string text = item.of(3).getValue();
    auto createdTimestamp =
        item.of(4).getValue<date::sys_time<std::chrono::microseconds>>();
    HistoryEntry entry(processName, serviceName, label, text, createdTimestamp);
    entries.push_back(entry);
  }
  return entries;
}
std::vector<HistoryEntry> HistoryRepository::allOf(
    const std::string &processName, const std::string &serviceName) const {
  db::Connection connection(m_connectionEntry);
  db::PreparedStatement preparedStatement(connection,
                                          R"(
       select process_name,
              service_name,
              label,
              text,
              created_timestamp
      from history
      where process_name = ?
            service_name = ?)",
                                          "history_all_process_service_of");
  std::vector<HistoryEntry> entries;
  db::ParameterBuilder builder(m_connectionEntry);
  builder.add(processName);
  builder.add(serviceName);
  db::Result result = preparedStatement.execute(builder);
  for (auto &item : result) {
    std::string label = item.of(2).getValue();
    std::string text = item.of(3).getValue();
    auto createdTimestamp =
        item.of(4).getValue<date::sys_time<std::chrono::microseconds>>();
    HistoryEntry entry(processName, serviceName, label, text, createdTimestamp);
    entries.push_back(entry);
  }
  return entries;
}
std::vector<HistoryEntry> HistoryRepository::allOfProcess(
    const std::string &processName) const {
  db::Connection connection(m_connectionEntry);
  db::PreparedStatement preparedStatement(connection,
                                          R"(
       select process_name,
              service_name,
              label,
              text,
              created_timestamp
      from history
      where process_name = ?)",
                                          "history_all_process_of");
  std::vector<HistoryEntry> entries;
  db::ParameterBuilder builder(m_connectionEntry);
  builder.add(processName);
  db::Result result = preparedStatement.execute(builder);
  for (auto &item : result) {
    std::string serviceName = item.of(1).getValue();
    std::string label = item.of(2).getValue();
    std::string text = item.of(3).getValue();
    auto createdTimestamp =
        item.of(4).getValue<date::sys_time<std::chrono::microseconds>>();
    HistoryEntry entry(processName, serviceName, label, text, createdTimestamp);
    entries.push_back(entry);
  }
  return entries;
}
std::vector<HistoryEntry> HistoryRepository::allOfService(
    const std::string &serviceName) const {
  db::Connection connection(m_connectionEntry);
  db::PreparedStatement preparedStatement(connection,
                                          R"(
       select process_name,
              service_name,
              label,
              text,
              created_timestamp
      from history
      where service_name = ?)",
                                          "history_all_service_of");
  std::vector<HistoryEntry> entries;
  db::ParameterBuilder builder(m_connectionEntry);
  builder.add(serviceName);
  db::Result result = preparedStatement.execute(builder);
  for (auto &item : result) {
    std::string processName = item.of(0).getValue();
    std::string label = item.of(2).getValue();
    std::string text = item.of(3).getValue();
    auto createdTimestamp =
        item.of(4).getValue<date::sys_time<std::chrono::microseconds>>();
    HistoryEntry entry(processName, serviceName, label, text, createdTimestamp);
    entries.push_back(entry);
  }
  return entries;
}
void HistoryRepository::insertOf(const std::vector<HistoryEntry> &entries) {
  db::Connection connection(m_connectionEntry);
  db::PreparedStatement preparedStatement(connection, R"(
    insert into history (
    process_name,
    service_name,
    label,
    text,
    created_timestamp)
    values (?, ?, ?, ?, ?)
)",
                                          "history_insert_of");
  db::ParameterBuilder builder(m_connectionEntry);
  for (const auto &entry : entries) {
    builder.add(entry.getProcessName());
    builder.add(entry.getServiceName());
    builder.add(entry.getLabel());
    builder.add(entry.getText());
    builder.add(entry.getCreatedTimestamp());
    preparedStatement.execute(builder);
    builder.clear();
  }
}
void HistoryRepository::cleanAllOlderThan(
    date::sys_time<std::chrono::microseconds> timestamp) {
  db::Connection connection(m_connectionEntry);
  db::PreparedStatement preparedStatement(connection, R"(
    delete from history
    where created_timestamp < ?
  )",
                                          "history_cleanup");
  db::ParameterBuilder builder(m_connectionEntry);
  builder.add(timestamp);
  preparedStatement.execute(builder);
}
