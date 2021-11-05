#ifndef CPP_BASE_LIBRARY_DATABASECONNECTIONCOMPONENT_H
#define CPP_BASE_LIBRARY_DATABASECONNECTIONCOMPONENT_H

#include "Component.h"

class DatabaseConnectionComponent : public Component {
 private:
  static struct Shapes {
    const std::string CONFIG_ROOT = "DatabaseConnections";
    const std::string DATABASE_ROOT = "DatabaseConnection";
    const std::string CONNECTION_NAME = "name";
    const std::string CONNECTION_TYPE = "type";
    const std::string CONNECTION_USER_NAME = "user_name";
    const std::string CONNECTION_PASSWORD = "password";
    const std::string CONNECTION_CONNECTION = "connection";
    const std::string CONNECTION_DATBASE_NAME = "database_name";
    const std::string CONNECTION_PORT = "port";
    const std::string CONNECTION_DEFAULT = "default";
  } shape;

 public:
  DatabaseConnectionComponent();

  std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                            const std::string &fileName,
                                            int32_t lineOffset) override;
};

#endif  // CPP_BASE_LIBRARY_DATABASECONNECTIONCOMPONENT_H
