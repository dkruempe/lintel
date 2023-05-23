#ifndef CPP_BASE_LIBRARY_TABLEBUILDER_H
#define CPP_BASE_LIBRARY_TABLEBUILDER_H

#include <algorithm>
#include <array>
#include <numeric>
#include <string>
#include <vector>

struct Column {
    std::vector<std::string> m_rows;
};

template<std::size_t N>
class TableBuilder {
private:
    std::array<Column, N> m_columns;

public:
    void add(std::array<std::string, N> line) {
        std::size_t i = 0;
        for (const auto &iter: line) {
            m_columns[i].m_rows.push_back(iter);
            i++;
        }
    }

    std::string build() {
        // 1. calc max length
        std::array<std::size_t, N> maxLengthPerCol;
        std::size_t i = 0;
        for (const auto &iter: m_columns) {
            auto maxLengthStr = std::max_element(
                    iter.m_rows.begin(), iter.m_rows.end(),
                    [](const auto &a, const auto &b) { return a.length() < b.length(); });
            maxLengthPerCol[i] = maxLengthStr->length();
            i++;
        }
        // 2. sum max length
        std::size_t sumMaxLength =
                std::accumulate(maxLengthPerCol.begin(), maxLengthPerCol.end(), 0);
        sumMaxLength += 4 * N + 1;
        // 3. start printing separator
        std::string separator;
        for (i; i < sumMaxLength; i++) {
            separator += "-";
        }
        separator += "\n";
        // 4 depth of columns
        const std::size_t m = N > 0 ? m_columns[0].m_rows.size() : 0;
        // 5 start printing content
        std::string temp;
        temp += separator;
        bool headerLine = true;
        for (std::size_t j = 0; j < m; j++) {
            std::string lineTemp = "|";
            for (i = 0; i < N; i++) {
                lineTemp += " ";
                std::string val = m_columns[i].m_rows[j];
                lineTemp += val;
                for (std::size_t k = val.length(); k < maxLengthPerCol[i] + 1; k++) {
                    lineTemp += " ";
                }
                lineTemp += "|";
            }
            lineTemp += "\n";
            temp += lineTemp;
            if (headerLine) {
                headerLine = false;
                temp += separator;
            }
        }
        temp += separator;
        return temp;
    }
};

#endif  // CPP_BASE_LIBRARY_TABLEBUILDER_H
