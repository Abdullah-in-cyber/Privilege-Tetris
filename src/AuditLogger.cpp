#include "AuditLogger.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <ostream>

namespace {
std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}
void printEntry(std::ostream& os, const AuditEntry& e) {
    os << "  #" << std::setw(3) << std::setfill('0') << e.id << std::setfill(' ') << "  [" << std::left
       << std::setw(11) << AuditLogger::categoryName(e.category) << std::right << "] " << e.message << "\n";
}
}  // namespace

const char* AuditLogger::categoryName(AuditCategory c) {
    switch (c) {
        case AuditCategory::INFO: return "INFO";
        case AuditCategory::VIOLATION: return "VIOLATION";
        case AuditCategory::REPAIR_ACTION: return "ACTION";
        case AuditCategory::EXPLORATION: return "EXPLORATION";
        case AuditCategory::SELECTION: return "SELECTION";
        case AuditCategory::EXECUTION: return "EXECUTION";
        case AuditCategory::VALIDATION: return "VALIDATION";
    }
    return "?";
}

void AuditLogger::log(AuditCategory category, const std::string& message) {
    entries_.pushBack({nextId_++, category, message});  // O(1) insertion at the tail
}

void AuditLogger::print(std::ostream& os) const {
    if (entries_.empty()) { os << "  (audit log is empty)\n"; return; }
    entries_.forEach([&](const AuditEntry& e) { printEntry(os, e); });  // traversal
}

void AuditLogger::printCategory(std::ostream& os, AuditCategory category) const {
    std::size_t shown = 0;
    entries_.forEach([&](const AuditEntry& e) {
        if (e.category == category) { printEntry(os, e); ++shown; }
    });
    if (shown == 0) os << "  (no entries in this category)\n";
}

std::size_t AuditLogger::search(std::ostream& os, const std::string& keyword,
                                std::size_t& comparisons) const {
    const std::string needle = lower(keyword);
    std::size_t matches = 0;
    comparisons = 0;
    for (const AuditEntry& e : entries_) {  // sequential scan, one node at a time
        ++comparisons;
        if (lower(e.message).find(needle) != std::string::npos) {
            printEntry(os, e);
            ++matches;
        }
    }
    return matches;
}

std::size_t AuditLogger::removeCategory(AuditCategory category) {
    return entries_.removeIf([&](const AuditEntry& e) { return e.category == category; });
}
