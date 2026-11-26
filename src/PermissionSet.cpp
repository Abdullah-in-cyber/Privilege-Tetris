#include "PermissionSet.h"

#include <algorithm>
#include <iterator>

PermissionSet::PermissionSet(const std::vector<std::string>& items)
    : items_(items.begin(), items.end()) {}

bool PermissionSet::insert(const std::string& id) { return items_.insert(id).second; }

bool PermissionSet::erase(const std::string& id) { return items_.erase(id) > 0; }

bool PermissionSet::contains(const std::string& id) const { return items_.count(id) > 0; }

PermissionSet PermissionSet::unionWith(const PermissionSet& other) const {
    PermissionSet result;
    std::set_union(items_.begin(), items_.end(), other.items_.begin(), other.items_.end(),
                   std::inserter(result.items_, result.items_.begin()));
    return result;
}

PermissionSet PermissionSet::intersectionWith(const PermissionSet& other) const {
    PermissionSet result;
    std::set_intersection(items_.begin(), items_.end(), other.items_.begin(), other.items_.end(),
                          std::inserter(result.items_, result.items_.begin()));
    return result;
}

PermissionSet PermissionSet::minus(const PermissionSet& other) const {
    PermissionSet result;
    std::set_difference(items_.begin(), items_.end(), other.items_.begin(), other.items_.end(),
                        std::inserter(result.items_, result.items_.begin()));
    return result;
}

bool PermissionSet::isSubsetOf(const PermissionSet& other) const {
    return std::includes(other.items_.begin(), other.items_.end(), items_.begin(), items_.end());
}

std::vector<std::string> PermissionSet::toVector() const {
    return std::vector<std::string>(items_.begin(), items_.end());
}

std::string PermissionSet::toString() const {
    std::string out = "{";
    bool first = true;
    for (const std::string& p : items_) {
        if (!first) out += ", ";
        out += p;
        first = false;
    }
    return out + "}";
}
