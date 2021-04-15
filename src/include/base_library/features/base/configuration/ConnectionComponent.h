#ifndef CPP_BASE_LIBRARY_CONNECTIONCOMPONENT_H
#define CPP_BASE_LIBRARY_CONNECTIONCOMPONENT_H

#include "Component.h"

class ConnectionComponent : public Component {
private:
  static struct Shapes {
    const std::string CONFIG_ROOT = "Connections";
    const std::string DATABASE_ROOT = "Connection";
    const std::string CONNECTION_TYPE = "type";
    const std::string CONNECTION_USER_NAME = "user_name";
    const std::string CONNECTION_PASSWORD = "password";
    const std::string CONNECTION_CONNECTION = "connection";
  } shape;

public:
  ConnectionComponent();

  std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                            const std::string &fileName,
                                            int32_t lineOffset) override;
};

#endif // CPP_BASE_LIBRARY_CONNECTIONCOMPONENT_H
