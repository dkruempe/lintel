#ifndef CPP_BASE_LIBRARY_PAGE_H
#define CPP_BASE_LIBRARY_PAGE_H

#include <optional>
#include <string>
#include <utility>
#include <vector>

/**
 * A single page of keyset-paginated results.
 * @tparam T the element type of the page
 */
template<typename T>
class Page {
private:
    /** The items belonging to this page */
    std::vector<T> m_items;
    /** True if further items exist after this page */
    bool m_hasMore;
    /** Sort key of the last item; use as 'after' to request the next page */
    std::optional<std::string> m_nextAfter;

public:
    /**
     * Constructs a page.
     * @param items the items of this page
     * @param hasMore true if more items follow
     * @param nextAfter the sort key of the last item (nullopt for an empty page)
     */
    Page(std::vector<T> items, const bool hasMore,
         std::optional<std::string> nextAfter)
        : m_items(std::move(items)),
          m_hasMore(hasMore),
          m_nextAfter(std::move(nextAfter)) {}

    /** @return the items of this page */
    [[nodiscard]] const std::vector<T> &getItems() const { return m_items; }

    /** @return true if further items exist after this page */
    [[nodiscard]] bool hasMore() const { return m_hasMore; }

    /** @return the sort key of the last item, nullopt if the page is empty */
    [[nodiscard]] const std::optional<std::string> &getNextAfter() const {
        return m_nextAfter;
    }
};

#endif  // CPP_BASE_LIBRARY_PAGE_H
