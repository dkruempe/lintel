#include "base_library/core/persistence/Cursor.h"

namespace db {
    constexpr std::size_t Cursor::BATCH_SIZE;

    void Cursor::start() {
        if (m_started) {
            return;
        }
        m_started = true;
        fillBatch();
    }

    void Cursor::fillBatch() {
        m_batch.clear();
        m_pos = 0;
        std::vector<db::Arguments> rows;
        if (m_cursor != nullptr) {
            rows = m_cursor->fetchNext(BATCH_SIZE);
        } else if (m_cursorSQLite != nullptr) {
            rows = m_cursorSQLite->fetchNext(BATCH_SIZE);
        }
        if (rows.size() < BATCH_SIZE) {
            m_exhausted = true;
        }
        m_batch = std::move(rows);
    }

    bool Cursor::hasCurrent() const { return m_pos < m_batch.size(); }

    const db::Arguments &Cursor::currentOf() const { return m_batch[m_pos]; }

    void Cursor::advance() {
        if (!hasCurrent()) {
            return;
        }
        ++m_pos;
        if (m_pos >= m_batch.size() && !m_exhausted) {
            fillBatch();
        }
    }

    bool Cursor::Iterator::reachedEnd() const {
        return m_end || !m_cursor.hasCurrent();
    }

    Cursor::Iterator::Iterator(Cursor &cursor, bool end)
            : m_cursor(cursor),
              m_end(end) {}

    Cursor::Iterator &Cursor::Iterator::operator++() {
        if (!m_end) {
            m_cursor.advance();
        }
        return *this;
    }

    Cursor::Iterator Cursor::Iterator::operator++(int) {
        Iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    bool Cursor::Iterator::operator==(const Iterator &other) const {
        return reachedEnd() == other.reachedEnd();
    }

    bool Cursor::Iterator::operator!=(const Iterator &other) const {
        return !(*this == other);
    }

    const db::Arguments &Cursor::Iterator::operator*() const {
        return m_cursor.currentOf();
    }

    Cursor::Cursor(std::shared_ptr<postgresql::Cursor> cursor,
                   std::shared_ptr<sqlite::Cursor> cursorSQLite)
            : m_cursor(std::move(cursor)),
              m_cursorSQLite(std::move(cursorSQLite)) {}

    std::vector<db::Arguments> Cursor::fetchNext(std::size_t maxRows) {
        start();
        std::vector<db::Arguments> rows;
        while (rows.size() < maxRows && hasCurrent()) {
            rows.push_back(currentOf());
            advance();
        }
        return rows;
    }

    Cursor::Iterator Cursor::begin() {
        start();
        return Iterator(*this);
    }

    Cursor::Iterator Cursor::end() { return Iterator(*this, true); }
}  // namespace db
