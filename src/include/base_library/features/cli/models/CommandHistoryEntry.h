#ifndef COMMANDHISTORYENTRY_H
#define COMMANDHISTORYENTRY_H

#include <date/date.h>
#include <date/tz.h>
#include <ostream>
#include <string>

/** Represents a single entry in the command history */
class CommandHistoryEntry
{
private:
  std::string m_command;
  std::string m_userName;
  date::sys_time<std::chrono::microseconds> m_executedTimestamp;
  std::string m_menu;

public:
  /** @param command the executed command
   *  @param user_name the user who executed it
   *  @param executed_timestamp time of execution
   *  @param menu the menu context */
  CommandHistoryEntry(const std::string &command,
    const std::string &user_name,
    const date::sys_time<std::chrono::microseconds> &executed_timestamp, const std::string &menu)
    : m_command(command), m_userName(user_name), m_executedTimestamp(executed_timestamp),m_menu(menu)
  {}

  /** @return the executed command */
  [[nodiscard]] std::string getCommand() const { return m_command; }
  /** @return the user name */
  [[nodiscard]] std::string getUserName() const { return m_userName; }
  /** @return the execution timestamp */
  [[nodiscard]] date::sys_time<std::chrono::microseconds> getExecutedTimestamp() const { return m_executedTimestamp; }
  /** @return the menu context */
  [[nodiscard]] std::string getMenu() const { return m_menu; }

  /** @return entry serialized as CSV line */
  [[nodiscard]] std::string asCsvEntry() const
  {
    return StringifyService<date::sys_time<std::chrono::microseconds>>::serializeToString(m_executedTimestamp) + ";"
           + m_userName + ";" + m_command +  ";" + m_menu;
  }


  friend bool operator==(const CommandHistoryEntry &lhs, const CommandHistoryEntry &rhs)
  {
    return lhs.m_command == rhs.m_command && lhs.m_userName == rhs.m_userName
           && lhs.m_executedTimestamp == rhs.m_executedTimestamp && lhs.m_menu == rhs.m_menu;
  }

  friend bool operator!=(const CommandHistoryEntry &lhs, const CommandHistoryEntry &rhs) { return !(lhs == rhs); }
};

#endif// COMMANDHISTORYENTRY_H
