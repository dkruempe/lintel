#pragma once

#include <string_view>

namespace lintel {
/** Return the fully qualified name of the given type at compile time.
 *
 * Unlike type_name(), namespaces are kept. Shared memory uses this form to
 * derive a type id, because two types with the same unqualified name in
 * different namespaces would otherwise share an id.
 *
 * @tparam T the type to name
 * @return the fully qualified type name as a string view */
template<typename T> constexpr std::string_view type_name_full()
{
  std::string_view name, prefix, suffix;
#ifdef __clang__
  name = __PRETTY_FUNCTION__;
  prefix = "std::string_view lintel::type_name_full() [T = ";
  suffix = "]";
#elif defined(__GNUC__)
  name = __PRETTY_FUNCTION__;
  prefix = "constexpr std::string_view lintel::type_name_full() [with T = ";
  suffix = "; std::string_view = std::basic_string_view<char>]";
#elif defined(_MSC_VER)
  name = __FUNCSIG__;
  prefix =
    "class std::basic_string_view<char,struct std::char_traits<char> > "
    "__cdecl lintel::type_name_full<";
  suffix = ">(void)";
#endif
  name.remove_prefix(prefix.size());
  name.remove_suffix(suffix.size());
  return name;
}

/** Return a human-readable name for the given type at compile time.
 * @tparam T the type to name
 * @return the unqualified type name as a string view */
template<typename T> constexpr std::string_view type_name()
{
  std::string_view name = type_name_full<T>();
  auto pos = name.rfind("::");
  if (pos != std::string_view::npos) { name.remove_prefix(pos + 2); }
  return name;
}
}// namespace lintel
