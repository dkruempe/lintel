#include "base_library/features/http/service/HttpStatusCodes.h"

HttpStatusCodes::HttpStatusCodes(HttpStatusCodes::Value value)
    : m_value(value) {}
HttpStatusCodes::HttpStatusCodes(int statusCode)
    : m_value(static_cast<Value>(statusCode)) {}
HttpStatusCodes::Value HttpStatusCodes::getValue() { return m_value; }
int HttpStatusCodes::getCode() const { return m_value; }
