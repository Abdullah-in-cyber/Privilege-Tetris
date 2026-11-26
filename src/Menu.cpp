#include "Menu.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "ComplianceScorer.h"
#include "ConfigIO.h"
#include "Input.h"
#include "ReportGenerator.h"
#include "RoleHierarchy.h"
#include "Scenarios.h"
#include "ViolationDetector.h"

// =====================================================================
//  Non-interactive helpers
// =====================================================================
namespace {
bool scenarioMatches(const Scenario& s, const PipelineResult& r) {
    if (r.userViolations != s.expectedViolations) return false;
    if (s.expectedCost < 0) return !r.plan.solvable;
    return r.plan.solvable && r.plan.totalCost == s.expectedCost && r.simulated && r.validation.passed;
}
}  // namespace

int printScenarioSummary(std::ostream& os, const CostModel& cost, bool pruning) {
    ReportGenerator::banner(os, "PREDEFINED SCENARIO RESULTS");
    os << "  " << std::left << std::setw(4) << "#" << std::setw(40) << "Scenario" << std::setw(11)
       << "Violations" << std::setw(21) << "Min. cost" << std::setw(15) << "Score" << "Result\n" << std::right;
    int failures = 0;
    for (const Scenario& s : buildScenarios()) {
        PipelineResult r = runPipeline(s.policy, cost, pruning);
        bool ok = scenarioMatches(s, r);
        if (!ok) ++failures;
        std::string actual = r.plan.solvable ? std::to_string(r.plan.totalCost) : "NO REPAIR";
        std::string expected = s.expectedCost < 0 ? "NO REPAIR" : std::to_string(s.expectedCost);
        os << "  " << std::left << std::setw(4) << s.id << std::setw(40) << s.title.substr(0, 38) << std::setw(11)
           << (std::to_string(r.userViolations) + "/" + std::to_string(s.expectedViolations)) << std::setw(21)
           << (actual + "/" + expected);
        std::ostringstream score;
        score << std::fixed << std::setprecision(1) << r.scoreBefore << "->";
        if (r.simulated) score << r.scoreAfter; else score << "n/a";
        os << std::setw(15) << score.str() << (ok ? "[PASS]" : "[FAIL]") << "\n" << std::right;
    }
    os << "\n  (Violations and Min. cost show actual/expected; Score = compliance before->after repair)"
       << "  Mismatches: " << failures << "\n";
    return failures;
}

bool printScenarioReport(std::ostream& os, int scenarioId, const CostModel& cost, bool pruning) {
    for (const Scenario& s : buildScenarios()) {
        if (s.id != scenarioId) continue;
        os << "\nSCENARIO " << s.id << ": " << s.title << "\n  " << s.description
           << "\n  Expected: " << s.expectedBehavior << "\n";
        PipelineResult r = runPipeline(s.policy, cost, pruning);
        ViolationList violations = ViolationDetector::detectAll(s.policy);
        ReportData d;
        d.original = &s.policy;
        d.violations = &violations;
        d.plan = &r.plan;
        d.repaired = r.simulated ? &r.repaired : nullptr;
        d.validation = r.simulated ? &r.validation : nullptr;
        d.cost = cost;
        d.pruning = pruning;
        ReportGenerator::fullReport(os, d);
        os << "  Scenario verdict: " << (scenarioMatches(s, r) ? "[PASS] matches expected behaviour"
                                                               : "[FAIL] differs from expected behaviour") << "\n";
        return true;
    }
    return false;
}

// =====================================================================
//  Application
// =====================================================================
Application::Application() {
    loadScenarioIntoSession(3);  // start with the "multiple violations" scenario
    log_.log(AuditCategory::INFO, "Session started with scenario: " + scenarioName_);
}

void Application::invalidate() {
    hasViolations_ = hasPlan_ = hasSim_ = hasValidation_ = false;
    applied_ = false;
    violations_.clear();
    plan_ = RepairPlan();
}

void Application::loadScenarioIntoSession(int index) {
    std::vector<Scenario> all = buildScenarios();
    policy_ = all[static_cast<std::size_t>(index)].policy;
    scenarioName_ = "Scenario " + std::to_string(all[static_cast<std::size_t>(index)].id) + " - " +
                    all[static_cast<std::size_t>(index)].title;
    invalidate();
}

void Application::run() {
    ReportGenerator::banner(std::cout, "PRIVILEGE TETRIS - Minimum-Change Repair Engine");
    std::cout << "  Loaded: " << scenarioName_ << "\n";
    while (true) {
        std::cout << "\n==================== MAIN MENU ====================\n"
                     "  1. View System Configuration\n"
                     "  2. View Users and Roles\n"
                     "  3. View Permission Assignments\n"
                     "  4. Run Security Audit (linear search)\n"
                     "  5. Detect Permission Violations\n"
                     "  6. Generate Repair Candidates\n"
                     "  7. Find Minimum-Cost Repair (backtracking)\n"
                     "  8. Simulate Repair (on a copy)\n"
                     "  9. Validate Repaired Configuration\n"
                     " 10. View Audit Logs\n"
                     " 11. Generate Security Report\n"
                     " 12. Run Predefined Test Scenarios\n"
                     " 13. Apply Validated Repair to Live Configuration\n"
                     " 14. Edit Configuration (users / roles / permissions)\n"
                     " 15. Load / Save Configuration File\n"
                     " 16. Settings (cost model, pruning)\n"
                     " 17. Linear Search Demo\n"
                     "  0. Exit\n"
                     "===================================================\n";
        int choice = Input::integer("Choose an option: ", 0, 17);
        switch (choice) {
            case 1: showConfiguration(); break;
            case 2: showUsersAndRoles(); break;
            case 3: showAssignments(); break;
            case 4: runSecurityAudit(); break;
            case 5: detectViolations(); break;
            case 6: showCandidates(); break;
            case 7: findMinimumRepair(); break;
            case 8: simulateRepair(); break;
            case 9: validateRepair(); break;
            case 10: viewAuditLogs(); break;
            case 11: generateReport(); break;
            case 12: scenarioMenu(); break;
            case 13: applyRepair(); break;
            case 14: editConfiguration(); break;
            case 15: loadSaveMenu(); break;
            case 16: settingsMenu(); break;
            case 17: searchDemo(); break;
            case 0: std::cout << "Goodbye.\n"; return;
        }
    }
}

// ----------------------------------------------------------------- views
void Application::showConfiguration() {
    std::cout << "  Current configuration: " << scenarioName_ << "\n";
    ReportGenerator::configuration(std::cout, policy_, cost_, pruning_);
}
void Application::showUsersAndRoles() { ReportGenerator::usersAndRoles(std::cout, policy_); }
void Application::showAssignments() { ReportGenerator::assignments(std::cout, policy_); }

// Module "audit": for each user and each catalog permission, LINEAR SEARCH the
// user's effective permission list and count the comparisons.
void Application::runSecurityAudit() {
    ReportGenerator::section(std::cout, "SECURITY AUDIT - privilege matrix (linear search)");
    if (policy_.users().empty() || policy_.permissions().empty()) {
        std::cout << "  Nothing to audit: need at least one user and one permission.\n";
        return;
    }
    const auto& perms = policy_.permissions();
    std::cout << "  Columns:";
    for (std::size_t i = 0; i < perms.size(); ++i) std::cout << "  P" << i + 1 << "=" << perms[i].id();
    std::cout << "\n\n  " << std::left << std::setw(12) << "User";
    for (std::size_t i = 0; i < perms.size(); ++i) std::cout << std::setw(5) << ("P" + std::to_string(i + 1));
    std::cout << std::right << " comparisons\n";

    std::size_t totalComparisons = 0;
    for (const User& u : policy_.users()) {
        std::vector<std::string> held = policy_.effectivePermissions(u).toVector();
        EffectiveRules rules = policy_.rulesFor(u);
        std::cout << "  " << std::left << std::setw(12) << u.name();
        std::size_t comparisons = 0;
        for (const Permission& p : perms) {
            SearchResult r = linearSearch(held, [&](const std::string& h) { return h == p.id(); });
            comparisons += r.comparisons;
            std::string mark = ".";
            if (r.found()) mark = rules.forbidden.contains(p.id()) ? "!!" : (rules.allowed.contains(p.id()) ? "X" : "?");
            std::cout << std::setw(5) << mark;
        }
        std::cout << std::right << " " << comparisons << "\n";
        totalComparisons += comparisons;
    }
    std::cout << "\n  Legend: X = held and allowed, !! = held but FORBIDDEN, ? = held but not allowed, . = not held\n"
              << "  Linear search made " << totalComparisons << " comparisons in total.\n";
    detectViolations();
    log_.log(AuditCategory::INFO, "Security audit executed (" + std::to_string(totalComparisons) + " comparisons)");
}

void Application::ensureDetected() {
    if (hasViolations_) return;
    violations_ = ViolationDetector::detectAll(policy_);
    hasViolations_ = true;
    for (const Violation& v : violations_) {
        log_.log(AuditCategory::VIOLATION, std::string(Violation::severityName(v.severity())) + " " +
                 Violation::typeName(v.type()) + ": " + (v.isRoleLevel() ? "" : "user '" + v.userName() + "' ") +
                 v.description());
    }
    ComplianceReport score = ComplianceScorer::evaluate(policy_);
    log_.log(AuditCategory::INFO, "Violation detection finished: " + std::to_string(violations_.size()) +
             " finding(s); compliance score " + std::to_string(static_cast<int>(score.overall + 0.5)) + "/100 [" + score.rating + "]");
}

void Application::detectViolations() {
    if (applied_) invalidate();  // configuration was repaired; analyse the new state afresh
    hasViolations_ = false;      // always re-run when asked explicitly
    ensureDetected();
    ReportGenerator::violations(std::cout, violations_);
    ReportGenerator::complianceScore(std::cout, policy_, nullptr);
}

void Application::ensurePlan() {
    ensureDetected();
    if (hasPlan_) return;
    RepairEngine engine(cost_, pruning_);
    plan_ = engine.findMinimumRepair(policy_);
    hasPlan_ = true;
    hasSim_ = hasValidation_ = false;

    for (const UserRepairResult& r : plan_.perUser) {
        for (const RepairAction& a : r.candidates)
            log_.log(AuditCategory::REPAIR_ACTION, "Candidate generated: " + a.describe());
        log_.log(AuditCategory::EXPLORATION,
                 "User '" + r.userName + "': " + std::to_string(r.stats.explored) + " combinations explored, " +
                 std::to_string(r.stats.rejected) + " rejected, " + std::to_string(r.stats.valid) + " valid, " +
                 std::to_string(r.stats.pruned) + " branches pruned");
        if (r.solvable) {
            std::string chosen;
            for (const RepairAction& a : r.best.actions) chosen += (chosen.empty() ? "" : "; ") + a.describe();
            log_.log(AuditCategory::SELECTION, "User '" + r.userName + "': best repair cost " +
                     std::to_string(r.best.cost) + (chosen.empty() ? "" : " -> " + chosen));
        } else {
            log_.log(AuditCategory::SELECTION, "User '" + r.userName + "': NO valid repair exists");
        }
    }
    log_.log(AuditCategory::SELECTION, plan_.solvable ? "Minimum total repair cost = " + std::to_string(plan_.totalCost)
                                                       : "No complete safe repair exists");
}

void Application::showCandidates() {
    ensurePlan();
    ReportGenerator::candidates(std::cout, plan_);
}

void Application::findMinimumRepair() {
    ensurePlan();
    ReportGenerator::selection(std::cout, plan_, pruning_);
}

bool Application::ensureSimulated() {
    ensurePlan();
    if (!plan_.solvable) return false;
    if (hasSim_) return true;
    simulated_ = ValidationEngine::simulate(policy_, plan_.allActions());  // a COPY; policy_ untouched
    hasSim_ = true;
    hasValidation_ = false;
    for (const RepairAction& a : plan_.allActions())
        log_.log(AuditCategory::EXECUTION, "[SIMULATED] " + a.describe());
    return true;
}

void Application::simulateRepair() {
    if (!ensureSimulated()) {
        std::cout << "  [FAIL] No valid repair exists, so there is nothing to simulate:\n";
        ReportGenerator::selection(std::cout, plan_, pruning_);
        return;
    }
    ReportGenerator::appliedRepair(std::cout, plan_, false);
    ReportGenerator::beforeAfter(std::cout, policy_, simulated_);
    std::cout << "\n  The LIVE configuration has NOT been modified. Use option 9 to validate the simulation.\n";
}

bool Application::ensureValidated() {
    if (!ensureSimulated()) return false;
    if (hasValidation_) return true;
    validation_ = ValidationEngine::validate(policy_, simulated_);
    hasValidation_ = true;
    for (const CheckResult& c : validation_.checks)
        log_.log(AuditCategory::VALIDATION, std::string(c.passed ? "PASS: " : "FAIL: ") + c.name +
                 (c.passed ? "" : " (" + c.details + ")"));
    log_.log(AuditCategory::VALIDATION, validation_.passed ? "Repair validated: configuration is secure"
                                                           : "Repair REJECTED by validation");
    return true;
}

void Application::validateRepair() {
    if (!ensureValidated()) {
        std::cout << "  [FAIL] No valid repair exists, so there is no repaired configuration to validate.\n";
        return;
    }
    ReportGenerator::validation(std::cout, validation_);
}

void Application::applyRepair() {
    if (applied_) { std::cout << "  The repair has already been applied.\n"; return; }
    if (!ensureValidated()) { std::cout << "  [FAIL] No valid repair exists; nothing can be applied.\n"; return; }
    ReportGenerator::appliedRepair(std::cout, plan_, false);
    ReportGenerator::validation(std::cout, validation_);
    if (!validation_.passed) { std::cout << "  [FAIL] Validation failed; repair will not be applied.\n"; return; }
    if (plan_.allActions().empty()) { std::cout << "  Nothing to apply: the system is already secure.\n"; return; }
    if (!Input::yesNo("\nApply this repair to the live configuration?")) {
        std::cout << "  Cancelled. Live configuration unchanged.\n";
        log_.log(AuditCategory::EXECUTION, "Apply cancelled by user");
        return;
    }
    beforeApply_ = policy_;
    policy_ = simulated_;
    applied_ = true;
    for (const RepairAction& a : plan_.allActions())
        log_.log(AuditCategory::EXECUTION, "[APPLIED] " + a.describe());
    log_.log(AuditCategory::EXECUTION, "Repair applied; total cost " + std::to_string(plan_.totalCost));
    std::cout << "  [OK] Repair applied to the live configuration.\n";
}

// ----------------------------------------------------------------- audit logs
void Application::viewAuditLogs() {
    while (true) {
        std::cout << "\n  AUDIT LOG MENU  (" << log_.size() << " entries, stored in a custom linked list)\n"
                     "   1. View all entries\n   2. View one category\n   3. Search by keyword (linear search)\n"
                     "   4. Delete all entries of a category\n   0. Back\n";
        int c = Input::integer("  Choose: ", 0, 4);
        if (c == 0) return;
        if (c == 1) { ReportGenerator::section(std::cout, "AUDIT LOG"); log_.print(std::cout); continue; }
        if (c == 3) {
            std::string kw = Input::line("  Keyword: ");
            if (kw.empty()) { std::cout << "  [!] Keyword must not be empty.\n"; continue; }
            std::size_t comparisons = 0;
            std::size_t hits = log_.search(std::cout, kw, comparisons);
            std::cout << "  " << hits << " match(es); " << comparisons << " node(s) examined.\n";
            continue;
        }
        std::cout << "   1 INFO  2 VIOLATION  3 ACTION  4 EXPLORATION  5 SELECTION  6 EXECUTION  7 VALIDATION\n";
        auto cat = static_cast<AuditCategory>(Input::integer("  Category number: ", 1, 7) - 1);
        if (c == 2) log_.printCategory(std::cout, cat);
        else {
            std::size_t removed = log_.removeCategory(cat);
            std::cout << "  Deleted " << removed << " linked-list node(s).\n";
        }
    }
}

// ----------------------------------------------------------------- report
void Application::generateReport() {
    ensureDetected();
    ensurePlan();
    if (plan_.solvable) ensureValidated();

    ReportData d;
    d.original = applied_ ? &beforeApply_ : &policy_;
    d.violations = &violations_;
    d.plan = &plan_;
    d.repaired = applied_ ? &policy_ : (hasSim_ ? &simulated_ : nullptr);
    d.validation = hasValidation_ ? &validation_ : nullptr;
    d.applied = applied_;
    d.cost = cost_;
    d.pruning = pruning_;
    ReportGenerator::fullReport(std::cout, d);

    if (Input::yesNo("Save this report to a text file?")) {
        std::string path = Input::line("  File name [security_report.txt]: ");
        if (path.empty()) path = "security_report.txt";
        std::ofstream file(path);
        if (!file) { std::cout << "  [!] Cannot write '" << path << "'.\n"; return; }
        ReportGenerator::fullReport(file, d);
        std::cout << "  Report saved to " << path << "\n";
    }
    log_.log(AuditCategory::INFO, "Security report generated");
}

// ----------------------------------------------------------------- scenarios
void Application::scenarioMenu() {
    std::vector<Scenario> all = buildScenarios();
    const int runAll = static_cast<int>(all.size()) + 1;
    while (true) {
        std::cout << "\n  PREDEFINED SCENARIOS\n";
        for (std::size_t i = 0; i < all.size(); ++i) {
            std::cout << "   " << all[i].id << ". " << all[i].title << (all[i].id > 6 ? "   [extension]" : "") << "\n";
        }
        std::cout << "   " << runAll << ". Run ALL scenarios and compare with expected results\n   0. Back\n";
        int c = Input::integer("  Choose: ", 0, runAll);
        if (c == 0) return;
        if (c == runAll) { printScenarioSummary(std::cout, cost_, pruning_); continue; }
        loadScenarioIntoSession(c - 1);
        log_.log(AuditCategory::INFO, "Loaded " + scenarioName_);
        printScenarioReport(std::cout, c, cost_, pruning_);
        std::cout << "\n  The scenario is now the current configuration; options 5-13 work on it.\n";
    }
}

// ----------------------------------------------------------------- editing
void Application::editConfiguration() {
    while (true) {
        std::cout << "\n  EDIT CONFIGURATION\n"
                     "   1. Add permission        2. Add role            3. Add user\n"
                     "   4. Give user a role      5. Remove user's role\n"
                     "   6. Give user a direct permission   7. Revoke user's direct permission\n"
                     "   8. Delete user           9. Clear everything (empty configuration)\n"
                     "  10. Make a role inherit from another role (checks for cycles)\n"
                     "  11. Add a separation-of-duties rule      0. Back\n";
        int c = Input::integer("  Choose: ", 0, 11);
        if (c == 0) return;

        if (c == 9) {
            if (Input::yesNo("  Delete ALL permissions, roles and users?")) {
                policy_ = SecurityPolicy();
                scenarioName_ = "Empty configuration";
                invalidate();
                std::cout << "  Configuration cleared.\n";
            }
            continue;
        }
        bool changed = false;
        if (c == 1) {
            std::string id = Input::name("  Permission id (e.g. reports.view): ");
            std::string desc = Input::line("  Description: ");
            int risk = Input::integer("  Risk weight 1 (harmless) to 10 (critical): ", Permission::kMinRisk, Permission::kMaxRisk);
            if (policy_.addPermission(Permission(id, desc, risk))) { changed = true; std::cout << "  Added.\n"; }
            else std::cout << "  [!] That permission already exists.\n";
        } else if (c == 2) {
            std::string name = Input::name("  Role name: ");
            if (policy_.findRole(name)) { std::cout << "  [!] That role already exists.\n"; continue; }
            Role role(name);
            const char* labels[4] = {"grants", "allowed", "forbidden", "required"};
            bool ok = true;
            for (int k = 0; k < 4 && ok; ++k) {
                for (const std::string& p : Input::words("  " + std::string(labels[k]) + " permissions (space separated, blank = none): ")) {
                    if (!policy_.findPermission(p)) { std::cout << "  [!] Unknown permission '" << p << "'. Role not created.\n"; ok = false; break; }
                    if (k == 0) role.addGrant(p); else if (k == 1) role.addAllowed(p);
                    else if (k == 2) role.addForbidden(p); else role.addRequired(p);
                }
            }
            if (ok) {
                for (const std::string& parent : Input::words("  inherits from roles (blank = none): ")) {
                    if (!policy_.findRole(parent)) { std::cout << "  [!] Unknown role '" << parent << "'. Role not created.\n"; ok = false; break; }
                    role.addParent(parent);  // a brand-new role has no children, so no cycle is possible
                }
            }
            if (ok && policy_.addRole(role)) { changed = true; std::cout << "  Role added.\n"; }
        } else if (c == 3) {
            std::string name = Input::name("  User name: ");
            if (policy_.findUser(name, nullptr)) { std::cout << "  [!] That user already exists.\n"; continue; }
            User user(name);
            bool ok = true;
            for (const std::string& r : Input::words("  Roles (space separated, blank = none): ")) {
                if (!policy_.findRole(r)) { std::cout << "  [!] Unknown role '" << r << "'. User not created.\n"; ok = false; break; }
                user.addRole(r);
            }
            if (ok) {
                for (const std::string& p : Input::words("  Direct permissions (blank = none): ")) {
                    if (!policy_.findPermission(p)) { std::cout << "  [!] Unknown permission '" << p << "'. User not created.\n"; ok = false; break; }
                    user.grantDirect(p);
                }
            }
            if (ok && policy_.addUser(user)) { changed = true; std::cout << "  User added.\n"; }
        } else if (c == 10) {
            if (policy_.roles().size() < 2) { std::cout << "  [!] Need at least two roles.\n"; continue; }
            std::string child = Input::name("  Child role (the one that inherits): ");
            std::string parent = Input::name("  Parent role (inherited from): ");
            std::string error;
            if (RoleHierarchy::addInheritance(policy_, child, parent, error)) { changed = true; std::cout << "  " << child << " now inherits " << parent << ".\n"; }
            else std::cout << "  [!] Rejected: " << error << "\n";
        } else if (c == 11) {
            std::string name = Input::name("  Rule name: ");
            std::vector<std::string> perms = Input::words("  Permissions of which a user may hold at most ONE (at least two): ");
            PermissionSet exclusive;
            bool ok = true;
            for (const std::string& p : perms) {
                if (!policy_.findPermission(p)) { std::cout << "  [!] Unknown permission '" << p << "'. Rule not created.\n"; ok = false; break; }
                exclusive.insert(p);
            }
            if (ok && exclusive.size() < 2) { std::cout << "  [!] A rule needs at least two different permissions.\n"; ok = false; }
            if (ok && !policy_.addSodRule(SodRule(name, exclusive))) { std::cout << "  [!] A rule with that name already exists.\n"; ok = false; }
            if (ok) { changed = true; std::cout << "  Rule added.\n"; }
        } else {
            if (policy_.users().empty()) { std::cout << "  [!] There are no users yet.\n"; continue; }
            std::string uname = Input::name("  User name: ");
            User* user = policy_.findUser(uname);
            if (!user) { std::cout << "  [!] No such user.\n"; continue; }
            if (c == 8) { policy_.removeUser(uname); changed = true; std::cout << "  User deleted.\n"; }
            else if (c == 4 || c == 5) {
                std::string r = Input::name("  Role name: ");
                if (!policy_.findRole(r)) std::cout << "  [!] No such role.\n";
                else if (c == 4) { changed = user->addRole(r); std::cout << (changed ? "  Role given.\n" : "  [!] User already has that role.\n"); }
                else { changed = user->removeRole(r); std::cout << (changed ? "  Role removed.\n" : "  [!] User does not have that role.\n"); }
            } else {  // 6 or 7
                std::string p = Input::name("  Permission id: ");
                if (!policy_.findPermission(p)) std::cout << "  [!] No such permission.\n";
                else if (c == 6) { changed = user->grantDirect(p); std::cout << (changed ? "  Permission given.\n" : "  [!] User already has it directly.\n"); }
                else { changed = user->revokeDirect(p); std::cout << (changed ? "  Permission revoked.\n" : "  [!] User has no such direct permission.\n"); }
            }
        }
        if (changed) {
            scenarioName_ = "Custom configuration";
            invalidate();
            log_.log(AuditCategory::INFO, "Configuration edited manually");
        }
    }
}

void Application::loadSaveMenu() {
    std::cout << "\n  1. Load configuration from file\n  2. Save current configuration to file\n  0. Back\n";
    int c = Input::integer("  Choose: ", 0, 2);
    if (c == 0) return;
    std::string path = Input::line("  File path (e.g. data/scenario1.txt): ");
    if (path.empty()) { std::cout << "  [!] No file name given.\n"; return; }
    std::string error;
    if (c == 1) {
        SecurityPolicy loaded;
        if (!ConfigIO::load(path, loaded, error)) { std::cout << "  [!] Load failed: " << error << "\n"; return; }
        policy_ = loaded;
        scenarioName_ = "File " + path;
        invalidate();
        log_.log(AuditCategory::INFO, "Loaded configuration from " + path);
        std::cout << "  Loaded " << policy_.users().size() << " user(s), " << policy_.roles().size() << " role(s).\n";
    } else {
        if (!ConfigIO::save(path, policy_, error)) std::cout << "  [!] Save failed: " << error << "\n";
        else std::cout << "  Saved to " << path << "\n";
    }
}

void Application::settingsMenu() {
    std::cout << "\n  SETTINGS\n  Cost model : " << cost_.describe() << "\n  Search     : "
              << (pruning_ ? "backtracking with pruning" : "brute force (no pruning)") << "\n"
              << "   1. Toggle uniform / weighted cost\n   2. Set weights\n   3. Toggle pruning\n   0. Back\n";
    int c = Input::integer("  Choose: ", 0, 3);
    if (c == 0) return;
    if (c == 1) cost_.setUniform(!cost_.isUniform());
    else if (c == 2) {
        cost_.setUniform(false);
        cost_.setWeight(RepairType::REVOKE_DIRECT, Input::integer("  Cost of REVOKE (1-100): ", 1, 100));
        cost_.setWeight(RepairType::GRANT_DIRECT, Input::integer("  Cost of GRANT (1-100): ", 1, 100));
        cost_.setWeight(RepairType::REMOVE_ROLE, Input::integer("  Cost of REMOVE-ROLE (1-100): ", 1, 100));
        cost_.setWeight(RepairType::ADD_ROLE, Input::integer("  Cost of ADD-ROLE (1-100): ", 1, 100));
    } else pruning_ = !pruning_;
    hasPlan_ = hasSim_ = hasValidation_ = false;  // results depend on these settings
    applied_ = false;
    std::cout << "  Now: " << cost_.describe() << "; " << (pruning_ ? "pruning ON" : "pruning OFF") << "\n";
}

void Application::searchDemo() {
    std::cout << "\n  LINEAR SEARCH DEMO\n   1. Find a user   2. Find a role   3. Find a permission\n"
                 "   4. Find a violation by user (searches the linked list)   0. Back\n";
    int c = Input::integer("  Choose: ", 0, 4);
    if (c == 0) return;
    std::string key = Input::line("  Search for: ");
    if (key.empty()) { std::cout << "  [!] Nothing to search for.\n"; return; }
    std::size_t steps = 0;
    bool found = false;
    if (c == 1) found = policy_.findUser(key, &steps) != nullptr;
    else if (c == 2) found = policy_.findRole(key, &steps) != nullptr;
    else if (c == 3) found = policy_.findPermission(key, &steps) != nullptr;
    else {
        ensureDetected();
        found = violations_.find([&](const Violation& v) { return v.userName() == key; }, &steps) != nullptr;
    }
    std::cout << "  " << (found ? "FOUND" : "NOT FOUND") << " after " << steps << " comparison(s).\n";
}
