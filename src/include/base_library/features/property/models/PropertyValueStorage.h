#ifndef CPP_BASE_LIBRARY_PROPERTYVALUESTORAGE_H
#define CPP_BASE_LIBRARY_PROPERTYVALUESTORAGE_H

#include <atomic>
#include <shared_mutex>
#include <type_traits>

/**
 * Detects whether std::atomic<T> is guaranteed to be lock-free.
 * std::atomic<T> is only instantiated for trivially copyable types, so the
 * check is performed through a partial specialization.
 */
template<class T, class Enable = void>
struct IsLockFreeAtomic : std::false_type {};

template<class T>
struct IsLockFreeAtomic<T,
        std::enable_if_t<std::is_trivially_copyable_v<T>>>
    : std::bool_constant<std::atomic<T>::is_always_lock_free> {};

/**
 * Storage for a property value.
 *
 * For trivially copyable types whose std::atomic<T> is always lock-free the
 * value is held in a std::atomic and read/written without any lock, so that it
 * can be accessed from real-time contexts. All other types (e.g. std::string)
 * fall back to a reader-writer lock.
 */
template<class T, class Enable = void>
class PropertyValueStorage {
private:
    mutable std::shared_mutex m_mutex;
    T m_value;

public:
    PropertyValueStorage() : m_value() {}

    explicit PropertyValueStorage(const T &value) : m_value(value) {}

    /** @return the current value (lock-free if the specialization applies) */
    T load() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_value;
    }

    /** Set the value */
    void store(const T &value) {
        std::lock_guard<std::shared_mutex> lock(m_mutex);
        m_value = value;
    }

    /** @return true if reads/writes are guaranteed lock-free */
    [[nodiscard]] static constexpr bool isLockFree() { return false; }
};

/** Lock-free specialization for trivially copyable, always lock-free types. */
template<class T>
class PropertyValueStorage<T,
        std::enable_if_t<IsLockFreeAtomic<T>::value>> {
private:
    std::atomic<T> m_value;

public:
    PropertyValueStorage() : m_value() {}

    explicit PropertyValueStorage(const T &value) : m_value(value) {}

    /** @return the current value (lock-free atomic load) */
    T load() const { return m_value.load(std::memory_order_relaxed); }

    /** Set the value (lock-free atomic store) */
    void store(const T &value) { m_value.store(value, std::memory_order_relaxed); }

    /** @return true if reads/writes are guaranteed lock-free */
    [[nodiscard]] static constexpr bool isLockFree() {
        return std::atomic<T>::is_always_lock_free;
    }
};

#endif  // CPP_BASE_LIBRARY_PROPERTYVALUESTORAGE_H
