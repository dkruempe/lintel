#ifndef CPP_BASE_LIBRARY_ENTRY_H
#define CPP_BASE_LIBRARY_ENTRY_H

#include <string>
#include <utility>
#include <vector>

namespace db {
/**
 * parameter of an statement or prepared statement
 */
    class Parameter {
    private:
        std::string m_value;

        explicit Parameter(std::string value);

    public:
        /**
         * Creates a Parameter from a string value.
         * @param value the parameter value
         * @return a new Parameter instance
         */
        static Parameter with(const std::string &value);

        /** @return the raw string value */
        [[nodiscard]] const std::string &getValue() const;

        /** @return the length of the value string */
        [[nodiscard]] std::size_t getLength() const;

        /** @return the format ID for this parameter type */
        static int32_t getFormatId();
    };

    /**
     * A collection of Parameter objects for use in query execution.
     */
    class Parameters {
    private:
        std::vector<Parameter> m_params;

        static std::vector<Parameter> init(const std::vector<std::string> &params);

    public:
        /**
         * Constructs Parameters from a list of string values.
         * @param params the parameter values
         */
        explicit Parameters(const std::vector<std::string> &params);

        /**
         * Constructs Parameters from a list of Parameter objects.
         * @param params the parameter objects
         */
        explicit Parameters(std::vector<Parameter> params);

        /** @return vector of C-string pointers to the parameter values */
        [[nodiscard]] std::vector<const char *> getParameters() const;

        /** @return vector of parameter value lengths */
        [[nodiscard]] std::vector<int32_t> getParametersLengths() const;

        /** @return vector of parameter type format IDs */
        [[nodiscard]] std::vector<int32_t> getParametersTypes() const;
    };
}  // namespace db

#endif  // CPP_BASE_LIBRARY_ENTRY_H
