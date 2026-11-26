// AuditLogger.h
// Keeps the complete audit history in the custom DoublyLinkedList.
// Demonstrates: insertion (log), traversal (print), linear search by keyword,
// and deletion (remove all entries of a category).
#pragma once

#include <iosfwd>
#include <string>

#include "LinkedList.h"

enum class AuditCategory {
    INFO,
    VIOLATION,
    REPAIR_ACTION,
    EXPLORATION,
    SELECTION,
    EXECUTION,
    VALIDATION
};

struct AuditEntry {
    std::size_t id;
    AuditCategory category;
    std::string message;
};

class AuditLogger {
public:
    void log(AuditCategory category, const std::string& message);

    std::size_t size() const { return entries_.size(); }
    void print(std::ostream& os) const;
    void printCategory(std::ostream& os, AuditCategory category) const;

    // Linear search over the list (case-insensitive). Returns number of matches;
    // `comparisons` receives how many nodes were examined.
    std::size_t search(std::ostream& os, const std::string& keyword, std::size_t& comparisons) const;

    // Deletes all entries of one category; returns how many nodes were removed.
    std::size_t removeCategory(AuditCategory category);
    void clear() { entries_.clear(); }

    static const char* categoryName(AuditCategory c);

private:
    DoublyLinkedList<AuditEntry> entries_;
    std::size_t nextId_ = 1;
};
