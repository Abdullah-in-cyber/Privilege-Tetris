// LinearSearch.h
// Linear (sequential) search: look at each element from the front until the
// predicate matches. Works on unsorted data. Best O(1), average O(n), worst O(n).
// The result also reports how many comparisons were made so the real
// execution of the algorithm can be demonstrated in the console.
#pragma once

#include <cstddef>
#include <vector>

struct SearchResult {
    long index = -1;               // position of the match, -1 if not found
    std::size_t comparisons = 0;   // elements examined
    bool found() const { return index >= 0; }
};

template <typename T, typename Pred>
SearchResult linearSearch(const std::vector<T>& items, Pred matches) {
    SearchResult result;
    for (std::size_t i = 0; i < items.size(); ++i) {
        ++result.comparisons;
        if (matches(items[i])) {
            result.index = static_cast<long>(i);
            return result;
        }
    }
    return result;
}
