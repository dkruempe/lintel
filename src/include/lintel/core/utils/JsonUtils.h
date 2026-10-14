#ifndef LINTEL_JSONUTILS_H
#define LINTEL_JSONUTILS_H

#include <cstdint>
#include <string>

#include <rapidjson/document.h>

namespace lintel::json {
/**
 * Hardening helpers for reading JSON from untrusted clients.
 *
 * rapidjson's HasMember/operator[]/Get* assert on a type mismatch in debug and
 * exhibit undefined behaviour in release: a non-object value reaches
 * MemberBegin() and a wrong-typed member reaches the accessor's
 * reinterpret_cast. Both are reachable from the network, so DTOs read members
 * through these helpers, which report a missing or mistyped member as `false`
 * instead of crashing. An object that is not an object at all is likewise
 * treated as having no members.
 */
namespace detail {
  inline const rapidjson::Value *findMember(const rapidjson::Value &obj, const char *key)
  {
    if (!obj.IsObject()) { return nullptr; }
    const auto iter = obj.FindMember(key);
    if (iter == obj.MemberEnd()) { return nullptr; }
    return &iter->value;
  }
}// namespace detail

/** @return the member value, or nullptr if @p obj is not an object or has no @p key. */
inline const rapidjson::Value *member(const rapidjson::Value &obj, const char *key)
{
  return detail::findMember(obj, key);
}

/** @return true iff the member exists and is a JSON string. */
inline bool getString(const rapidjson::Value &obj, const char *key, std::string &out)
{
  const rapidjson::Value *value = detail::findMember(obj, key);
  if (value == nullptr || !value->IsString()) { return false; }
  out = std::string(value->GetString(), value->GetStringLength());
  return true;
}

/** @return true iff the member exists and is a JSON boolean. */
inline bool getBool(const rapidjson::Value &obj, const char *key, bool &out)
{
  const rapidjson::Value *value = detail::findMember(obj, key);
  if (value == nullptr || !value->IsBool()) { return false; }
  out = value->GetBool();
  return true;
}

/** @return true iff the member exists and is a 32-bit JSON integer. */
inline bool getInt(const rapidjson::Value &obj, const char *key, std::int32_t &out)
{
  const rapidjson::Value *value = detail::findMember(obj, key);
  if (value == nullptr || !value->IsInt()) { return false; }
  out = value->GetInt();
  return true;
}

/** @return true iff the member exists and is a JSON integer (any width). */
inline bool getInt64(const rapidjson::Value &obj, const char *key, std::int64_t &out)
{
  const rapidjson::Value *value = detail::findMember(obj, key);
  if (value == nullptr) { return false; }
  if (value->IsInt64()) {
    out = value->GetInt64();
  } else if (value->IsInt()) {
    out = value->GetInt();
  } else if (value->IsUint()) {
    out = static_cast<std::int64_t>(value->GetUint());
  } else if (value->IsUint64()) {
    out = static_cast<std::int64_t>(value->GetUint64());
  } else {
    return false;
  }
  return true;
}

/** @return true iff the member exists and is a non-negative JSON integer (any width). */
inline bool getUint64(const rapidjson::Value &obj, const char *key, std::uint64_t &out)
{
  const rapidjson::Value *value = detail::findMember(obj, key);
  if (value == nullptr) { return false; }
  if (value->IsUint64()) {
    out = value->GetUint64();
  } else if (value->IsUint()) {
    out = value->GetUint();
  } else if (value->IsInt64()) {
    const std::int64_t signedValue = value->GetInt64();
    if (signedValue < 0) { return false; }
    out = static_cast<std::uint64_t>(signedValue);
  } else if (value->IsInt()) {
    if (value->GetInt() < 0) { return false; }
    out = static_cast<std::uint64_t>(value->GetInt());
  } else {
    return false;
  }
  return true;
}

/** @return true iff the member exists and is a JSON number. */
inline bool getDouble(const rapidjson::Value &obj, const char *key, double &out)
{
  const rapidjson::Value *value = detail::findMember(obj, key);
  if (value == nullptr || !value->IsNumber()) { return false; }
  out = value->GetDouble();
  return true;
}
}// namespace lintel::json

#endif// LINTEL_JSONUTILS_H
