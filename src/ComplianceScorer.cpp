#include "ComplianceScorer.h"

#include "ViolationDetector.h"

int ComplianceScorer::pointsFor(Severity severity) {
    switch (severity) {
        case Severity::CRITICAL: return 25;
        case Severity::HIGH: return 15;
        case Severity::MEDIUM: return 8;
        case Severity::LOW: return 3;
    }
    return 0;
}

std::string ComplianceScorer::ratingFor(double overall) {
    if (overall >= 100.0) return "SECURE";
    if (overall >= 90.0) return "LOW RISK";
    if (overall >= 70.0) return "MODERATE RISK";
    if (overall >= 50.0) return "HIGH RISK";
    return "CRITICAL RISK";
}

ComplianceReport ComplianceScorer::evaluate(const SecurityPolicy& policy) {
    ComplianceReport report;
    double total = 0;
    for (const User& user : policy.users()) {
        UserScore us;
        us.userName = user.name();
        int penalty = 0;
        for (const Violation& v : ViolationDetector::detectForUser(policy, user)) {
            penalty += pointsFor(v.severity());
            ++us.violations;
        }
        us.score = penalty >= 100 ? 0 : 100 - penalty;
        total += us.score;
        report.users.push_back(us);
    }
    report.overall = report.users.empty() ? 100.0 : total / static_cast<double>(report.users.size());
    report.rating = ratingFor(report.overall);
    return report;
}
