#include "Scenarios.h"

#include "ComplianceScorer.h"
#include "ViolationDetector.h"

namespace {

Role makeRole(const std::string& name, std::initializer_list<const char*> grants,
              std::initializer_list<const char*> allowed, std::initializer_list<const char*> forbidden,
              std::initializer_list<const char*> required) {
    Role r(name);
    for (const char* p : grants) r.addGrant(p);
    for (const char* p : allowed) r.addAllowed(p);
    for (const char* p : forbidden) r.addForbidden(p);
    for (const char* p : required) r.addRequired(p);
    return r;
}

User makeUser(const std::string& name, std::initializer_list<const char*> roles,
              std::initializer_list<const char*> direct = {}) {
    User u(name);
    for (const char* r : roles) u.addRole(r);
    for (const char* p : direct) u.grantDirect(p);
    return u;
}

// Permission catalog + the three core university roles used by every scenario.
SecurityPolicy baseUniversity() {
    SecurityPolicy p;
    // Third argument = risk weight (1-10): how costly it is to change this permission.
    p.addPermission(Permission("grades.view", "View student grades", 1));
    p.addPermission(Permission("grades.edit", "Edit student grades", 3));
    p.addPermission(Permission("accounts.manage", "Create/delete user accounts", 5));
    p.addPermission(Permission("system.settings", "Change system-wide settings", 5));
    p.addPermission(Permission("attendance.mark", "Mark class attendance", 1));
    p.addPermission(Permission("courses.enroll", "Enroll in courses", 1));
    p.addPermission(Permission("logs.read", "Read system logs", 2));
    p.addPermission(Permission("audit.export", "Export audit data", 3));

    p.addRole(makeRole("Student",
        {"grades.view", "courses.enroll"}, {"grades.view", "courses.enroll"},
        {"grades.edit", "accounts.manage", "system.settings", "attendance.mark"}, {"grades.view"}));
    p.addRole(makeRole("Teacher",
        {"grades.view", "grades.edit", "attendance.mark"}, {"grades.view", "grades.edit", "attendance.mark"},
        {"accounts.manage", "system.settings"}, {"grades.view", "grades.edit"}));
    p.addRole(makeRole("Admin",
        {"accounts.manage", "system.settings", "logs.read", "grades.view"},
        {"accounts.manage", "system.settings", "logs.read", "grades.view"},
        {"grades.edit"}, {"accounts.manage"}));
    return p;
}

// Roles used by scenarios 3 and 4.
void addAssistantRoles(SecurityPolicy& p) {
    // Assistant REQUIRES attendance.mark but (legacy misconfiguration) does not grant it.
    p.addRole(makeRole("Assistant", {"grades.view"}, {"grades.view", "attendance.mark"},
                       {"grades.edit", "accounts.manage", "system.settings"},
                       {"grades.view", "attendance.mark"}));
    p.addRole(makeRole("AttendanceClerk", {"attendance.mark"}, {"attendance.mark"}, {}, {}));
}

}  // namespace

std::vector<Scenario> buildScenarios() {
    std::vector<Scenario> list;

    {   // 1 -------------------------------------------------------------
        SecurityPolicy p = baseUniversity();
        p.addUser(makeUser("alice", {"Student"}, {"grades.edit"}));
        p.addUser(makeUser("bob", {"Student"}));
        list.push_back({1, "Student Excessive Privileges",
            "Student 'alice' can view AND edit grades. Students may only view them.",
            "Detect forbidden grades.edit and recommend revoking it (cost 3 = revoke 1 x risk 3).", 1, 3, p});
    }
    {   // 2 -------------------------------------------------------------
        SecurityPolicy p = baseUniversity();
        p.addUser(makeUser("tom", {"Teacher"}, {"accounts.manage"}));
        p.addUser(makeUser("lisa", {"Teacher"}));
        p.addUser(makeUser("ann", {"Admin"}));
        list.push_back({2, "Teacher Administrative Privileges",
            "Teacher 'tom' also holds accounts.manage, an administrative right.",
            "Revoke accounts.manage only; tom keeps grades.view/edit and attendance (cost 5 = revoke 1 x risk 5).", 1, 5, p});
    }
    {   // 3 -------------------------------------------------------------
        SecurityPolicy p = baseUniversity();
        addAssistantRoles(p);
        p.addUser(makeUser("sam", {"Assistant"}));
        p.addUser(makeUser("lisa", {"Teacher"}));
        list.push_back({3, "Missing Required Permissions",
            "Teaching assistant 'sam' holds role Assistant, which requires attendance.mark, but lacks it.",
            "Identify the missing permission; grant it directly (cost 1) rather than adding a role (cost 2).",
            1, 1, p});
    }
    {   // 4 -------------------------------------------------------------
        SecurityPolicy p = baseUniversity();
        addAssistantRoles(p);
        p.addUser(makeUser("dave", {"Assistant"}, {"grades.edit", "accounts.manage"}));
        p.addUser(makeUser("eve", {"Student"}, {"grades.edit"}));
        list.push_back({4, "Multiple Violations",
            "'dave' (Assistant) holds two forbidden permissions AND lacks a required one; 'eve' is an over-privileged student.",
            "Explore many combinations; dave: revoke accounts.manage 5 + revoke grades.edit 3 + grant attendance.mark 1 = 9; eve: revoke grades.edit 3; total 12.",
            4, 12, p});
    }
    {   // 5 -------------------------------------------------------------
        SecurityPolicy p = baseUniversity();
        p.addRole(makeRole("Auditor", {"logs.read", "audit.export"}, {"logs.read", "audit.export"},
                           {"audit.export"}, {"logs.read", "audit.export"}));
        p.addUser(makeUser("zara", {"Auditor"}));
        p.addUser(makeUser("ann", {"Admin"}));
        list.push_back({5, "Impossible Repair (conflicting policy)",
            "Role Auditor both REQUIRES and FORBIDS audit.export, and zara holds it.",
            "Detect the conflict; every candidate is rejected; explain why no safe repair exists.",
            1, -1, p});
    }
    {   // 6 -------------------------------------------------------------
        SecurityPolicy p = baseUniversity();
        p.addUser(makeUser("alice", {"Student"}));
        p.addUser(makeUser("tom", {"Teacher"}));
        p.addUser(makeUser("ann", {"Admin"}));
        list.push_back({6, "Already Secure System",
            "Every user already satisfies the policy.",
            "Report zero violations and zero repair cost.", 0, 0, p});
    }
    {   // 7 -------------------------------------------------------------
        SecurityPolicy p = baseUniversity();
        p.addRole(makeRole("ComplianceOfficer", {"audit.export", "logs.read"}, {"audit.export", "logs.read"}, {}, {}));
        p.addSodRule(SodRule("SOD-ADMIN-AUDIT", PermissionSet{"accounts.manage", "audit.export"}));
        p.addUser(makeUser("finn", {"Admin", "ComplianceOfficer"}));
        p.addUser(makeUser("ann", {"Admin"}));
        list.push_back({7, "Separation of Duties Violation",
            "'finn' holds Admin (accounts.manage) AND ComplianceOfficer (audit.export); rule SOD-ADMIN-AUDIT allows only one of them.",
            "Detect the SoD conflict; remove role ComplianceOfficer (cost 2), keeping all of finn's admin access.",
            1, 2, p});
    }
    {   // 8 -------------------------------------------------------------
        SecurityPolicy p = baseUniversity();
        Role head = makeRole("HeadTeacher", {"logs.read"}, {"logs.read"}, {}, {});
        head.addParent("Teacher");  // HeadTeacher inherits everything Teacher has, including Teacher's rules
        p.addRole(head);
        p.addUser(makeUser("hana", {"HeadTeacher"}, {"accounts.manage"}));
        p.addUser(makeUser("hugo", {"HeadTeacher"}));
        list.push_back({8, "Role Hierarchy (inherited rules)",
            "HeadTeacher inherits Teacher. 'hana' holds accounts.manage, which is forbidden only through the INHERITED Teacher rule.",
            "Resolve inheritance, detect the forbidden permission, revoke it (cost 5); hugo is clean and keeps inherited teaching access.",
            1, 5, p});
    }
    return list;
}

PipelineResult runPipeline(const SecurityPolicy& policy, const CostModel& cost, bool pruning) {
    PipelineResult out;
    ViolationList all = ViolationDetector::detectAll(policy);
    for (const Violation& v : all) {
        if (v.isRoleLevel()) ++out.roleIssues; else ++out.userViolations;
    }
    RepairEngine engine(cost, pruning);
    out.plan = engine.findMinimumRepair(policy);
    out.scoreBefore = ComplianceScorer::evaluate(policy).overall;
    if (out.plan.solvable) {
        out.repaired = ValidationEngine::simulate(policy, out.plan.allActions());
        out.validation = ValidationEngine::validate(policy, out.repaired);
        out.simulated = true;
        out.scoreAfter = ComplianceScorer::evaluate(out.repaired).overall;
    }
    return out;
}
