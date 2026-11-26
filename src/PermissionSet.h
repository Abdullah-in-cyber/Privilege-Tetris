// PermissionSet.h
// A set of permission ids with the classic set operations:
// union, intersection, difference and membership checking.
// Backed by std::set (a balanced tree), so every set is kept sorted and the
// std::set_union / set_intersection / set_difference algorithms (which need
// sorted input) can be used directly.
#pragma once

#include <initializer_list>
#include <set>
#include <string>
#include <vector>

class PermissionSet {
public:
    PermissionSet() = default;
    PermissionSet(std::initializer_list<std::string> items) : items_(items) {}
    explicit PermissionSet(const std::vector<std::string>& items);

    bool insert(const std::string& id);          // true if newly added
    bool erase(const std::string& id);           // true if it was present
    bool contains(const std::string& id) const;  // membership check, O(log n)
    bool empty() const { return items_.empty(); }
    std::size_t size() const { return items_.size(); }

    PermissionSet unionWith(const PermissionSet& other) const;         // A ∪ B
    PermissionSet intersectionWith(const PermissionSet& other) const;  // A ∩ B
    PermissionSet minus(const PermissionSet& other) const;             // A \ B
    bool isSubsetOf(const PermissionSet& other) const;                 // A ⊆ B

    const std::set<std::string>& items() const { return items_; }
    std::vector<std::string> toVector() const;
    std::string toString() const;  // "{a, b, c}" or "{}"

    bool operator==(const PermissionSet& other) const { return items_ == other.items_; }
    bool operator!=(const PermissionSet& other) const { return !(*this == other); }

private:
    std::set<std::string> items_;
};
