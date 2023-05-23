#ifndef CPP_BASE_LIBRARY_SQLITE_NOTIFY_H
#define CPP_BASE_LIBRARY_SQLITE_NOTIFY_H

#include <functional>
#include <set>

#include "Connection.h"

namespace sqlite {
    class Notify {
    private:
        std::string m_tableName;
        std::function<void()> m_functionCallBack;
        Connection &m_connection;

        static void callBack(void *arg, int operation, const char *thread,
                             const char *tableName, sqlite3_int64 changes);

        void notify();

    public:
        explicit Notify(Connection &connection,
                        std::function<void()> functionCallBack,
                        std::string tableName);
    };
}  // namespace sqlite

#endif  // CPP_BASE_LIBRARY_SQLITE_NOTIFY_H
