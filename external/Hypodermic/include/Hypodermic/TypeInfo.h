#pragma once

#include <string>
#include <typeinfo>

#if defined(__GNUC__)

#include <cxxabi.h>

#endif /* __GNUC__ */

namespace Hypodermic {

    struct TypeInfo {
        explicit TypeInfo(const std::type_info &typeInfo)
                : m_typeInfo(&typeInfo),
                  m_fullyQualifiedName(dotNetify(demangleTypeName(m_typeInfo->name()))) {}

        const std::type_info &intrinsicTypeInfo() const { return *m_typeInfo; }

        const std::string &fullyQualifiedName() const { return m_fullyQualifiedName; }

        bool operator==(const TypeInfo &rhs) const {
            return intrinsicTypeInfo() == rhs.intrinsicTypeInfo();
        }

        static std::string dotNetify(const std::string &typeName) {
            std::string result;
            result.reserve(typeName.size());
            for (std::string::size_type i = 0; i < typeName.size();) {
                if (typeName.compare(i, 2, "::") == 0) {
                    result += '.';
                    i += 2;
                } else {
                    result += typeName[i];
                    ++i;
                }
            }
            return result;
        }

        static std::string demangleTypeName(const std::string &typeName) {
#if defined(__GNUC__)
            int status;

            auto demangledName = abi::__cxa_demangle(typeName.c_str(), 0, 0, &status);
            if (demangledName == nullptr) return typeName;

            std::string result = demangledName;
            free(demangledName);
            return result;
#else
            // Only non-GNU compilers demangle by hand, so <regex> is needed on
            // those toolchains only. Keeping it out of the GNU branch removes
            // ~72k preprocessed lines from every including translation unit.
            #include <regex>
            std::string demangled = typeName;
            demangled = std::regex_replace(
                demangled, std::regex("(const\\s+|\\s+const)"), std::string());
            demangled = std::regex_replace(
                demangled, std::regex("(volatile\\s+|\\s+volatile)"), std::string());
            demangled = std::regex_replace(
                demangled, std::regex("(static\\s+|\\s+static)"), std::string());
            demangled = std::regex_replace(
                demangled, std::regex("(class\\s+|\\s+class)"), std::string());
            demangled = std::regex_replace(
                demangled, std::regex("(struct\\s+|\\s+struct)"), std::string());
            return demangled;
#endif /* defined(__GNUC__) */
        }

    private:
        const std::type_info *m_typeInfo;
        std::string m_fullyQualifiedName;
    };

    namespace Utils {

        template<class T>
        const TypeInfo &getMetaTypeInfo() {
            static TypeInfo result(typeid(T));
            return result;
        }

    }  // namespace Utils
}  // namespace Hypodermic

#include <functional>
#include <typeindex>

namespace std {

    template<>
    struct hash<Hypodermic::TypeInfo> {
        typedef Hypodermic::TypeInfo argument_type;
        typedef size_t result_type;

        size_t operator()(const Hypodermic::TypeInfo &value) const {
            return hash<type_index>()(type_index(value.intrinsicTypeInfo()));
        }
    };

}  // namespace std