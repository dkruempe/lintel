#include <base_library/core/persistence/Connection.h>
#include <base_library/core/persistence/Notify.h>
#include <base_library/core/persistence/PreparedStatement.h>
#include <base_library/core/persistence/Statement.h>
#include <base_library/core/persistence/Transaction.h>

#include <chrono>
#include <iostream>

std::shared_ptr<ConnectionEntry> connPostgres =
    std::make_shared<ConnectionEntry>("", "", "", "",
                                      db::ConnectionType::PostgreSQL,
                                      "postgresql", -1, "keyValueStore");
std::shared_ptr<ConnectionEntry> connSqlite = std::make_shared<ConnectionEntry>(
    "", "/Users/dkruempe/Documents/dev/plc/test.db", "", "",
    db::ConnectionType::SQLite, "sqlite", -1, "");

void selectExample() {
  try {
    db::Connection connection(connPostgres);
    db::Transaction transaction(connection);
    db::Statement query(connection);
    db::Result result =
        query.execute("select key, value from key_value_store limit 100");
    for (auto &arguments : result) {
      for (auto &argument : arguments) {
        std::cout << argument << std::endl;
      }
    }
  } catch (db::SQLException &exception) {
    fprintf(stderr, "%s", exception.what());
  }
}

void selectExampleSQLite() {
  try {
    db::Connection connection(connSqlite);
    db::Transaction transaction(connection);
    db::Statement query(connection);
    db::Result result = query.execute("select key, value from key_value");
    for (auto &arguments : result) {
      for (auto &argument : arguments) {
        std::cout << argument << std::endl;
      }
    }
  } catch (db::SQLException &exception) {
    fprintf(stderr, "%s", exception.what());
  }
}

void deleteExample() {
  try {
    db::Connection connection(connPostgres);
    db::Transaction transaction(connection);
    db::Statement query(connection);
    db::Result result =
        query.execute("delete from key_value_store where key like '%'");
  } catch (db::SQLException &exception) {
    fprintf(stderr, "%s", exception.what());
  }
}

void deleteExampleSQLite() {
  try {
    db::Connection connection(connSqlite);
    db::Transaction transaction(connection);
    db::Statement query(connection);
    db::Result result =
        query.execute("delete from key_value where key like '%'");
  } catch (db::SQLException &exception) {
    fprintf(stderr, "%s", exception.what());
  }
}

void insertExample() {
  try {
    db::Connection connection(connPostgres);
    db::Transaction transaction(connection);
    db::PreparedStatement preparedStatement(
        connection, "INSERT INTO KEY_VALUE_STORE (key, value) values (?,?)",
        "insertKeyValueStore");
    preparedStatement.execute({"Anna", "Dominik"});
    preparedStatement.execute({"Mama", "Papa"});
    preparedStatement.execute({"Oma", "Opa"});
  } catch (db::SQLException &exception) {
    fprintf(stderr, "%s", exception.what());
  }
}

void insertExampleSQLite() {
  try {
    db::Connection connection(connSqlite);
    db::Transaction transaction(connection);
    db::PreparedStatement preparedStatement(
        connection, "INSERT INTO KEY_VALUE (key, value) values (?,?)",
        "insertKeyValueStore");
    preparedStatement.execute({"Anna", "Dominik"});
    preparedStatement.execute({"Mama", "Papa"});
    preparedStatement.execute({"Oma", "Opa"});
  } catch (db::SQLException &exception) {
    fprintf(stderr, "%s", exception.what());
  }
}

void testNotify() {
  try {
    db::Connection connection(connPostgres);
    std::function<void()> func = [&]() {
      std::cout << "VALUE CHANGED" << std::endl;
    };
    db::Notify notify(connection, func, "my_channel");
    std::this_thread::sleep_for(std::chrono::seconds(10));
  } catch (db::SQLException &exception) {
    fprintf(stderr, "%s", exception.what());
  }
}

void insertStatementExample() {
  try {
    db::Connection connection(connPostgres);
    db::Connection connectionNotify(connPostgres);
    std::function<void()> func = [&]() {
      std::cout << "VALUE CHANGED" << std::endl;
    };
    db::Notify notify(connectionNotify, func, "key_value_store_channel");
    db::Transaction transaction(connection);
    db::Statement statement(connection);
    statement.execute("INSERT INTO KEY_VALUE_STORE (key, value) values (?,?)",
                      {"Example", "User"});
  } catch (db::SQLException &exception) {
    fprintf(stderr, "%s", exception.what());
  }
}

void insertStatementExampleSQLite() {
  try {
    db::Connection connection(connSqlite);
    std::function<void()> func = [&]() {
      std::cout << "NOTIFY DATA CHANGED" << std::endl;
    };
    db::Notify notify(connection, func, "key_value");
    db::Transaction transaction(connection);
    db::Statement statement(connection);
    statement.execute("INSERT INTO KEY_VALUE (key, value) values (?,?)",
                      {"Example", "User"});
  } catch (db::SQLException &exception) {
    fprintf(stderr, "%s", exception.what());
  }
}

void insertPerformanceTest() {
  auto start = std::chrono::steady_clock::now();
  int32_t numOfTelegrams = 1000000;
  try {
    db::Connection connection(connPostgres);
    db::Transaction transaction(connection);
    int32_t statementBatchSize = 100;
    int32_t batchSize = 1000;
    std::string statement = "INSERT INTO KEY_VALUE_STORE (key, value) values";
    for (int i = 0; i < statementBatchSize; i++) {
      if (i != statementBatchSize - 1) {
        statement += " (?,?),";
      } else {
        statement += " (?,?)";
      }
    }
    db::PreparedStatement preparedStatement(connection, statement,
                                            "insertKeyValueStore");
    for (int i = 0; i < numOfTelegrams; i = i + batchSize) {
      for (int k = 0; k < batchSize / statementBatchSize; k++) {
        std::vector<std::string> params;
        for (int j = 0; j < statementBatchSize; j++) {
          params.push_back("key-" +
                           std::to_string(i + statementBatchSize * k + j));
          params.push_back("value-" +
                           std::to_string(i + statementBatchSize * k + j));
        }
        preparedStatement.execute(params);
      }
    }
  } catch (db::SQLException &exception) {
    fprintf(stderr, "%s", exception.what());
  }
  auto now = std::chrono::steady_clock::now();
  auto nanoseconds =
      std::chrono::duration_cast<std::chrono::nanoseconds>(now - start);
  float performance =
      numOfTelegrams / (float)nanoseconds.count() * 1000000000.0;
  std::cout << "Performance: " << std::to_string(performance) << std::endl;
}

void insertPerformanceTestSQLite() {
  auto start = std::chrono::steady_clock::now();
  int32_t numOfTelegrams = 1000000;
  try {
    db::Connection connection(connSqlite);
    db::Transaction transaction(connection);
    int32_t statementBatchSize = 100;
    int32_t batchSize = 1000;
    std::string statement = "INSERT INTO KEY_VALUE (key, value) values";
    for (int i = 0; i < statementBatchSize; i++) {
      if (i != statementBatchSize - 1) {
        statement += " (?,?),";
      } else {
        statement += " (?,?)";
      }
    }
    db::PreparedStatement preparedStatement(connection, statement,
                                            "insertKeyValueStore");
    for (int i = 0; i < numOfTelegrams; i = i + batchSize) {
      for (int k = 0; k < batchSize / statementBatchSize; k++) {
        std::vector<std::string> params;
        for (int j = 0; j < statementBatchSize; j++) {
          params.push_back("key-" +
                           std::to_string(i + statementBatchSize * k + j));
          params.push_back("value-" +
                           std::to_string(i + statementBatchSize * k + j));
        }
        preparedStatement.execute(params);
      }
    }
  } catch (db::SQLException &exception) {
    fprintf(stderr, "%s", exception.what());
  }
  auto now = std::chrono::steady_clock::now();
  auto nanoseconds =
      std::chrono::duration_cast<std::chrono::nanoseconds>(now - start);
  float performance =
      numOfTelegrams / (float)nanoseconds.count() * 1000000000.0;
  std::cout << "Performance: " << std::to_string(performance) << std::endl;
}

void transActionTest() {
  try {
    db::Connection connection(connPostgres);
    db::Transaction transaction(connection);
    db::Statement query(connection);
    query.execute(
        "INSERT INTO key_value_store (key, value) values('Anna', 'Dominik')");
    transaction.commit();  // finished transaction
    transaction.start();
    query.execute(
        "INSERT INTO key_value_store (key, value) values('Oma', 'Opa')");
    transaction.rollback();  // finished transaction
    transaction.start();
    query.execute(
        "INSERT INTO key_value_store (key,value) values('Katharina', "
        "'Pierre')");
    transaction.save("save");
    query.execute(
        "INSERT INTO key_value_store (key,value) values('Jeniffer', 'Daniel')");
    transaction.rollbackTo("save");
    transaction.commit();
    transaction.start();
    const db::Result &result =
        query.execute("select key, value from key_value_store");
    for (int i = 0; i < result.getSize(); i++) {
      for (int j = 0; j < result.getNumOfAttributes(); j++) {
        const std::string &string = result.getValue(i, j);
        std::cout << string << std::endl;
      }
    }
    transaction.commit();
  } catch (db::SQLException &exception) {
    fprintf(stderr, "%s", exception.what());
  }
}

void transActionTestSQLite() {
  try {
    db::Connection connection(connSqlite);
    db::Transaction transaction(connection);
    db::Statement query(connection);
    query.execute(
        "INSERT INTO key_value (key, value) values('Anna', 'Dominik')");
    transaction.commit();  // finished transaction

    transaction.start();
    query.execute("INSERT INTO key_value (key, value) values('Oma', 'Opa')");
    transaction.rollback();  // finished transaction

    transaction.start();
    query.execute(
        "INSERT INTO key_value (key,value) values('Katharina', "
        "'Pierre')");
    transaction.save("save");
    query.execute(
        "INSERT INTO key_value (key,value) values('Jeniffer', 'Daniel')");
    transaction.rollbackTo("save");
    transaction.commit();

    transaction.start();
    const db::Result &result =
        query.execute("select key, value from key_value");
    for (int i = 0; i < result.getSize(); i++) {
      for (int j = 0; j < result.getNumOfAttributes(); j++) {
        const std::string &string = result.getValue(i, j);
        std::cout << string << std::endl;
      }
    }
    transaction.commit();
  } catch (db::SQLException &exception) {
    fprintf(stderr, "%s", exception.what());
  }
}

void testPostgres() {
  // postgresql test
  deleteExample();
  insertStatementExample();
  insertExample();
  selectExample();
  deleteExample();
  transActionTest();
  selectExample();
  deleteExample();
  insertPerformanceTest();
}

void testSQLite() {
  deleteExampleSQLite();
  insertStatementExampleSQLite();
  insertExampleSQLite();
  selectExampleSQLite();
  deleteExampleSQLite();
  transActionTestSQLite();
  selectExampleSQLite();
  deleteExampleSQLite();
  insertPerformanceTestSQLite();
}

int main(int argc, char *argv[]) {
  testPostgres();
  testSQLite();
  return 0;
}