#include "RepairEngine.h"

#include <algorithm>
#include <set>

#include "Sorting.h"
#include "ValidationEngine.h"
#include "ViolationDetector.h"

// =====================================================================
//  BACKTRACKING SEARCH (one user)
//
//  We have n candidate actions a0..a(n-1). A "solution" is any subset of them.
//  The search builds subsets one decision at a time:
//
//                       explore(i)
//                      /          \\  (two branches)
//            INCLUDE a_i          EXCLUDE a_i
//          explore(i+1)           explore(i+1)
//
//  When i == n, the subset is complete: apply it to a COPY of the user and ask
//  the ValidationEngine whether the result is acceptable. Valid subsets are
//  compared by cost; the cheapest one wins (ties: fewer actions).
//
//  PRUNING (cutting branches that cannot lead to a better answer):
//   1. Cost bound  : if the cost so far is already above the best valid cost,
//                    adding more actions can only make it worse -> backtrack.
//   2. Lower bound : every violated permission needs at least one action that
//                    could fix it. If none of the chosen actions fixes it yet, the
//                    cheapest still-available fixer MUST be paid for. When
//                    (cost so far + that minimum) already exceeds the best valid
//                    cost -> backtrack. If no fixer is left at all, the branch is
//                    infeasible -> backtrack.
//  Neither rule discards a better solution, so the result stays optimal.
//  With pruning switched off the same code is a pure brute-force enumeration.
// =====================================================================
namespace {

constexpr std::size_t kMaxAlternativesKept = 500;
constexpr int kNoFixer = 1000000000;  // "infinite" cost: no action can fix this target

// One thing that must be fixed: a bad permission the user holds, or a missing one.
struct Target {
    PermissionSet perms;             // one permission, or the held set of a broken separation-of-duties rule
    bool missing;                    // true: needs adding, false: needs removing
    std::vector<bool> canFix;        // canFix[a] = could action a fix this target?
    std::vector<int> minCostFrom;    // minCostFrom[i] = cheapest fixing action with index >= i (kNoFixer if none)
};

class Backtracker {
public:
    Backtracker(const SecurityPolicy& policy, const User& user,
                const std::vector<RepairAction>& actions, bool pruning, UserRepairResult& out)
        : policy_(policy), user_(user), actions_(actions), pruning_(pruning), out_(out) {
        buildTargets();
    }

    void run() {
        explore(0, 0);
        mergeSort(alternatives_, [](const RepairCandidate& a, const RepairCandidate& b) {
            if (a.cost != b.cost) return a.cost < b.cost;
            return a.actions.size() < b.actions.size();
        });
        out_.solvable = haveBest_;
        out_.alternatives = alternatives_;
        if (haveBest_) out_.best = best_;
    }

private:
    void buildTargets() {
        const PermissionSet eff = policy_.effectivePermissions(user_);
        const EffectiveRules rules = policy_.rulesFor(user_);
        PermissionSet bad = eff.intersectionWith(rules.forbidden).unionWith(eff.minus(rules.allowed));
        PermissionSet missing = rules.required.minus(eff);

        auto make = [&](const PermissionSet& perms, bool isMissing) {
            Target t;
            t.perms = perms;
            t.missing = isMissing;
            t.canFix.assign(actions_.size(), false);
            for (std::size_t a = 0; a < actions_.size(); ++a) {
                const RepairAction& act = actions_[a];
                bool roleChange = act.type() == RepairType::REMOVE_ROLE || act.type() == RepairType::ADD_ROLE;
                if (roleChange) t.canFix[a] = true;  // conservative: role changes may alter any rule
                else if (!isMissing && act.type() == RepairType::REVOKE_DIRECT) t.canFix[a] = perms.contains(act.target());
                else if (isMissing && act.type() == RepairType::GRANT_DIRECT) t.canFix[a] = perms.contains(act.target());
            }
            // minCostFrom[i]: cheapest fixing action among indices >= i (suffix minimum)
            t.minCostFrom.assign(actions_.size() + 1, kNoFixer);
            for (std::size_t i = actions_.size(); i-- > 0;) {
                t.minCostFrom[i] = t.canFix[i] ? std::min(t.minCostFrom[i + 1], actions_[i].cost())
                                               : t.minCostFrom[i + 1];
            }
            targets_.push_back(std::move(t));
        };
        for (const std::string& p : bad.items()) make(PermissionSet{p}, false);
        for (const std::string& p : missing.items()) make(PermissionSet{p}, true);
        // A broken separation-of-duties rule is ONE target: at least one of the held
        // conflicting permissions must be removed.
        for (const SodRule& rule : policy_.sodRules()) {
            PermissionSet held = rule.heldConflict(eff);
            if (!held.empty()) make(held, false);
        }
    }

    // Lower bound on the FINAL cost of any solution that extends the current path:
    // cost so far + the largest "cheapest remaining fixer" over unfixed targets
    // (max, not sum, because one action may fix several targets).
    // Returns kNoFixer when some target can no longer be fixed.
    int lowerBound(std::size_t next, int cost) const {
        int extra = 0;
        for (const Target& t : targets_) {
            bool fixedByChosen = false;
            for (std::size_t idx : chosen_) {
                if (t.canFix[idx]) { fixedByChosen = true; break; }
            }
            if (fixedByChosen) continue;
            if (t.minCostFrom[next] == kNoFixer) return kNoFixer;
            if (t.minCostFrom[next] > extra) extra = t.minCostFrom[next];
        }
        return cost + extra;
    }

    void explore(std::size_t i, int cost) {
        ++out_.stats.nodesVisited;

        if (pruning_) {
            if (haveBest_ && cost > best_.cost) { ++out_.stats.pruned; return; }  // rule 1
            int bound = lowerBound(i, cost);                                      // rule 2
            if (bound == kNoFixer || (haveBest_ && bound > best_.cost)) { ++out_.stats.pruned; return; }
        }

        if (i == actions_.size()) {  // complete combination -> evaluate it
            evaluate(cost);
            return;
        }

        chosen_.push_back(i);                       // choice 1: INCLUDE action i
        explore(i + 1, cost + actions_[i].cost());
        chosen_.pop_back();                         // undo the choice (the "back" in backtrack)

        explore(i + 1, cost);                       // choice 2: EXCLUDE action i
    }

    void evaluate(int cost) {
        ++out_.stats.explored;
        User simulated = user_;  // work on a copy, never on the real user
        for (std::size_t idx : chosen_) actions_[idx].applyTo(simulated);

        UserCheck check = ValidationEngine::checkUser(policy_, user_, policy_, simulated);
        if (!check.passed()) {
            ++out_.stats.rejected;
            for (const std::string& reason : check.failures) ++out_.rejectionReasons[reason];
            return;
        }

        ++out_.stats.valid;
        RepairCandidate candidate;
        candidate.cost = cost;
        for (std::size_t idx : chosen_) candidate.actions.push_back(actions_[idx]);
        if (alternatives_.size() < kMaxAlternativesKept) alternatives_.push_back(candidate);

        bool better = !haveBest_ || cost < best_.cost ||
                      (cost == best_.cost && candidate.actions.size() < best_.actions.size());
        if (better) { best_ = candidate; haveBest_ = true; }
    }

    const SecurityPolicy& policy_;
    const User& user_;
    const std::vector<RepairAction>& actions_;
    bool pruning_;
    UserRepairResult& out_;

    std::vector<Target> targets_;
    std::vector<std::size_t> chosen_;  // indices of actions included on the current path
    std::vector<RepairCandidate> alternatives_;
    RepairCandidate best_;
    bool haveBest_ = false;
};

std::string explainFailure(const SecurityPolicy& policy, const User& user, const UserRepairResult& r) {
    std::string text;
    const EffectiveRules rules = policy.rulesFor(user);
    const PermissionSet conflicting = rules.required.intersectionWith(rules.forbidden);
    const PermissionSet notAllowed = rules.required.minus(rules.allowed).minus(rules.forbidden);
    for (const std::string& p : conflicting.items()) {
        text += "  - Policy conflict: '" + p + "' is both REQUIRED and FORBIDDEN for this user, "
                "so no configuration can satisfy both.\n";
    }
    for (const std::string& p : notAllowed.items()) {
        text += "  - Policy conflict: '" + p + "' is REQUIRED but not ALLOWED.\n";
    }
    if (r.searchTooLarge) {
        text += "  - Search space too large (" + std::to_string(r.stats.generated) +
                " candidate actions; limit is " +
                std::to_string(RepairEngine::kMaxCandidatesPerUser) + ").\n";
    } else {
        text += "  - All " + std::to_string(r.stats.explored) + " combinations of the " +
                std::to_string(r.stats.generated) + " candidate actions were rejected:\n";
        for (const auto& kv : r.rejectionReasons) {
            text += "      * " + kv.first + " (" + std::to_string(kv.second) + " candidate(s))\n";
        }
    }
    text += "  - No safe repair exists; the configuration must be fixed manually.";
    return text;
}

}  // namespace

// ---------------------------------------------------------------------
//  Candidate generation (Module 3)
//  Only actions that touch a violated or affected permission are generated,
//  so the search space stays small. Nothing here validates the result: unsafe
//  candidates are generated on purpose and then REJECTED by the search.
// ---------------------------------------------------------------------
std::vector<RepairAction> RepairEngine::generateCandidates(const SecurityPolicy& policy,
                                                           const User& user) const {
    std::vector<RepairAction> actions;
    std::set<std::string> seen;
    auto add = [&](RepairType type, const std::string& target) {
        // REVOKE/GRANT cost scales with the permission's risk weight; role actions do not.
        const Permission* perm = policy.findPermission(target);
        int risk = (perm && (type == RepairType::REVOKE_DIRECT || type == RepairType::GRANT_DIRECT)) ? perm->riskWeight() : 1;
        RepairAction a(type, user.name(), target, cost_.costOf(type, risk));
        if (seen.insert(a.key()).second) actions.push_back(a);
    };

    const PermissionSet eff = policy.effectivePermissions(user);
    const EffectiveRules rules = policy.rulesFor(user);
    PermissionSet bad = eff.intersectionWith(rules.forbidden).unionWith(eff.minus(rules.allowed));
    for (const SodRule& rule : policy.sodRules()) {      // permissions caught in a broken SoD rule
        bad = bad.unionWith(rule.heldConflict(eff));      // are removal candidates too
    }
    const PermissionSet missing = rules.required.minus(eff);
    const PermissionSet legitimate = eff.intersectionWith(rules.allowed).minus(rules.forbidden);

    // (a) Remove what should not be there.
    PermissionSet lostByRoleRemoval;  // legitimate permissions that disappear if a role is removed
    for (const std::string& p : bad.items()) {
        if (user.directPermissions().contains(p)) add(RepairType::REVOKE_DIRECT, p);
    }
    for (const std::string& roleName : user.roles()) {
        const PermissionSet roleGrants = policy.resolvedRole(roleName).grants;  // includes inherited
        if (!roleGrants.intersectionWith(bad).empty()) {
            add(RepairType::REMOVE_ROLE, roleName);
            lostByRoleRemoval = lostByRoleRemoval.unionWith(roleGrants.intersectionWith(legitimate));
        }
    }

    // (b) Restore what is missing, or what a role removal would take away.
    const PermissionSet needed = missing.unionWith(lostByRoleRemoval);
    for (const std::string& p : needed.items()) {
        bool grantable = !rules.forbidden.contains(p) &&
                         (rules.allowed.contains(p) || rules.required.contains(p));
        if (grantable && !user.directPermissions().contains(p)) add(RepairType::GRANT_DIRECT, p);

        for (const Role& role : policy.roles()) {  // replace/add a role that provides p
            if (user.hasRole(role.name())) continue;
            const PermissionSet roleGrants = policy.resolvedRole(role.name()).grants;
            if (!roleGrants.contains(p)) continue;
            if (!roleGrants.intersectionWith(rules.forbidden).empty()) continue;
            add(RepairType::ADD_ROLE, role.name());
        }
    }

    // Sorting the candidates by cost (cheapest first) also makes pruning effective.
    mergeSort(actions, [](const RepairAction& a, const RepairAction& b) { return a.cost() < b.cost(); });
    return actions;
}

UserRepairResult RepairEngine::solveUser(const SecurityPolicy& policy, const User& user) const {
    UserRepairResult result;
    result.userName = user.name();
    result.candidates = generateCandidates(policy, user);
    result.stats.generated = result.candidates.size();

    if (result.candidates.size() > kMaxCandidatesPerUser) {
        result.searchTooLarge = true;
        result.explanation = explainFailure(policy, user, result);
        return result;
    }
    result.stats.searchSpace = 1ULL << result.candidates.size();

    Backtracker search(policy, user, result.candidates, pruning_, result);
    search.run();

    if (!result.solvable) result.explanation = explainFailure(policy, user, result);
    return result;
}

RepairPlan RepairEngine::findMinimumRepair(const SecurityPolicy& policy) const {
    RepairPlan plan;
    for (const User& user : policy.users()) {
        if (ViolationDetector::detectForUser(policy, user).empty()) continue;  // already secure
        UserRepairResult r = solveUser(policy, user);
        plan.totals.add(r.stats);
        if (r.solvable) plan.totalCost += r.best.cost;
        else plan.solvable = false;
        plan.perUser.push_back(std::move(r));
    }
    return plan;
}

std::vector<RepairAction> RepairPlan::allActions() const {
    std::vector<RepairAction> all;
    for (const UserRepairResult& r : perUser) {
        if (!r.solvable) continue;
        for (const RepairAction& a : r.best.actions) all.push_back(a);
    }
    return all;
}
