// Violation.h
// One security problem found by the ViolationDetector.
#pragma once

#include <string>

#include "LinkedList.h"

enum class ViolationType {
    FORBIDDEN_PERMISSION,       // user holds something a role forbids
    UNAUTHORIZED_PERMISSION,    // user holds something outside every "allowed" set (excessive privilege)
    MISSING_REQUIRED,           // user lacks something their roles require
    ROLE_POLICY_INCONSISTENCY,  // the role definition contradicts itself
    SOD_CONFLICT                // user holds two permissions a separation-of-duties rule keeps apart
};

enum class Severity { LOW = 1, MEDIUM = 2, HIGH = 3, CRITICAL = 4 };

class Violation {
public:
    Violation(ViolationType type, std::string user, std::string role, std::string permission,
              std::string description)
        : type_(type), user_(std::move(user)), role_(std::move(role)),
          permission_(std::move(permission)), description_(std::move(description)) {}

    ViolationType type() const { return type_; }
    const std::string& userName() const { return user_; }   // empty for role-level findings
    const std::string& roleName() const { return role_; }
    const std::string& permission() const { return permission_; }
    const std::string& description() const { return description_; }
    bool isRoleLevel() const { return user_.empty(); }

    Severity severity() const {
        switch (type_) {
            case ViolationType::FORBIDDEN_PERMISSION: return Severity::CRITICAL;
            case ViolationType::SOD_CONFLICT: return Severity::CRITICAL;
            case ViolationType::UNAUTHORIZED_PERMISSION: return Severity::HIGH;
            case ViolationType::ROLE_POLICY_INCONSISTENCY: return Severity::HIGH;
            case ViolationType::MISSING_REQUIRED: return Severity::MEDIUM;
        }
        return Severity::LOW;
    }

    static const char* typeName(ViolationType t) {
        switch (t) {
            case ViolationType::FORBIDDEN_PERMISSION: return "FORBIDDEN";
            case ViolationType::UNAUTHORIZED_PERMISSION: return "UNAUTHORIZED";
            case ViolationType::MISSING_REQUIRED: return "MISSING";
            case ViolationType::ROLE_POLICY_INCONSISTENCY: return "ROLE-POLICY";
            case ViolationType::SOD_CONFLICT: return "SOD";
        }
        return "?";
    }
    static const char* severityName(Severity s) {
        switch (s) {
            case Severity::CRITICAL: return "CRITICAL";
            case Severity::HIGH: return "HIGH";
            case Severity::MEDIUM: return "MEDIUM";
            case Severity::LOW: return "LOW";
        }
        return "?";
    }

private:
    ViolationType type_;
    std::string user_, role_, permission_, description_;
};

using ViolationList = DoublyLinkedList<Violation>;
