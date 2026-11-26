// ComplianceScorer.h
// Turns the list of violations into a 0-100 compliance score.
//   Each user starts at 100 and loses points per violation by severity:
//       CRITICAL 25   HIGH 15   MEDIUM 8   LOW 3        (never below 0)
//   System score = average of the user scores (100 when there are no users).
//   Rating: 100 SECURE | >=90 LOW RISK | >=70 MODERATE RISK | >=50 HIGH RISK | else CRITICAL RISK
// Role-level advisories (role definitions) are listed separately and are NOT scored,
// because the repair engine only changes users.
#pragma once

#include <string>
#include <vector>

#include "SecurityPolicy.h"
#include "Violation.h"

struct UserScore {
    std::string userName;
    int score = 100;
    int violations = 0;
};

struct ComplianceReport {
    std::vector<UserScore> users;
    double overall = 100.0;
    std::string rating;
};

class ComplianceScorer {
public:
    static ComplianceReport evaluate(const SecurityPolicy& policy);
    static int pointsFor(Severity severity);
    static std::string ratingFor(double overall);
};
