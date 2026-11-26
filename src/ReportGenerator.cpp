#include "ReportGenerator.h"

#include <iomanip>
#include <ostream>

#include "ComplianceScorer.h"
#include "RoleHierarchy.h"

namespace {
const int kWidth = 78;

void rule(std::ostream& os, char c) { os << std::string(kWidth, c) << "\n"; }

std::string joinActions(const std::vector<RepairAction>& actions) {
    if (actions.empty()) return "(no changes)";
    std::string s;
    for (const RepairAction& a : actions) {
        s += (s.empty() ? "" : " + ") + std::string(RepairAction::typeName(a.type())) + " " + a.target();
    }
    return s;
}

std::string ruleSummary(const PermissionSet& s) { return s.toString(); }
}  // namespace

void ReportGenerator::banner(std::ostream& os, const std::string& title) {
    rule(os, '=');
    int pad = (kWidth - static_cast<int>(title.size())) / 2;
    os << std::string(pad > 0 ? pad : 0, ' ') << title << "\n";
    rule(os, '=');
}

void ReportGenerator::section(std::ostream& os, const std::string& title) {
    os << "\n";
    rule(os, '-');
    os << " " << title << "\n";
    rule(os, '-');
}

void ReportGenerator::configuration(std::ostream& os, const SecurityPolicy& p, const CostModel& c,
                                    bool pruning) {
    section(os, "SYSTEM CONFIGURATION");
    os << "  Permissions in catalog : " << p.permissions().size() << "\n";
    os << "  Roles                  : " << p.roles().size() << "\n";
    os << "  Users                  : " << p.users().size() << "\n";
    os << "  Cost model             : " << c.describe() << "\n";
    os << "  Search mode            : " << (pruning ? "Backtracking WITH pruning" : "Brute force (no pruning)")
       << "\n\n  Permission catalog (risk weight 1-10 scales the cost of revoking/granting):\n";
    for (const Permission& perm : p.permissions()) {
        os << "    " << std::left << std::setw(18) << perm.id() << "risk " << std::setw(3) << perm.riskWeight()
           << perm.description() << std::right << "\n";
    }
    if (p.permissions().empty()) os << "    (none)\n";
    os << "\n  Separation-of-duties rules (a user may hold at most ONE of each set):\n";
    for (const SodRule& rule : p.sodRules()) {
        os << "    " << std::left << std::setw(20) << rule.name() << std::right << rule.exclusive().toString() << "\n";
    }
    if (p.sodRules().empty()) os << "    (none)\n";
}

void ReportGenerator::usersAndRoles(std::ostream& os, const SecurityPolicy& p) {
    section(os, "ROLES AND THEIR POLICY RULES");
    if (p.roles().empty()) os << "  (no roles defined)\n";
    for (const Role& r : p.roles()) {
        os << "  Role " << r.name() << "\n";
        if (!r.parents().empty()) {
            std::string parents;
            for (const std::string& parent : r.parents()) parents += (parents.empty() ? "" : ", ") + parent;
            os << "      inherits  : " << parents << "\n";
        }
        os << "      grants    : " << ruleSummary(r.grants()) << "\n";
        os << "      allowed   : " << ruleSummary(r.allowed()) << "\n";
        os << "      forbidden : " << ruleSummary(r.forbidden()) << "\n";
        os << "      required  : " << ruleSummary(r.required()) << "\n";
        if (!r.parents().empty()) {
            ResolvedRole rr = p.resolvedRole(r.name());
            os << "      -- with inheritance applied --\n";
            os << "      grants    : " << ruleSummary(rr.grants) << "\n";
            os << "      allowed   : " << ruleSummary(rr.allowed) << "\n";
            os << "      forbidden : " << ruleSummary(rr.forbidden) << "\n";
            os << "      required  : " << ruleSummary(rr.required) << "\n";
        }
    }
    bool hasLinks = false;
    for (const Role& r : p.roles()) hasLinks = hasLinks || !r.parents().empty();
    if (hasLinks) {
        section(os, "ROLE HIERARCHY (parent -> children)");
        os << RoleHierarchy::render(p);
        bool ok = true;
        std::vector<std::string> order = RoleHierarchy::topologicalOrder(p, ok);
        os << "  Topological order (parents first):";
        for (const std::string& name : order) os << " " << name;
        os << (ok ? "\n" : "   [!] incomplete: cycle detected\n");
    }
    section(os, "USERS");
    if (p.users().empty()) { os << "  (no users defined)\n"; return; }
    os << "  " << std::left << std::setw(14) << "User" << std::setw(26) << "Roles" << "Direct permissions\n"
       << std::right;
    for (const User& u : p.users()) {
        std::string roles;
        for (const std::string& r : u.roles()) roles += (roles.empty() ? "" : ", ") + r;
        os << "  " << std::left << std::setw(14) << u.name() << std::setw(26) << (roles.empty() ? "(none)" : roles)
           << u.directPermissions().toString() << std::right << "\n";
    }
}

void ReportGenerator::assignments(std::ostream& os, const SecurityPolicy& p) {
    section(os, "PERMISSION ASSIGNMENTS (effective permissions)");
    if (p.users().empty()) { os << "  (no users defined)\n"; return; }
    for (const User& u : p.users()) {
        EffectiveRules rules = p.rulesFor(u);
        os << "  " << u.name() << "\n";
        os << "      effective : " << p.effectivePermissions(u).toString() << "\n";
        os << "      allowed   : " << rules.allowed.toString() << "\n";
        os << "      forbidden : " << rules.forbidden.toString() << "\n";
        os << "      required  : " << rules.required.toString() << "\n";
    }
}

void ReportGenerator::violations(std::ostream& os, const ViolationList& list) {
    section(os, "DETECTED SECURITY VIOLATIONS (most severe first)");
    if (list.empty()) { os << "  [OK] No violations detected.\n"; return; }
    int n = 0;
    for (const Violation& v : list) {
        os << "  " << std::setw(2) << ++n << ". [" << std::left << std::setw(8)
           << Violation::severityName(v.severity()) << "] " << std::setw(12) << Violation::typeName(v.type())
           << std::right << (v.isRoleLevel() ? "" : "user '" + v.userName() + "' ")
           << v.description() << "\n";
    }
    os << "  Total: " << list.size() << " finding(s)\n";
}

void ReportGenerator::complianceScore(std::ostream& os, const SecurityPolicy& before, const SecurityPolicy* after) {
    section(os, "COMPLIANCE SCORE (0-100)");
    ComplianceReport b = ComplianceScorer::evaluate(before);
    ComplianceReport a;
    if (after) a = ComplianceScorer::evaluate(*after);
    os << "  Scoring: every user starts at 100; CRITICAL -25, HIGH -15, MEDIUM -8, LOW -3 per violation.\n\n";
    os << "  " << std::left << std::setw(16) << "User" << std::setw(12) << "Violations" << std::setw(10) << "Before"
       << (after ? "After" : "") << std::right << "\n";
    for (std::size_t i = 0; i < b.users.size(); ++i) {
        os << "  " << std::left << std::setw(16) << b.users[i].userName << std::setw(12) << b.users[i].violations
           << std::setw(10) << b.users[i].score;
        if (after && i < a.users.size()) os << a.users[i].score;
        os << std::right << "\n";
    }
    os << std::fixed << std::setprecision(1);
    os << "\n  SYSTEM SCORE BEFORE: " << b.overall << " / 100  [" << b.rating << "]\n";
    if (after) os << "  SYSTEM SCORE AFTER : " << a.overall << " / 100  [" << a.rating << "]\n";
    os << std::defaultfloat;
}

void ReportGenerator::candidates(std::ostream& os, const RepairPlan& plan) {
    section(os, "CANDIDATE REPAIR ACTIONS (sorted by cost)");
    if (plan.perUser.empty()) { os << "  No repairs needed.\n"; return; }
    for (const UserRepairResult& r : plan.perUser) {
        os << "  User '" << r.userName << "': " << r.candidates.size() << " candidate action(s)\n";
        for (const RepairAction& a : r.candidates) os << "      - " << a.describe() << "\n";
    }
}

void ReportGenerator::selection(std::ostream& os, const RepairPlan& plan, bool pruning) {
    section(os, "MINIMUM-COST REPAIR SELECTION");
    os << "  Search mode: " << (pruning ? "backtracking with pruning" : "brute force (no pruning)") << "\n";
    if (plan.perUser.empty()) {
        os << "  No violations -> nothing to repair. Minimum repair cost: 0\n";
        return;
    }
    const SearchStats& t = plan.totals;
    os << "\n  Candidate evaluation statistics (all users)\n";
    os << "      Candidate actions generated     : " << t.generated << "\n";
    os << "      Combinations in space (sum 2^n) : " << t.searchSpace << "\n";
    os << "      Recursive calls (nodes visited) : " << t.nodesVisited << "\n";
    os << "      Combinations explored           : " << t.explored << "\n";
    os << "      Invalid combinations rejected   : " << t.rejected << "\n";
    os << "      Valid combinations found        : " << t.valid << "\n";
    os << "      Branches pruned                 : " << t.pruned << "\n";

    for (const UserRepairResult& r : plan.perUser) {
        os << "\n  User '" << r.userName << "'  (" << r.stats.generated << " actions, " << r.stats.explored
           << " explored, " << r.stats.rejected << " rejected, " << r.stats.valid << " valid)\n";
        if (!r.solvable) {
            os << "      [FAIL] NO VALID REPAIR EXISTS\n" << r.explanation << "\n";
            continue;
        }
        os << "      [OK] Best solution (cost " << r.best.cost << "): " << joinActions(r.best.actions) << "\n";
        std::size_t shown = 0;
        for (const RepairCandidate& alt : r.alternatives) {
            if (shown >= 6) { os << "      ... and " << (r.alternatives.size() - shown) << " more valid alternative(s)\n"; break; }
            os << "      " << (shown == 0 ? "#1 selected   " : "alternative   ") << "cost " << alt.cost << " : "
               << joinActions(alt.actions) << "\n";
            ++shown;
        }
        if (r.alternatives.size() > 1) {
            os << "      Why it wins: the cheapest OTHER valid solution seen costs "
               << r.alternatives[1].cost << " vs " << r.best.cost
               << (pruning ? " (branches that could not beat the best cost were pruned, so they are not listed)" : "")
               << ".\n";
        } else {
            os << "      Why it wins: it is the only valid combination that survived the search.\n";
        }
        if (!r.rejectionReasons.empty()) {
            os << "      Rejections by reason:";
            for (const auto& kv : r.rejectionReasons) os << " [" << kv.first << " x" << kv.second << "]";
            os << "\n";
        }
    }
    os << "\n  ";
    if (plan.solvable) os << "MINIMUM TOTAL REPAIR COST: " << plan.totalCost << "\n";
    else os << "MINIMUM TOTAL REPAIR COST: undefined (at least one user cannot be repaired safely)\n";
}

void ReportGenerator::appliedRepair(std::ostream& os, const RepairPlan& plan, bool applied) {
    section(os, applied ? "APPLIED REPAIR" : "PROPOSED REPAIR (not yet applied)");
    std::vector<RepairAction> actions = plan.allActions();
    if (actions.empty()) { os << "  (no changes)\n"; return; }
    int n = 0;
    for (const RepairAction& a : actions) os << "  " << std::setw(2) << ++n << ". " << a.describe() << "\n";
    os << "  Total cost: " << plan.totalCost << "\n";
}

void ReportGenerator::beforeAfter(std::ostream& os, const SecurityPolicy& before, const SecurityPolicy& after) {
    section(os, "BEFORE / AFTER PERMISSION COMPARISON");
    if (before.users().empty()) { os << "  (no users)\n"; return; }
    for (const User& u : before.users()) {
        const User* a = after.findUser(u.name(), nullptr);
        PermissionSet b = before.effectivePermissions(u);
        PermissionSet f = a ? after.effectivePermissions(*a) : PermissionSet();
        os << "  " << u.name() << "\n";
        os << "      before : " << b.toString() << "\n";
        os << "      after  : " << f.toString() << "\n";
        PermissionSet removed = b.minus(f), added = f.minus(b);
        if (removed.empty() && added.empty()) os << "      change : (none)\n";
        else {
            if (!removed.empty()) os << "      change : - " << removed.toString() << "\n";
            if (!added.empty()) os << "      change : + " << added.toString() << "\n";
        }
    }
}

void ReportGenerator::validation(std::ostream& os, const ValidationReport& report) {
    section(os, "POST-REPAIR VALIDATION");
    for (const CheckResult& c : report.checks) {
        os << "  " << (c.passed ? "[PASS] " : "[FAIL] ") << c.name << "\n";
        if (!c.passed) os << "           " << c.details << "\n";
    }
    os << "  " << (report.remainingViolations == 0 ? "[PASS] " : "[FAIL] ")
       << "Remaining violations after repair: " << report.remainingViolations << "\n";
    os << "\n  FINAL SECURITY STATUS: " << (report.passed ? "[SECURE] repair validated" : "[NOT SECURE] validation failed")
       << "\n";
}

void ReportGenerator::fullReport(std::ostream& os, const ReportData& d) {
    banner(os, "PRIVILEGE TETRIS - SECURITY AUDIT REPORT");
    if (!d.original) { os << "  No configuration loaded.\n"; return; }
    configuration(os, *d.original, d.cost, d.pruning);                    // 1
    usersAndRoles(os, *d.original);                                       // 2
    assignments(os, *d.original);                                         // 3
    if (d.violations) violations(os, *d.violations);                      // 4
    else os << "\n  (violation detection has not been run)\n";
    complianceScore(os, *d.original, d.repaired);
    if (!d.plan) { os << "\n  (repair search has not been run)\n"; return; }
    candidates(os, *d.plan);                                              // 5
    selection(os, *d.plan, d.pruning);                                    // 6, 7
    if (!d.plan->solvable) { os << "\n  FINAL SECURITY STATUS: [NOT SECURE] no safe repair exists\n"; return; }
    appliedRepair(os, *d.plan, d.applied);                                // 8
    if (d.repaired) beforeAfter(os, *d.original, *d.repaired);            // 9
    if (d.validation) validation(os, *d.validation);                      // 10
    else os << "\n  (post-repair validation has not been run)\n";
    os << "\n";
    rule(os, '=');
}
