// ReportGenerator.h
// Formats everything for the console. It only READS data; it never changes
// the configuration (single responsibility: presentation).
#pragma once

#include <iosfwd>
#include <string>

#include "CostModel.h"
#include "RepairEngine.h"
#include "SecurityPolicy.h"
#include "ValidationEngine.h"
#include "Violation.h"

// Everything the full report may contain. Null pointers mean "not available yet".
struct ReportData {
    const SecurityPolicy* original = nullptr;   // configuration BEFORE any repair
    const ViolationList* violations = nullptr;
    const RepairPlan* plan = nullptr;
    const SecurityPolicy* repaired = nullptr;   // simulated or applied result
    const ValidationReport* validation = nullptr;
    bool applied = false;                       // was the repair applied to the live configuration?
    CostModel cost;
    bool pruning = true;
};

class ReportGenerator {
public:
    static void banner(std::ostream& os, const std::string& title);
    static void section(std::ostream& os, const std::string& title);

    static void configuration(std::ostream& os, const SecurityPolicy& p, const CostModel& c, bool pruning);
    static void usersAndRoles(std::ostream& os, const SecurityPolicy& p);
    static void assignments(std::ostream& os, const SecurityPolicy& p);
    static void violations(std::ostream& os, const ViolationList& list);
    // Compliance score of `before`, and of `after` when a repaired configuration is available.
    static void complianceScore(std::ostream& os, const SecurityPolicy& before, const SecurityPolicy* after);
    static void candidates(std::ostream& os, const RepairPlan& plan);
    static void selection(std::ostream& os, const RepairPlan& plan, bool pruning);
    static void appliedRepair(std::ostream& os, const RepairPlan& plan, bool applied);
    static void beforeAfter(std::ostream& os, const SecurityPolicy& before, const SecurityPolicy& after);
    static void validation(std::ostream& os, const ValidationReport& report);

    static void fullReport(std::ostream& os, const ReportData& data);
};
