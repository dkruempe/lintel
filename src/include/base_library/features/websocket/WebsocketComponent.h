#ifndef CPP_BASE_LIBRARY_WEBSOCKETCOMPONENT_H
#define CPP_BASE_LIBRARY_WEBSOCKETCOMPONENT_H

#include "base_library/features/base/configuration/Component.h"

class WebsocketComponent : public Component {
 private:
  enum TYPE { Server, Client, UNDEFINED };
  static struct Shapes {
    const std::string CONFIG_ROOT = "Websockets";
    const std::string WEBSOCKET_ROOT = "Websocket";
    const std::string PORT = "port";
    const std::string ADDRESS = "address";
    const std::string TYPE = "type";
    const std::string NAME = "name";
  } shape;

 public:
  WebsocketComponent();

  std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                            const std::string &fileName,
                                            int32_t lineOffset) override;
};

#endif  // CPP_BASE_LIBRARY_WEBSOCKETCOMPONENT_H
