#include "base_library/core/persistence/Result.h"

#include <utility>

import base_library.core.exceptions;

namespace db {
    std::string Result::getValue(int row, int attribute) const {
        if (m_result != nullptr) {
            return m_result->getValue(row, attribute);
        }
        if (m_resultSQLite != nullptr) {
            return m_resultSQLite->getValue(row, attribute);
        }
        return {};
    }

    int Result::getNumOfAttributes() const {
        if (m_result != nullptr) {
            return m_result->getNumOfAttributes();
        }
        if (m_resultSQLite != nullptr) {
            return m_resultSQLite->getNumOfAttributes();
        }
        return 0;
    }

    int Result::getSize() const {
        if (m_result != nullptr) {
            return m_result->getSize();
        }
        if (m_resultSQLite != nullptr) {
            return m_resultSQLite->getSize();
        }
        return 0;
    }

    Result::Result(std::shared_ptr<postgresql::Result> result)
            : m_result(std::move(result)), m_resultSQLite(nullptr) {}

    Result::Result(std::shared_ptr<sqlite::Result> result)
            : m_result(nullptr), m_resultSQLite(std::move(result)) {}

    Arguments &Result::of(std::size_t pos) {
        if (m_result != nullptr) {
            return m_result->of(pos);
        }
        if (m_resultSQLite != nullptr) {
            return m_resultSQLite->of(pos);
        }
        throw db::SQLException("Result is not initialized");
    }

    const Arguments &Result::of(std::size_t pos) const {
        if (m_result != nullptr) {
            return m_result->of(pos);
        }
        if (m_resultSQLite != nullptr) {
            return m_resultSQLite->of(pos);
        }
        throw db::SQLException("Result is not initialized");
    }

    Result::~Result() = default;
}  // namespace db