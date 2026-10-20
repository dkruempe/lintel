#ifndef LINTEL_MESSAGEQUEUEREQUESTREPLY_H
#define LINTEL_MESSAGEQUEUEREQUESTREPLY_H

#include <chrono>
#include <functional>
#include <string>

#include "lintel/features/base/msg/CorrelationId.h"

namespace lintel {

template<typename TReq, typename TResp> class MessageQueueRequestReply
{
public:
  using RequestHandler = std::function<TResp(const TReq &, const std::string &)>;
};

}// namespace lintel
#endif
