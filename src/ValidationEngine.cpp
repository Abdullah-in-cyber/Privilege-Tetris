#include "ValidationEngine.h"

#include <set>

#include "ViolationDetector.h"

namespace {
// Violations are compared by (type, permission) so "same problem before/after" is easy to test.
std::set<std::string> violationKeys(const std::vector<Violation>& vs) {
    std::set<std::string> keys;
    for (const Violation& v : vs) {
        keys.insert(std::string(Violation::typeName(v.type())) + "|" + v.permission());
    }
    return keys;
}
}  // namespace

UserCheck ValidationEngine::checkUser(const SecurityPolicy& beforePolicy, const User& before,
                                      const SecurityPolicy& afterPolicy, const User& after) {
    UserCheck c;
    const PermissionSet effBefore = beforePolicy.effectivePermissions(before);
    const PermissionSet effAfter = afterPolicy.effectivePermissions(after);
    const EffectiveRules rulesBefore = beforePolicy.rulesFor(before);
    const EffectiveRules rulesAfter = afterPolicy.rulesFor(after);

    if (!effAfter.intersectionWith(rulesAfter.forbidden).empty()) {
        c.forbiddenRemoved = false;
        c.failures.push_back("forbidden permission still held");
    }
    if (!rulesAfter.required.isSubsetOf(effAfter)) {
        c.requiredPreserved = false;
        c.failures.push_back("required permission missing");
    }
    if (!effAfter.isSubsetOf(rulesAfter.allowed)) {
        c.allowedSatisfied = false;
        c.failures.push_back("permission outside allowed set");
    }

    // Separation of duties: no rule may be broken after the repair.
    for (const SodRule& rule : afterPolicy.sodRules()) {
        if (!rule.heldConflict(effAfter).empty()) {
            c.sodSatisfied = false;
            c.failures.push_back("separation-of-duties rule still broken");
            break;
        }
    }

    // No NEW violation: everything wrong after must already have been wrong before.
    std::set<std::string> keysBefore = violationKeys(ViolationDetector::detectForUser(beforePolicy, before));
    for (const std::string& k : violationKeys(ViolationDetector::detectForUser(afterPolicy, after))) {
        if (keysBefore.count(k) == 0) {
            c.noNewViolations = false;
            c.failures.push_back("new violation introduced");
            break;
        }
    }

    // Access preservation: what the user legitimately had must survive.
    // Permissions caught in a broken separation-of-duties rule are the exception: by definition
    // some of them must go, but at least ONE of each broken rule's permissions must be kept.
    PermissionSet legitimate = effBefore.intersectionWith(rulesBefore.allowed).minus(rulesBefore.forbidden);
    for (const SodRule& rule : beforePolicy.sodRules()) {
        const PermissionSet heldBefore = rule.heldConflict(effBefore);
        if (heldBefore.empty()) continue;
        legitimate = legitimate.minus(heldBefore);
        if (heldBefore.intersectionWith(effAfter).empty()) {
            c.accessPreserved = false;
            c.failures.push_back("legitimate access lost");
        }
    }
    if (!legitimate.isSubsetOf(effAfter) && c.accessPreserved) {
        c.accessPreserved = false;
        c.failures.push_back("legitimate access lost");
    }

    // Least privilege: a repair may only ADD permissions the user was required to have.
    if (!effAfter.minus(effBefore).isSubsetOf(rulesBefore.required)) {
        c.noEscalation = false;
        c.failures.push_back("privilege escalation");
    }
    return c;
}

SecurityPolicy ValidationEngine::simulate(const SecurityPolicy& original,
                                          const std::vector<RepairAction>& actions) {
    SecurityPolicy copy = original;  // independent snapshot
    for (const RepairAction& a : actions) {
        if (User* u = copy.findUser(a.userName())) a.applyTo(*u);
    }
    return copy;
}

ValidationReport ValidationEngine::validate(const SecurityPolicy& original,
                                            const SecurityPolicy& repaired) {
    struct Item { const char* name; std::string offenders; };
    Item forbidden{"All forbidden permissions removed", ""};
    Item required{"All required permissions preserved", ""};
    Item allowed{"All allowed-permission rules satisfied", ""};
    Item sod{"Separation-of-duties rules satisfied", ""};
    Item noNew{"No new violations introduced", ""};
    Item access{"Legitimate access preserved (no collateral damage)", ""};
    Item escalation{"No privilege escalation", ""};

    auto note = [](Item& item, const std::string& user, const std::string& what) {
        item.offenders += (item.offenders.empty() ? "" : "; ") + user + ": " + what;
    };

    for (const User& before : original.users()) {
        const User* after = repaired.findUser(before.name());
        if (!after) {  // a user vanished: that destroys all their access
            note(access, before.name(), "user missing after repair");
            continue;
        }
        UserCheck c = checkUser(original, before, repaired, *after);
        if (!c.forbiddenRemoved) note(forbidden, before.name(), "still holds a forbidden permission");
        if (!c.requiredPreserved) note(required, before.name(), "required permission missing");
        if (!c.allowedSatisfied) note(allowed, before.name(), "holds a permission outside allowed set");
        if (!c.sodSatisfied) note(sod, before.name(), "separation-of-duties rule still broken");
        if (!c.noNewViolations) note(noNew, before.name(), "new violation");
        if (!c.accessPreserved) note(access, before.name(), "legitimate access lost");
        if (!c.noEscalation) note(escalation, before.name(), "gained extra permission");
    }

    ValidationReport report;
    report.passed = true;
    for (const Item* item : {&forbidden, &required, &allowed, &sod, &noNew, &access, &escalation}) {
        bool ok = item->offenders.empty();
        report.checks.push_back({item->name, ok, item->offenders});
        if (!ok) report.passed = false;
    }
    for (const User& u : repaired.users()) {
        report.remainingViolations += static_cast<int>(ViolationDetector::detectForUser(repaired, u).size());
    }
    if (report.remainingViolations > 0) report.passed = false;
    return report;
}
