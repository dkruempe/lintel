#include "base_library/core/persistence/Cursor.h"

#include "base_library/core/persistence/postgresql/Statement.h"

namespace db {

    void Cursor::initBackendCursor() {
        if (!m_needsInit) {
            return;
        }
        m_needsInit = false;

        if (m_pgConn) {
            // PostgreSQL
            std::string pgQuery =
                    postgresql::Statement::initStatement(m_query);
            if (m_fetchSize > 0) {
                m_cursor = std::make_shared<postgresql::Cursor>(
                        *m_pgConn, pgQuery, m_params,
                        m_fetchSize, m_cursorName, m_noScroll);
            } else {
                m_cursor = std::make_shared<postgresql::Cursor>(
                        *m_pgConn, pgQuery, m_params);
            }
        } else if (m_sqliteConn) {
            // SQLite
            m_cursorSQLite = std::make_shared<sqlite::Cursor>(
                    *m_sqliteConn, m_query, m_params,
                    m_fetchSize > 0 ? m_fetchSize : 64);
        }
    }

    void Cursor::start() {
        if (m_started) {
            return;
        }
        m_started = true;
        initBackendCursor();
        fillBatch();
    }

    void Cursor::fillBatch() {
        m_batch.clear();
        m_pos = 0;
        std::vector<db::Arguments> rows;
        const std::size_t batchSize = m_fetchSize > 0 ? m_fetchSize : 64;
        if (m_cursor != nullptr) {
            rows = m_cursor->fetchNext(batchSize);
        } else if (m_cursorSQLite != nullptr) {
            rows = m_cursorSQLite->fetchNext(batchSize);
        }
        if (rows.size() < batchSize) {
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

    // ── Configuration (must be called before start()) ────────────────

    Cursor &Cursor::setFetchSize(std::size_t rows) {
        m_fetchSize = rows;
        return *this;
    }

    Cursor &Cursor::setCursorName(const std::string &name) {
        m_cursorName = name;
        return *this;
    }

    Cursor &Cursor::setNoScroll(bool enable) {
        m_noScroll = enable;
        return *this;
    }

    // ── Consumption ──────────────────────────────────────────────────

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
