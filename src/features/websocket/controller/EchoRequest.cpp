#include "base_library/features/websocket/controller/EchoRequest.h"

#include "base_library/core/utils/TypeName.h"
#include "base_library/core/utils/UUID.h"

EchoRequest::EchoRequest()
    : Request(std::string(type_name<EchoRequest>()), "", UUID::generate()) {}
