// ValidationEngine.h
// Decides whether a repaired configuration is acceptable.
// It is used in two places:
//   1. inside the backtracking search, to accept/reject each candidate solution;
//   2. after the search, to re-verify the final simulated configuration.
//
// Per-user checks (BEFORE = original user, AFTER = repaired user):
//   forbiddenRemoved  : AFTER holds nothing its rules forbid
//   requiredPreserved : every permission its rules require is present
//   allowedSatisfied  : AFTER holds nothing outside its allowed set
//   sodSatisfied      : no separation-of-duties rule is broken
//   noNewViolations   : every violation after the repair already existed before
//   accessPreserved   : legitimate access BEFORE (allowed and not forbidden) is kept; for a
//                       broken separation-of-duties rule at least one of its permissions is kept
//   noEscalation      : AFTER gains no new permission except originally-required ones
#pragma once

#include <string>
#include <vector>

#include "RepairAction.h"
#include "SecurityPolicy.h"

struct UserCheck {
    bool forbiddenRemoved = true;
    bool requiredPreserved = true;
    bool allowedSatisfied = true;
    bool sodSatisfied = true;
    bool noNewViolations = true;
    bool accessPreserved = true;
    bool noEscalation = true;
    std::vector<std::string> failures;  // short labels of failed checks

    bool passed() const {
        return forbiddenRemoved && requiredPreserved && allowedSatisfied && sodSatisfied && noNewViolations &&
               accessPreserved && noEscalation;
    }
};

struct CheckResult {
    std::string name;
    bool passed;
    std::string details;  // offending users/permissions, empty when passed
};

struct ValidationReport {
    std::vector<CheckResult> checks;
    bool passed = false;
    int remainingViolations = 0;
};

class ValidationEngine {
public:
    static UserCheck checkUser(const SecurityPolicy& beforePolicy, const User& before,
                               const SecurityPolicy& afterPolicy, const User& after);

    // Copy the original and apply the actions to the COPY (original untouched).
    static SecurityPolicy simulate(const SecurityPolicy& original,
                                   const std::vector<RepairAction>& actions);

    // Full validation of a repaired configuration against the original.
    static ValidationReport validate(const SecurityPolicy& original, const SecurityPolicy& repaired);
};
