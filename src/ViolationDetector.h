// ViolationDetector.h
// Finds violations using SET OPERATIONS. For each user:
//   eff = effective permissions, R = rules of the user's roles
//   FORBIDDEN    = eff ∩ R.forbidden
//   UNAUTHORIZED = (eff \ R.allowed) \ R.forbidden      (excessive privilege)
//   MISSING      = R.required \ eff
// For each role (role-policy inconsistencies):
//   required ∩ forbidden, required \ allowed, grants ∩ forbidden,
//   grants \ allowed, required \ grants
#pragma once

#include <vector>

#include "SecurityPolicy.h"
#include "Violation.h"

class ViolationDetector {
public:
    static std::vector<Violation> detectForUser(const SecurityPolicy& policy, const User& user);
    static std::vector<Violation> detectRoleIssues(const SecurityPolicy& policy);

    // All findings in a custom linked list, sorted by severity (most severe first).
    static ViolationList detectAll(const SecurityPolicy& policy);

    // Merge-sorts the list by severity (descending). Stable.
    static void sortBySeverity(ViolationList& list);
};
