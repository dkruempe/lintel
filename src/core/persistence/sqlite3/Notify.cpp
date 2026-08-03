#include "base_library/core/persistence/sqlite3/Notify.h"

#include <utility>

namespace sqlite {
    Notify::Notify(Connection &connection, std::function<void()> functionCallBack,
                   std::string tableName)
            : m_tableName(std::move(tableName)),
              m_functionCallBack(std::move(functionCallBack)),
              m_connection(connection) {
        void *thisPtr = static_cast<void *>(this);
        sqlite3_update_hook(m_connection.m_db, callBack, thisPtr);
    }

    void Notify::notify() { m_functionCallBack(); }

    Notify::~Notify() {
        if (m_connection.m_db != nullptr) {
            sqlite3_update_hook(m_connection.m_db, nullptr, nullptr);
        }
    }

    void Notify::callBack(void *arg, int operation, const char *thread,
                          const char *tableName, sqlite3_int64 changes) {
        auto *notify = static_cast<Notify *>(arg);
        // only invoke the callback for the configured table
        if (tableName != nullptr && tableName == notify->m_tableName) {
            notify->notify();
        }
    }
}  // namespace sqlite