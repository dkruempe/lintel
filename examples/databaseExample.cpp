#include <base_library/persistence/Connection.h>
#include <base_library/persistence/PreparedStatement.h>
#include <base_library/persistence/Statement.h>
#include <base_library/persistence/Transaction.h>

#include <iostream>
#include <chrono>

void selectExample() {
  std::string connInfo = "dbname = keyValueStore";
  try {
    db::Connection connection(connInfo);
    db::Transaction transaction(connection);
    db::Statement query(connection);
    db::Result result =
        query.execute("select key, value from key_value_store limit 100");
    for (int i = 0; i < result.getSize(); i++) {
      std::cout << "(";
      for (int j = 0; j < result.getNumOfAttributes(); j++) {
        std::cout << result.getValue(i, j);
        if (j != result.getNumOfAttributes() - 1) {
          std::cout << ", ";
        } else {
          std::cout << ")";
        }
      }
      std::cout << "\n";
    }
  } catch (db::SQLException &exception) {
    fprintf(stderr, "%s", exception.what());
  }
}

void deleteExample() {
  std::string connInfo = "dbname = keyValueStore";
  try {
    db::Connection connection(connInfo);
    db::Transaction transaction(connection);
    db::Statement query(connection);
    db::Result result =
        query.execute("delete from key_value_store where key like '%'");
  } catch (db::SQLException &exception) {
    fprintf(stderr, "%s", exception.what());
  }
}

void insertExample() {
  std::string connInfo = "dbname = keyValueStore";
  try {
    db::Connection connection(connInfo);
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

void insertStatementExample() {
  std::string connInfo = "dbname = keyValueStore";
  try {
    db::Connection connection(connInfo);
    db::Transaction transaction(connection);
    db::Statement statement(connection);
    statement.execute("INSERT INTO KEY_VALUE_STORE (key, value) values (?,?)", {"Example", "User"});
  } catch (db::SQLException &exception) {
    fprintf(stderr, "%s", exception.what());
  }
}

void insertPerformanceTest() {
  auto start = std::chrono::steady_clock::now();
  std::string connInfo = "dbname = keyValueStore";
  int32_t numOfTelegrams = 1000000;
  try {
    db::Connection connection(connInfo);
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

void transActionTest() {
  std::string connInfo = "dbname = keyValueStore";
  try {
    db::Connection connection(connInfo);
    db::Transaction transaction(connection);
    db::Statement query(connection);
    query.execute(
        "INSERT INTO key_value_store (key, value) values('Anna', 'Dominik')");
    transaction.commit(); // finished transaction
    transaction.start();
    query.execute(
        "INSERT INTO key_value_store (key, value) values('Oma', 'Opa')");
    transaction.rollback(); // finished transaction
    transaction.start();
    query.execute("INSERT INTO key_value_store (key,value) values('Katharina', "
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

int main(int argc, char *argv[]) {
  deleteExample();
  insertStatementExample();
  insertExample();
  selectExample();
  deleteExample();
  transActionTest();
  selectExample();
  deleteExample();
  insertPerformanceTest();
  return 0;
}