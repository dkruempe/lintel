#include "base_library/features/websocket/controller/EchoResponse.h"
EchoResponse::EchoResponse(const std::string &id) : Response("ACK", "", id) {}
