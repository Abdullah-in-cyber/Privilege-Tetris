// Sorting.h
// MERGE SORT (top-down, stable).
//   Idea: split the array in half, sort each half recursively, then merge the
//   two sorted halves by repeatedly taking the smaller front element.
//   Time : O(n log n) in best, average AND worst case.
//   Space: O(n) for the temporary buffer.
//   Stable: equal elements keep their original order (we take from the LEFT
//   half unless the right element is strictly smaller).
// Used to sort: violations by severity, repair candidates by cost,
// and alternative solutions in the report.
#pragma once

#include <cstddef>
#include <vector>

namespace sorting_detail {
template <typename T, typename Less>
void mergeSortRange(std::vector<T>& v, std::vector<T>& tmp, std::size_t lo, std::size_t hi, Less less) {
    if (hi - lo < 2) return;  // 0 or 1 element is already sorted
    std::size_t mid = lo + (hi - lo) / 2;
    mergeSortRange(v, tmp, lo, mid, less);
    mergeSortRange(v, tmp, mid, hi, less);

    std::size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        if (less(v[j], v[i])) tmp[k++] = v[j++];  // right strictly smaller -> take it
        else                  tmp[k++] = v[i++];  // otherwise keep left (stability)
    }
    while (i < mid) tmp[k++] = v[i++];
    while (j < hi)  tmp[k++] = v[j++];
    for (k = lo; k < hi; ++k) v[k] = tmp[k];
}
}  // namespace sorting_detail

// `less(a, b)` must return true when a should come BEFORE b.
template <typename T, typename Less>
void mergeSort(std::vector<T>& v, Less less) {
    std::vector<T> tmp(v);
    sorting_detail::mergeSortRange(v, tmp, 0, v.size(), less);
}
