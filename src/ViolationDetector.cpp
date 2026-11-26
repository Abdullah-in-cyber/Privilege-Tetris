#include "ViolationDetector.h"

#include "Sorting.h"

std::vector<Violation> ViolationDetector::detectForUser(const SecurityPolicy& policy,
                                                        const User& user) {
    std::vector<Violation> found;
    const PermissionSet eff = policy.effectivePermissions(user);
    const EffectiveRules rules = policy.rulesFor(user);

    // Set operations decide WHAT is wrong ...
    const PermissionSet forbiddenHeld = eff.intersectionWith(rules.forbidden);
    const PermissionSet unauthorized = eff.minus(rules.allowed).minus(rules.forbidden);
    const PermissionSet missing = rules.required.minus(eff);

    // ... and one pass over each result turns it into Violation records.
    for (const std::string& p : forbiddenHeld.items()) {
        found.emplace_back(ViolationType::FORBIDDEN_PERMISSION, user.name(), "", p,
                           "holds forbidden permission '" + p + "' (via " +
                               policy.sourcesOf(user, p) + ")");
    }
    for (const std::string& p : unauthorized.items()) {
        found.emplace_back(ViolationType::UNAUTHORIZED_PERMISSION, user.name(), "", p,
                           "holds permission '" + p + "' that none of their roles allow (via " +
                               policy.sourcesOf(user, p) + ")");
    }
    for (const std::string& p : missing.items()) {
        found.emplace_back(ViolationType::MISSING_REQUIRED, user.name(), "", p,
                           "is missing required permission '" + p + "'");
    }

    // Separation of duties: more than one permission of an exclusive set held at once.
    for (const SodRule& rule : policy.sodRules()) {
        const PermissionSet held = rule.heldConflict(eff);
        if (held.empty()) continue;
        std::string joined;
        for (const std::string& p : held.items()) joined += (joined.empty() ? "" : "+") + p;
        found.emplace_back(ViolationType::SOD_CONFLICT, user.name(), rule.name(), joined,
                           "breaks separation-of-duties rule '" + rule.name() + "': holds " + held.toString() +
                               " together (at most one allowed)");
    }
    return found;
}

std::vector<Violation> ViolationDetector::detectRoleIssues(const SecurityPolicy& policy) {
    std::vector<Violation> found;
    auto report = [&](const Role& role, const PermissionSet& set, const std::string& text) {
        for (const std::string& p : set.items()) {
            found.emplace_back(ViolationType::ROLE_POLICY_INCONSISTENCY, "", role.name(), p,
                               "role '" + role.name() + "' " + text + " '" + p + "'");
        }
    };
    for (const Role& role : policy.roles()) {
        // Use the RESOLVED sets so inherited grants/rules are checked too.
        const ResolvedRole rr = policy.resolvedRole(role.name());
        report(role, rr.required.intersectionWith(rr.forbidden), "both REQUIRES and FORBIDS");
        report(role, rr.required.minus(rr.allowed).minus(rr.forbidden), "requires but does not allow");
        report(role, rr.grants.intersectionWith(rr.forbidden), "grants forbidden");
        report(role, rr.grants.minus(rr.allowed).minus(rr.forbidden), "grants beyond its allowed set:");
        report(role, rr.required.minus(rr.grants).minus(rr.forbidden), "requires but does not grant");
    }
    return found;
}

ViolationList ViolationDetector::detectAll(const SecurityPolicy& policy) {
    ViolationList list;
    for (const User& user : policy.users()) {
        for (const Violation& v : detectForUser(policy, user)) list.pushBack(v);
    }
    for (const Violation& v : detectRoleIssues(policy)) list.pushBack(v);
    sortBySeverity(list);
    return list;
}

void ViolationDetector::sortBySeverity(ViolationList& list) {
    std::vector<Violation> items = list.toVector();
    mergeSort(items, [](const Violation& a, const Violation& b) {
        return static_cast<int>(a.severity()) > static_cast<int>(b.severity());
    });
    ViolationList sorted;
    for (const Violation& v : items) sorted.pushBack(v);
    list = sorted;
}
