// Menu.h
// The interactive console application. It owns the session state (current
// configuration, last detection / plan / simulation / validation, audit log)
// and connects the menu choices to the engines. All real work is delegated.
#pragma once

#include <iosfwd>

#include "AuditLogger.h"
#include "CostModel.h"
#include "RepairEngine.h"
#include "SecurityPolicy.h"
#include "ValidationEngine.h"
#include "Violation.h"

class Application {
public:
    Application();
    void run();  // main menu loop; returns when the user exits or input ends

private:
    // ----- menu actions (numbers match the on-screen menu) -----
    void showConfiguration();
    void showUsersAndRoles();
    void showAssignments();
    void runSecurityAudit();
    void detectViolations();
    void showCandidates();
    void findMinimumRepair();
    void simulateRepair();
    void validateRepair();
    void viewAuditLogs();
    void generateReport();
    void scenarioMenu();
    void applyRepair();
    void editConfiguration();
    void loadSaveMenu();
    void settingsMenu();
    void searchDemo();

    // ----- lazily compute whatever the next step needs -----
    void ensureDetected();
    void ensurePlan();
    bool ensureSimulated();   // false when the plan has no valid repair
    bool ensureValidated();
    void invalidate();        // configuration changed: forget derived results
    void loadScenarioIntoSession(int index);

    SecurityPolicy policy_;      // the LIVE configuration
    SecurityPolicy beforeApply_; // snapshot taken just before a repair is applied
    SecurityPolicy simulated_;   // repaired COPY (live configuration untouched)
    ViolationList violations_;
    RepairPlan plan_;
    ValidationReport validation_;
    bool hasViolations_ = false, hasPlan_ = false, hasSim_ = false, hasValidation_ = false;
    bool applied_ = false;

    CostModel cost_;
    bool pruning_ = true;
    AuditLogger log_;
    std::string scenarioName_;
};

// Non-interactive helpers (also used by command-line switches and tests).
// Returns the number of scenarios whose actual result differs from the expected one.
int printScenarioSummary(std::ostream& os, const CostModel& cost, bool pruning);
bool printScenarioReport(std::ostream& os, int scenarioId, const CostModel& cost, bool pruning);
