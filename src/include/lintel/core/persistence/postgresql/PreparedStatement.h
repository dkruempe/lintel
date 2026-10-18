#ifndef LINTEL_POSTGRESQL_PREPAREDSTATEMENT_H
#define LINTEL_POSTGRESQL_PREPAREDSTATEMENT_H

#include <string_view>

#include "Connection.h"

namespace lintel {
namespace postgresql {
  /**
   * format of prepared statement
   * example:
   * insert into table (..,..) values(?,?);
   *
   * => ? will be replace automatically with $1 and $2
   * => algorithm will automatically count the number of parameters
   */
  class PreparedStatement
  {
  private:
    bool m_closed = false;
    const Connection &m_connection;
    const int32_t m_nParams;
    const std::string m_statementName;
    const std::string m_statement;

  public:
    /**
     * Counts the number of '?' placeholders in a statement template.
     * @param tempStatement the statement template
     * @return the number of parameters
     */
    static constexpr int32_t initNParams(std::string_view tempStatement)
    {
      int32_t count = 0;
      for (const char c : tempStatement) {
        if (c == '?') { ++count; }
      }
      return count;
    }

    /**
     * Converts '?' placeholders to PostgreSQL's $1, $2, ... format.
     * @param tempStatement the statement template with '?' placeholders
     * @return the converted statement
     */
    static std::string initStatement(const std::string &tempStatement);

    /** The name and SQL template of a prepared statement. */
    struct Definition
    {
      /** the name the statement is prepared under */
      std::string name;
      /** the SQL statement template with '?' placeholders */
      std::string statement;
    };

    /**
     * Prepares a statement on the given connection.
     * @param connection the PostgreSQL connection
     * @param definition the statement name and SQL template
     */
    PreparedStatement(const Connection &connection, const Definition &definition);

    /**
     * Destroys the object and deallocates the server-side prepared statement.
     *
     * A pooled connection is reused, so a prepared statement that survived the
     * release would still be visible to the next lease (and a name collision
     * would make the next prepare fail). The destructor deallocates it, and
     * because destructors must not throw it swallows the failure of a
     * connection that is already gone - the pool's health check drops such a
     * connection anyway.
     *
     * The statement stays *named* rather than unnamed (`PQprepare` with `""`):
     * the parameterized query path (`Statement::execute(query, builder)`) uses
     * `PQexecParams`, which also targets the unnamed statement, so an unnamed
     * prepared statement would be clobbered between its prepare and execute.
     * Deallocating in the destructor is the deterministic equivalent for reuse.
     */
    ~PreparedStatement() noexcept;

    /**
     * Executes the prepared statement with the given parameters.
     * @param params the parameter values as strings
     * @return shared pointer to the result
     */
    std::shared_ptr<Result> execute(const std::vector<std::string> &params);

    /** Closes the prepared statement. */
    void close();
  };
}// namespace postgresql
}// namespace lintel
#endif// LINTEL_POSTGRESQL_PREPAREDSTATEMENT_H
