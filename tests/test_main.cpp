// Automated tests for Privilege Tetris. Run with `make test`.
// Plain asserts with a counter: no external test framework needed.
#include <iostream>
#include <random>
#include <sstream>

#include <algorithm>

#include "AuditLogger.h"
#include "ComplianceScorer.h"
#include "ConfigIO.h"
#include "LinkedList.h"
#include "LinearSearch.h"
#include "PermissionSet.h"
#include "RepairEngine.h"
#include "RoleHierarchy.h"
#include "Scenarios.h"
#include "Sorting.h"
#include "ValidationEngine.h"
#include "ViolationDetector.h"
#include "WebApi.h"

static int g_passed = 0, g_failed = 0;

#define CHECK(cond, msg)                                                         \
    do {                                                                         \
        if (cond) { ++g_passed; }                                                \
        else { ++g_failed; std::cout << "  [FAIL] " << msg << "  (" #cond ")\n"; } \
    } while (0)

static void section(const char* name) { std::cout << "\n== " << name << " ==\n"; }

// ---------------------------------------------------------------- data structures
static void testLinkedList() {
    section("DoublyLinkedList");
    DoublyLinkedList<int> l;
    CHECK(l.empty() && l.size() == 0, "new list is empty");
    l.pushBack(2); l.pushBack(3); l.pushFront(1);                 // 1 2 3
    CHECK(l.size() == 3 && *l.at(0) == 1 && *l.at(2) == 3, "pushFront / pushBack order");
    CHECK(l.insertAt(1, 10) && *l.at(1) == 10 && l.size() == 4, "insertAt middle");
    CHECK(l.insertAt(0, 0) && *l.at(0) == 0, "insertAt front");
    CHECK(l.insertAt(l.size(), 99) && *l.at(l.size() - 1) == 99, "insertAt back");
    CHECK(!l.insertAt(100, 5), "insertAt beyond size rejected");
    std::size_t steps = 0;
    int* hit = l.find([](int v) { return v == 3; }, &steps);   // 0 1 10 2 3 99
    CHECK(hit && *hit == 3 && steps == 5, "linear search finds 3 after 5 comparisons");
    CHECK(l.find([](int v) { return v == 12345; }, &steps) == nullptr && steps == l.size(), "miss examines every node");
    CHECK(l.removeAt(0) && *l.at(0) == 1, "removeAt head");
    CHECK(l.removeAt(l.size() - 1) && *l.at(l.size() - 1) == 3, "removeAt tail");
    CHECK(!l.removeAt(50), "removeAt out of range rejected");
    DoublyLinkedList<int> copy(l);
    CHECK(copy.size() == l.size(), "copy has same size");
    *copy.at(0) = 777;
    CHECK(*l.at(0) == 1, "copy is deep (independent nodes)");
    std::size_t removed = l.removeIf([](int v) { return v > 1; });
    CHECK(removed == 3 && l.size() == 1 && *l.at(0) == 1, "removeIf deletes matching nodes");
    l.clear();
    CHECK(l.empty() && l.at(0) == nullptr, "clear empties the list");
    DoublyLinkedList<int> single;
    single.pushBack(5);
    single.removeAt(0);
    CHECK(single.empty(), "removing the only node leaves a valid empty list");
}

static void testPermissionSet() {
    section("PermissionSet (set operations)");
    PermissionSet a{"x", "y", "z"}, b{"y", "z", "w"};
    CHECK(a.unionWith(b) == PermissionSet({"w", "x", "y", "z"}), "union");
    CHECK(a.intersectionWith(b) == PermissionSet({"y", "z"}), "intersection");
    CHECK(a.minus(b) == PermissionSet({"x"}), "difference A\\B");
    CHECK(b.minus(a) == PermissionSet({"w"}), "difference B\\A");
    CHECK(a.contains("x") && !a.contains("w"), "membership");
    CHECK(PermissionSet({"y"}).isSubsetOf(a) && !b.isSubsetOf(a), "subset test");
    CHECK(PermissionSet().unionWith(PermissionSet()).empty(), "union of empty sets is empty");
    CHECK(a.toString() == "{x, y, z}" && PermissionSet().toString() == "{}", "toString");
}

static void testSortingAndSearch() {
    section("Merge sort / linear search");
    std::vector<std::pair<int, char>> v{{3, 'a'}, {1, 'b'}, {3, 'c'}, {2, 'd'}, {1, 'e'}};
    mergeSort(v, [](const std::pair<int, char>& x, const std::pair<int, char>& y) { return x.first < y.first; });
    bool sorted = true;
    for (std::size_t i = 1; i < v.size(); ++i) sorted = sorted && v[i - 1].first <= v[i].first;
    CHECK(sorted, "sorted ascending");
    CHECK(v[0].second == 'b' && v[1].second == 'e' && v[3].second == 'a' && v[4].second == 'c', "merge sort is stable");
    std::vector<int> empty;
    mergeSort(empty, [](int x, int y) { return x < y; });
    CHECK(empty.empty(), "sorting an empty vector is safe");
    std::vector<int> big;
    for (int i = 0; i < 1000; ++i) big.push_back((i * 7919) % 1000);
    mergeSort(big, [](int x, int y) { return x < y; });
    bool ok = true;
    for (std::size_t i = 1; i < big.size(); ++i) ok = ok && big[i - 1] <= big[i];
    CHECK(ok, "1000 elements sorted");

    std::vector<int> items{5, 8, 2, 9};
    SearchResult r = linearSearch(items, [](int x) { return x == 2; });
    CHECK(r.found() && r.index == 2 && r.comparisons == 3, "linear search hit: index 2 after 3 comparisons");
    r = linearSearch(items, [](int x) { return x == 42; });
    CHECK(!r.found() && r.comparisons == 4, "linear search miss: 4 comparisons");
}

// ---------------------------------------------------------------- scenarios
static void testScenarios() {
    section("Predefined scenarios (expected vs actual)");
    CostModel cost;
    for (const Scenario& s : buildScenarios()) {
        PipelineResult r = runPipeline(s.policy, cost, true);
        std::string tag = "scenario " + std::to_string(s.id) + ": ";
        CHECK(r.userViolations == s.expectedViolations, tag + "violation count");
        if (s.expectedCost < 0) {
            CHECK(!r.plan.solvable, tag + "no repair exists");
            CHECK(!r.simulated, tag + "nothing simulated when impossible");
            bool explained = !r.plan.perUser.empty() && !r.plan.perUser[0].explanation.empty();
            CHECK(explained, tag + "explanation provided");
        } else {
            CHECK(r.plan.solvable && r.plan.totalCost == s.expectedCost, tag + "minimum cost");
            CHECK(r.simulated && r.validation.passed, tag + "post-repair validation passes");
            CHECK(r.validation.remainingViolations == 0, tag + "no violations remain");
        }
        // Pruning must never change the optimal COST.
        PipelineResult brute = runPipeline(s.policy, cost, false);
        CHECK(brute.plan.solvable == r.plan.solvable && brute.plan.totalCost == r.plan.totalCost,
              tag + "brute force and pruned backtracking agree");
        CHECK(brute.plan.totals.explored >= r.plan.totals.explored, tag + "pruning never explores more");
    }
}

// Independent oracle: enumerate EVERY subset of the candidates with a bitmask and
// validate each one directly. The engine's optimum must equal this minimum.
static void testOptimalityAgainstOracle() {
    section("Optimality vs independent brute-force oracle");
    CostModel cost;
    RepairEngine engine(cost, true);
    for (const Scenario& s : buildScenarios()) {
        for (const User& u : s.policy.users()) {
            if (ViolationDetector::detectForUser(s.policy, u).empty()) continue;
            std::vector<RepairAction> acts = engine.generateCandidates(s.policy, u);
            int oracleBest = -1;
            for (unsigned mask = 0; mask < (1u << acts.size()); ++mask) {
                User sim = u;
                int c = 0;
                for (std::size_t i = 0; i < acts.size(); ++i)
                    if (mask & (1u << i)) { acts[i].applyTo(sim); c += acts[i].cost(); }
                if (ValidationEngine::checkUser(s.policy, u, s.policy, sim).passed() && (oracleBest < 0 || c < oracleBest))
                    oracleBest = c;
            }
            UserRepairResult r = engine.solveUser(s.policy, u);
            std::string tag = "scenario " + std::to_string(s.id) + " user " + u.name() + ": ";
            CHECK(r.solvable == (oracleBest >= 0), tag + "solvability matches oracle");
            if (r.solvable) CHECK(r.best.cost == oracleBest, tag + "cost " + std::to_string(r.best.cost) + " == oracle " + std::to_string(oracleBest));
        }
    }
}

static void testSafetyProperties() {
    section("Safety properties");
    std::vector<Scenario> all = buildScenarios();
    CostModel cost;

    // Scenario 2: teacher keeps teaching access.
    const Scenario& s2 = all[1];
    PipelineResult r2 = runPipeline(s2.policy, cost, true);
    const User* tom = r2.repaired.findUser("tom");
    PermissionSet tomPerms = r2.repaired.effectivePermissions(*tom);
    CHECK(tomPerms.contains("grades.view") && tomPerms.contains("grades.edit") && !tomPerms.contains("accounts.manage"),
          "teacher keeps grades.view/edit and loses accounts.manage");

    // Simulation must not touch the original.
    const User* before = s2.policy.findUser("tom");
    CHECK(before->directPermissions().contains("accounts.manage"), "original configuration untouched by simulation");

    // Required permissions are never removed: every user in every solvable scenario satisfies required sets.
    for (const Scenario& s : all) {
        PipelineResult r = runPipeline(s.policy, cost, true);
        if (!r.simulated) continue;
        bool requiredOk = true;
        for (const User& u : r.repaired.users())
            requiredOk = requiredOk && r.repaired.rulesFor(u).required.isSubsetOf(r.repaired.effectivePermissions(u));
        CHECK(requiredOk, "scenario " + std::to_string(s.id) + ": required permissions present after repair");
    }

    // A deliberately BAD repair must be caught by validation (remove the Teacher role entirely).
    SecurityPolicy bad = ValidationEngine::simulate(s2.policy, {RepairAction(RepairType::REMOVE_ROLE, "tom", "Teacher", 2)});
    ValidationReport rep = ValidationEngine::validate(s2.policy, bad);
    CHECK(!rep.passed, "validation rejects a repair that destroys legitimate access");

    // Scenario 6: zero violations and zero cost, no actions.
    PipelineResult r6 = runPipeline(all[5].policy, cost, true);
    CHECK(r6.plan.totalCost == 0 && r6.plan.allActions().empty() && r6.plan.perUser.empty(), "secure system: zero cost, no actions");
}

static void testCostModel() {
    section("Cost model");
    const std::vector<Scenario> all = buildScenarios();
    const Scenario& s3 = all[2];   // GRANT (1) vs ADD-ROLE (2) for sam
    CostModel weighted;
    CHECK(runPipeline(s3.policy, weighted, true).plan.totalCost == 1, "default weights: grant is cheapest (1)");
    CostModel flipped;
    flipped.setWeight(RepairType::GRANT_DIRECT, 5);
    flipped.setWeight(RepairType::ADD_ROLE, 1);
    PipelineResult r = runPipeline(s3.policy, flipped, true);
    CHECK(r.plan.totalCost == 1 && r.plan.allActions().size() == 1 && r.plan.allActions()[0].type() == RepairType::ADD_ROLE,
          "changed weights change the chosen repair (ADD-ROLE now cheapest)");
    CostModel uniform;
    uniform.setUniform(true);
    CHECK(runPipeline(all[3].policy, uniform, true).plan.totalCost == 4, "uniform cost = number of changes");
}

static void testEdgeCases() {
    section("Edge cases");
    SecurityPolicy empty;
    CostModel cost;
    PipelineResult r = runPipeline(empty, cost, true);
    CHECK(r.userViolations == 0 && r.plan.solvable && r.plan.totalCost == 0, "empty configuration: no violations, cost 0");
    CHECK(ViolationDetector::detectAll(empty).empty(), "empty configuration: empty violation list");

    SecurityPolicy noRoles;  // a user with a direct permission but no roles at all
    noRoles.addPermission(Permission("p.one", ""));
    User lone("lone");
    lone.grantDirect("p.one");
    noRoles.addUser(lone);
    PipelineResult r2 = runPipeline(noRoles, cost, true);
    CHECK(r2.userViolations == 1 && r2.plan.solvable && r2.plan.totalCost == 1, "role-less user with direct permission: revoke it");

    CHECK(!noRoles.addUser(lone), "duplicate user rejected");
    CHECK(noRoles.findUser("nobody", nullptr) == nullptr, "missing user lookup returns null");

    // Too many candidates must not hang or crash.
    SecurityPolicy big;
    Role role("R");
    User u("u");
    for (int i = 0; i < 25; ++i) {
        std::string p = "perm" + std::to_string(i);
        big.addPermission(Permission(p, ""));
        role.addForbidden(p);
        u.grantDirect(p);
    }
    big.addRole(role);
    u.addRole("R");
    big.addUser(u);
    PipelineResult r3 = runPipeline(big, cost, true);
    CHECK(!r3.plan.solvable && r3.plan.perUser[0].searchTooLarge, "oversized search space is refused with an explanation");
}


// ---------------------------------------------------------------- risk weights
static void testRiskWeights() {
    section("Per-permission risk weights");
    std::vector<Scenario> all = buildScenarios();
    CostModel weighted;
    CHECK(runPipeline(all[0].policy, weighted, true).plan.totalCost == 3, "revoking grades.edit (risk 3) costs 1 x 3 = 3");
    CHECK(runPipeline(all[1].policy, weighted, true).plan.totalCost == 5, "revoking accounts.manage (risk 5) costs 1 x 5 = 5");
    CHECK(runPipeline(all[2].policy, weighted, true).plan.totalCost == 1, "granting attendance.mark (risk 1) costs 1");
    CostModel uniform;
    uniform.setUniform(true);
    CHECK(runPipeline(all[0].policy, uniform, true).plan.totalCost == 1, "uniform mode ignores risk (1 change = 1)");
    CHECK(runPipeline(all[1].policy, uniform, true).plan.totalCost == 1, "uniform mode ignores risk (scenario 2)");
    CHECK(weighted.costOf(RepairType::REMOVE_ROLE, 9) == 2, "role actions are not scaled by risk");
    CHECK(Permission("x", "", 99).riskWeight() == Permission::kMaxRisk && Permission("x", "", -4).riskWeight() == Permission::kMinRisk,
          "risk weight is clamped to 1..10");

    // Risk changes WHICH repair wins: removing a role (2) beats revoking a risk-9 permission (9).
    SecurityPolicy p;
    p.addPermission(Permission("base", "", 1));
    p.addPermission(Permission("hot", "", 9));
    Role r("R");
    r.addGrant("base"); r.addAllowed("base"); r.addRequired("base");
    p.addRole(r);
    Role extra("Extra");
    extra.addGrant("hot"); extra.addAllowed("hot");
    Role guard("Guard");
    guard.addForbidden("hot");
    p.addRole(extra);
    p.addRole(guard);
    User u("u");
    u.addRole("R"); u.addRole("Guard"); u.addRole("Extra");   // Extra grants hot, Guard forbids it
    u.grantDirect("hot");                                      // ... and hot is also assigned directly
    p.addUser(u);
    PipelineResult res = runPipeline(p, weighted, true);
    // Removing role Extra (2) does not help alone (hot still direct); revoking hot (9) alone does not help
    // (Extra still grants it). Both are required: 2 + 9 = 11.
    CHECK(res.plan.solvable && res.plan.totalCost == 11, "risk-weighted combination: remove role (2) + revoke risk-9 permission (9) = 11");
    CHECK(runPipeline(p, uniform, true).plan.totalCost == 2, "same case in uniform mode costs 2");
}

// ---------------------------------------------------------------- separation of duties
static void testSeparationOfDuties() {
    section("Separation of duties");
    PermissionSet a{"x", "y", "z"};
    SodRule rule("R", PermissionSet{"x", "y"});
    CHECK(rule.heldConflict(PermissionSet{"x"}).empty(), "holding one exclusive permission is fine");
    CHECK(rule.heldConflict(PermissionSet{"x", "y", "q"}) == PermissionSet({"x", "y"}), "holding both is a conflict (set intersection)");
    CHECK(SodRule("T", PermissionSet{"x", "y", "z"}).heldConflict(PermissionSet{"x", "z"}).size() == 2, "3-permission rule: two held is a conflict");

    Scenario s7 = buildScenarios()[6];
    ViolationList v = ViolationDetector::detectAll(s7.policy);
    bool sodFound = false;
    for (const Violation& x : v) sodFound = sodFound || x.type() == ViolationType::SOD_CONFLICT;
    CHECK(sodFound, "scenario 7: SoD conflict detected");
    CostModel cost;
    PipelineResult r = runPipeline(s7.policy, cost, true);
    CHECK(r.plan.solvable && r.plan.totalCost == 2, "scenario 7: minimum cost 2");
    std::vector<RepairAction> acts = r.plan.allActions();
    CHECK(acts.size() == 1 && acts[0].type() == RepairType::REMOVE_ROLE && acts[0].target() == "ComplianceOfficer",
          "scenario 7: removes ComplianceOfficer only");
    const User* finn = r.repaired.findUser("finn");
    PermissionSet perms = r.repaired.effectivePermissions(*finn);
    CHECK(perms.contains("accounts.manage") && !perms.contains("audit.export") && perms.contains("logs.read") &&
          perms.contains("system.settings"), "finn keeps admin access (incl. logs.read) and loses audit.export");
    CHECK(r.validation.passed, "scenario 7: validation passes (incl. SoD check)");

    // Validation must reject a 'repair' that leaves the SoD rule broken, and one that drops both sides.
    SecurityPolicy noChange = ValidationEngine::simulate(s7.policy, {});
    CHECK(!ValidationEngine::validate(s7.policy, noChange).passed, "validation rejects: SoD still broken");
    SecurityPolicy dropBoth = ValidationEngine::simulate(
        s7.policy, {RepairAction(RepairType::REMOVE_ROLE, "finn", "Admin", 2), RepairAction(RepairType::REMOVE_ROLE, "finn", "ComplianceOfficer", 2)});
    CHECK(!ValidationEngine::validate(s7.policy, dropBoth).passed, "validation rejects: removing BOTH sides loses all access");

    // Two roles that each require one side of the rule: removing ONE role (which also drops its
    // requirement) resolves the conflict while the other side stays intact.
    SecurityPolicy two;
    two.addPermission(Permission("a", "")); two.addPermission(Permission("b", ""));
    Role ra("RA"); ra.addGrant("a"); ra.addAllowed("a"); ra.addRequired("a");
    Role rb("RB"); rb.addGrant("b"); rb.addAllowed("b"); rb.addRequired("b");
    two.addRole(ra); two.addRole(rb);
    two.addSodRule(SodRule("A-B", PermissionSet{"a", "b"}));
    User both("u"); both.addRole("RA"); both.addRole("RB");
    two.addUser(both);
    PipelineResult feasible = runPipeline(two, cost, true);
    CHECK(feasible.userViolations == 1 && feasible.plan.solvable && feasible.plan.totalCost == 2 && feasible.validation.passed,
          "two roles, one side each: solvable by removing one role (cost 2)");

    // Impossible SoD: ONE role requires both conflicting permissions. It cannot stay (rule broken) and
    // cannot go (all access lost, and nothing may be granted directly without a role to allow it).
    SecurityPolicy p;
    p.addPermission(Permission("a", "")); p.addPermission(Permission("b", ""));
    Role both_r("R"); both_r.addGrant("a"); both_r.addGrant("b"); both_r.addAllowed("a"); both_r.addAllowed("b");
    both_r.addRequired("a"); both_r.addRequired("b");
    p.addRole(both_r);
    p.addSodRule(SodRule("A-B", PermissionSet{"a", "b"}));
    User u("u"); u.addRole("R");
    p.addUser(u);
    PipelineResult imp = runPipeline(p, cost, true);
    CHECK(imp.userViolations == 1 && !imp.plan.solvable, "one role requires both sides of a SoD rule: detected, no safe repair");
    CHECK(!imp.plan.perUser.empty() && !imp.plan.perUser[0].explanation.empty(), "impossible SoD case comes with an explanation");
}

// ---------------------------------------------------------------- role hierarchy
static void testRoleHierarchy() {
    section("Role hierarchy (graph algorithms)");
    SecurityPolicy p;
    for (const char* id : {"a", "b", "c", "d"}) p.addPermission(Permission(id, ""));
    auto role = [&](const char* name, const char* grant, const char* forbid) {
        Role r(name);
        if (*grant) { r.addGrant(grant); r.addAllowed(grant); }
        if (*forbid) r.addForbidden(forbid);
        p.addRole(r);
    };
    role("Base", "a", ""); role("Mid1", "b", "d"); role("Mid2", "c", ""); role("Top", "", "");
    std::string error;
    CHECK(RoleHierarchy::addInheritance(p, "Mid1", "Base", error), "Mid1 -> Base");
    CHECK(RoleHierarchy::addInheritance(p, "Mid2", "Base", error), "Mid2 -> Base");
    CHECK(RoleHierarchy::addInheritance(p, "Top", "Mid1", error), "Top -> Mid1");
    CHECK(RoleHierarchy::addInheritance(p, "Top", "Mid2", error), "Top -> Mid2 (diamond shape)");

    ResolvedRole top = p.resolvedRole("Top");
    CHECK(top.grants == PermissionSet({"a", "b", "c"}), "resolved grants union all ancestors (diamond counted once)");
    CHECK(top.forbidden == PermissionSet({"d"}), "forbidden rules are inherited");
    CHECK(p.resolvedRole("Base").grants == PermissionSet({"a"}), "a root role resolves to itself");
    CHECK(RoleHierarchy::ancestors(p, "Top") == std::vector<std::string>({"Base", "Mid1", "Mid2"}), "ancestors of Top");
    CHECK(RoleHierarchy::ancestors(p, "Base").empty(), "root has no ancestors");
    CHECK(RoleHierarchy::depth(p, "Base") == 0 && RoleHierarchy::depth(p, "Mid1") == 1 && RoleHierarchy::depth(p, "Top") == 2,
          "depths 0 / 1 / 2");

    bool ok = false;
    std::vector<std::string> order = RoleHierarchy::topologicalOrder(p, ok);
    auto pos = [&](const char* n) { return std::find(order.begin(), order.end(), n) - order.begin(); };
    CHECK(ok && order.size() == 4 && pos("Base") < pos("Mid1") && pos("Base") < pos("Mid2") && pos("Mid1") < pos("Top") && pos("Mid2") < pos("Top"),
          "topological order lists parents before children");

    // Rejections.
    CHECK(!RoleHierarchy::addInheritance(p, "Base", "Top", error) && error.find("cycle") != std::string::npos,
          "cycle Base -> Top rejected: " + error);
    CHECK(error.find("Base -> Top -> ") != std::string::npos && error.rfind("Base") == error.size() - 4, "error shows the full cycle path");
    CHECK(!RoleHierarchy::addInheritance(p, "Mid1", "Mid1", error), "self inheritance rejected");
    CHECK(!RoleHierarchy::addInheritance(p, "Ghost", "Base", error) && !RoleHierarchy::addInheritance(p, "Base", "Ghost", error),
          "unknown roles rejected");
    CHECK(!RoleHierarchy::addInheritance(p, "Mid1", "Base", error), "duplicate link rejected");
    CHECK(p.findRole("Base")->parents().empty(), "failed additions leave the graph unchanged");

    // Even if a cycle were forced in by low-level calls, resolution and ordering terminate.
    SecurityPolicy loop = p;
    loop.mutableRole("Base")->addParent("Top");
    CHECK(loop.resolvedRole("Top").grants.contains("a"), "resolvedRole terminates on a forced cycle");
    RoleHierarchy::topologicalOrder(loop, ok);
    CHECK(!ok, "topological order reports the cycle");
    CHECK(RoleHierarchy::render(loop).size() < 1000, "render terminates on a forced cycle");

    // Inherited access is checked by the detector.
    Scenario s8 = buildScenarios()[7];
    ViolationList v = ViolationDetector::detectAll(s8.policy);
    bool forbiddenViaInheritance = false;
    for (const Violation& x : v) forbiddenViaInheritance = forbiddenViaInheritance ||
        (x.type() == ViolationType::FORBIDDEN_PERMISSION && x.userName() == "hana" && x.permission() == "accounts.manage");
    CHECK(forbiddenViaInheritance, "scenario 8: forbidden permission found through an INHERITED rule");
    const User* hugo = s8.policy.findUser("hugo");
    CHECK(s8.policy.effectivePermissions(*hugo).contains("grades.edit"), "scenario 8: hugo receives Teacher's grants by inheritance");
    CHECK(s8.policy.sourcesOf(*hugo, "grades.edit").find("inherited") != std::string::npos, "source text says 'inherited'");

    // Without the inheritance link the same direct permission is NOT a violation.
    SecurityPolicy flat = s8.policy;
    SecurityPolicy noLink;
    for (const Permission& pm : flat.permissions()) noLink.addPermission(pm);
    for (const Role& r : flat.roles()) { Role copy(r.name()); if (r.name() != "HeadTeacher") noLink.addRole(r); else { copy.addGrant("logs.read"); copy.addAllowed("logs.read"); noLink.addRole(copy); } }
    noLink.addUser(*flat.findUser("hana"));
    bool unauthorized = false, forbidden = false;
    for (const Violation& x : ViolationDetector::detectForUser(noLink, *noLink.findUser("hana"))) {
        unauthorized = unauthorized || x.type() == ViolationType::UNAUTHORIZED_PERMISSION;
        forbidden = forbidden || x.type() == ViolationType::FORBIDDEN_PERMISSION;
    }
    CHECK(unauthorized && !forbidden, "without the inheritance link accounts.manage is only 'unauthorized' (the FORBIDDEN rule came from the parent)");
}

// ---------------------------------------------------------------- compliance score
static void testComplianceScore() {
    section("Compliance score");
    std::vector<Scenario> all = buildScenarios();
    CHECK(ComplianceScorer::evaluate(all[5].policy).overall == 100.0, "secure system scores 100");
    CHECK(ComplianceScorer::evaluate(all[5].policy).rating == "SECURE", "secure system rating");
    ComplianceReport s1 = ComplianceScorer::evaluate(all[0].policy);   // alice: 1 critical = -25; bob 100
    CHECK(s1.overall == 87.5 && s1.users[0].score == 75 && s1.users[1].score == 100, "scenario 1: 75 and 100 -> 87.5");
    CHECK(s1.rating == "MODERATE RISK", "scenario 1 rating");
    ComplianceReport s4 = ComplianceScorer::evaluate(all[3].policy);   // dave: 25+25+8=58 -> 42 ; eve 75
    CHECK(s4.users[0].score == 42 && s4.users[1].score == 75 && s4.overall == 58.5 && s4.rating == "HIGH RISK", "scenario 4: 42 and 75 -> 58.5 HIGH RISK");
    CostModel cost;
    for (const Scenario& s : all) {
        PipelineResult r = runPipeline(s.policy, cost, true);
        if (r.simulated) CHECK(r.scoreAfter == 100.0, "scenario " + std::to_string(s.id) + ": score is 100 after a validated repair");
        else CHECK(r.scoreAfter < 0, "scenario " + std::to_string(s.id) + ": no 'after' score without a repair");
        CHECK(r.scoreBefore <= 100.0 && r.scoreBefore >= 0.0, "scenario " + std::to_string(s.id) + ": score within 0..100");
    }
    SecurityPolicy empty;
    CHECK(ComplianceScorer::evaluate(empty).overall == 100.0 && ComplianceScorer::evaluate(empty).users.empty(), "empty configuration scores 100");

    // Floor at 0: 5 critical violations = -125 but the score never goes below 0.
    SecurityPolicy p;
    Role r("R");
    for (int i = 0; i < 5; ++i) { std::string id = "p" + std::to_string(i); p.addPermission(Permission(id, "")); r.addForbidden(id); }
    p.addRole(r);
    User u("u"); u.addRole("R");
    for (int i = 0; i < 5; ++i) u.grantDirect("p" + std::to_string(i));
    p.addUser(u);
    ComplianceReport floor = ComplianceScorer::evaluate(p);
    CHECK(floor.users[0].score == 0 && floor.overall == 0.0 && floor.rating == "CRITICAL RISK", "score is floored at 0");
    CHECK(ComplianceScorer::ratingFor(95) == "LOW RISK" && ComplianceScorer::ratingFor(70) == "MODERATE RISK" &&
          ComplianceScorer::ratingFor(50) == "HIGH RISK" && ComplianceScorer::ratingFor(49.9) == "CRITICAL RISK", "rating thresholds");
}

// ---------------------------------------------------------------- randomized optimality check
// Builds many random policies (random roles, hierarchy, SoD rules, direct permissions, risks) and checks
// the engine, with AND without pruning, against the independent bit-mask oracle.
static void testRandomizedOptimality() {
    section("Randomized optimality vs oracle (seeded, 400 policies)");
    std::mt19937 rng(20261126);
    auto chance = [&](int percent) { return static_cast<int>(rng() % 100) < percent; };
    int policiesWithViolations = 0, usersChecked = 0, unsolvable = 0, mismatches = 0, sodUsers = 0;
    for (int round = 0; round < 400; ++round) {
        SecurityPolicy p;
        const int permCount = 6;
        for (int i = 0; i < permCount; ++i) p.addPermission(Permission("p" + std::to_string(i), "", 1 + static_cast<int>(rng() % 5)));
        const int roleCount = 2 + static_cast<int>(rng() % 3);
        for (int i = 0; i < roleCount; ++i) {
            Role r("R" + std::to_string(i));
            for (int k = 0; k < permCount; ++k) {
                std::string id = "p" + std::to_string(k);
                if (chance(35)) r.addGrant(id);
                if (chance(70)) r.addAllowed(id);
                if (chance(10)) r.addForbidden(id);
                if (chance(8)) r.addRequired(id);
            }
            for (int parent = 0; parent < i; ++parent) if (chance(25)) r.addParent("R" + std::to_string(parent));  // acyclic by construction
            p.addRole(r);
        }
        if (chance(60)) p.addSodRule(SodRule("S", PermissionSet{"p" + std::to_string(rng() % 3), "p" + std::to_string(3 + rng() % 3)}));
        const int userCount = 1 + static_cast<int>(rng() % 2);
        for (int i = 0; i < userCount; ++i) {
            User u("u" + std::to_string(i));
            for (int k = 0; k < roleCount; ++k) if (chance(45)) u.addRole("R" + std::to_string(k));
            for (int k = 0; k < permCount; ++k) if (chance(25)) u.grantDirect("p" + std::to_string(k));
            p.addUser(u);
        }
        bool any = false;
        for (const User& u : p.users()) {
            auto found = ViolationDetector::detectForUser(p, u);
            if (found.empty()) continue;
            any = true;
            for (const Violation& v : found) if (v.type() == ViolationType::SOD_CONFLICT) { ++sodUsers; break; }
            CostModel cost;
            RepairEngine pruned(cost, true), brute(cost, false);
            std::vector<RepairAction> acts = pruned.generateCandidates(p, u);
            if (acts.size() > 13) continue;   // keep the oracle fast
            int oracleBest = -1;
            for (unsigned mask = 0; mask < (1u << acts.size()); ++mask) {
                User sim = u;
                int c = 0;
                for (std::size_t i = 0; i < acts.size(); ++i) if (mask & (1u << i)) { acts[i].applyTo(sim); c += acts[i].cost(); }
                if (ValidationEngine::checkUser(p, u, p, sim).passed() && (oracleBest < 0 || c < oracleBest)) oracleBest = c;
            }
            UserRepairResult a = pruned.solveUser(p, u), b = brute.solveUser(p, u);
            ++usersChecked;
            if (oracleBest < 0) ++unsolvable;
            bool same = a.solvable == (oracleBest >= 0) && b.solvable == a.solvable &&
                        (!a.solvable || (a.best.cost == oracleBest && b.best.cost == oracleBest));
            if (!same) { ++mismatches; std::cout << "  mismatch in round " << round << " user " << u.name() << ": oracle " << oracleBest
                                                    << " pruned " << (a.solvable ? a.best.cost : -1) << " brute " << (b.solvable ? b.best.cost : -1) << "\n"; }
        }
        if (any) ++policiesWithViolations;
    }
    std::cout << "  policies with violations: " << policiesWithViolations << ", users checked: " << usersChecked
              << " (unsolvable: " << unsolvable << ", with SoD conflicts: " << sodUsers << ")\n";
    CHECK(mismatches == 0, "pruned and brute-force engine both equal the oracle on every random user");
    CHECK(usersChecked >= 100, "the random test actually exercised at least 100 users");
    CHECK(unsolvable > 0 && unsolvable < usersChecked, "the random test covers both solvable and unsolvable cases");
    CHECK(sodUsers > 0, "the random test covers separation-of-duties conflicts");
}

static void testPruningScales() {
    section("Pruning vs brute force on a larger user (14 forbidden permissions)");
    SecurityPolicy p;
    Role role("R");
    User u("u");
    for (int i = 0; i < 14; ++i) {
        std::string perm = "perm" + std::to_string(i);
        p.addPermission(Permission(perm, ""));
        role.addForbidden(perm);
        u.grantDirect(perm);
    }
    u.addRole("R");
    p.addRole(role);
    p.addUser(u);
    PipelineResult pruned = runPipeline(p, CostModel(), true);
    PipelineResult brute = runPipeline(p, CostModel(), false);
    const SearchStats& a = pruned.plan.totals;
    const SearchStats& b = brute.plan.totals;
    std::cout << "  brute force : nodes=" << b.nodesVisited << " explored=" << b.explored << " valid=" << b.valid << "\n";
    std::cout << "  backtracking: nodes=" << a.nodesVisited << " explored=" << a.explored << " valid=" << a.valid
              << " pruned=" << a.pruned << "\n";
    CHECK(pruned.plan.totalCost == 14 && brute.plan.totalCost == 14, "both modes find the optimal cost 14");
    CHECK(a.nodesVisited < b.nodesVisited && a.explored < b.explored, "pruning visits and evaluates fewer combinations");
}

static void testConfigIO() {
    section("Config file parsing");
    for (const Scenario& s : buildScenarios()) {
        std::ostringstream out;
        ConfigIO::write(out, s.policy);
        SecurityPolicy loaded;
        std::string error;
        std::istringstream in(out.str());
        bool ok = ConfigIO::parse(in, loaded, error);
        CHECK(ok, "scenario " + std::to_string(s.id) + " round-trips: " + error);
        if (ok) {
            PipelineResult a = runPipeline(s.policy, CostModel(), true), b = runPipeline(loaded, CostModel(), true);
            CHECK(a.plan.totalCost == b.plan.totalCost && a.userViolations == b.userViolations,
                  "scenario " + std::to_string(s.id) + " gives identical results after save/load");
        }
    }
    auto expectError = [](const std::string& text, const std::string& label) {
        SecurityPolicy p;
        p.addPermission(Permission("keep.me", ""));
        std::string error;
        std::istringstream in(text);
        bool ok = ConfigIO::parse(in, p, error);
        CHECK(!ok && !error.empty(), label + " -> error: " + error);
        CHECK(p.permissions().size() == 1, label + ": policy unchanged after failed load");
    };
    expectError("role R\n  grants nope\n", "unknown permission");
    expectError("permission a | x\nuser u\n  roles Ghost\n", "unknown role");
    expectError("permission a | x\npermission a | y\n", "duplicate permission");
    expectError("garbage line here\n", "unexpected keyword");
    expectError("permission bad name | x\n", "invalid permission id");
    expectError("permission a risk=0 | x\n", "risk below range");
    expectError("permission a risk=11 | x\n", "risk above range");
    expectError("permission a risk=high | x\n", "risk not a number");
    expectError("permission a b c | x\n", "too many words in permission line");
    expectError("permission a | x\nsod S a\n", "sod rule with one permission");
    expectError("permission a | x\nsod S a a\n", "sod rule with duplicate permission");
    expectError("permission a | x\npermission b | x\nsod S a nope\n", "sod rule with unknown permission");
    expectError("permission a | x\npermission b | x\nsod S a b\nsod S a b\n", "duplicate sod rule");
    expectError("role Child\n  inherits Parent\n", "inherits an undeclared parent");
    {
        // risk, sod and inheritance survive a save/load round trip, and children may be listed before parents in memory.
        SecurityPolicy src;
        src.addPermission(Permission("a", "alpha", 7));
        src.addPermission(Permission("b", "beta"));
        src.addRole(Role("Child"));
        src.addRole(Role("Parent"));
        std::string e;
        RoleHierarchy::addInheritance(src, "Child", "Parent", e);   // Parent is stored AFTER Child
        src.addSodRule(SodRule("S", PermissionSet{"a", "b"}));
        std::ostringstream out;
        ConfigIO::write(out, src);
        SecurityPolicy back;
        std::string err;
        std::istringstream in(out.str());
        bool parsed = ConfigIO::parse(in, back, err);
        CHECK(parsed, "extended format round-trips: " + err);
        if (parsed) {
            CHECK(back.findPermission("a")->riskWeight() == 7 && back.findPermission("b")->riskWeight() == 1, "risk weights survive save/load");
            CHECK(back.sodRules().size() == 1 && back.sodRules()[0].exclusive() == PermissionSet({"a", "b"}), "SoD rule survives save/load");
            CHECK(back.findRole("Child")->parents().count("Parent") == 1, "inheritance survives save/load");
        }
    }
    SecurityPolicy p;
    std::string error;
    std::istringstream emptyInput("");
    CHECK(ConfigIO::parse(emptyInput, p, error) && p.empty(), "empty file loads as empty configuration");
    CHECK(!ConfigIO::load("does/not/exist.txt", p, error), "missing file reports an error");
}

static void testAuditLogger() {
    section("Audit logger");
    AuditLogger log;
    log.log(AuditCategory::VIOLATION, "user alice holds grades.edit");
    log.log(AuditCategory::SELECTION, "best cost 1");
    log.log(AuditCategory::VIOLATION, "user bob holds grades.edit");
    std::ostringstream sink;
    std::size_t comparisons = 0;
    CHECK(log.search(sink, "ALICE", comparisons) == 1 && comparisons == 3, "case-insensitive keyword search scans all 3 nodes");
    CHECK(log.removeCategory(AuditCategory::VIOLATION) == 2 && log.size() == 1, "category deletion removes nodes");
}


static bool contains(const std::string& hay, const std::string& needle) { return hay.find(needle) != std::string::npos; }

static void testWebApi() {
    section("Web API (HTTP-independent part)");
    CHECK(WebApi::jsonString("a\"b\\c\n\t") == "\"a\\\"b\\\\c\\n\\t\"", "JSON string escaping");
    CHECK(WebApi::jsonString(std::string("x\x01y")) == "\"x\\u0001y\"", "control characters become \\u00xx");

    HttpResponse page = WebApi::handle("GET", "/", "", "");
    CHECK(page.status == 200 && contains(page.contentType, "text/html") && contains(page.body, "Privilege Tetris"), "GET / serves the page");
    CHECK(WebApi::handle("GET", "/nope", "", "").status == 404, "unknown path -> 404");
    CHECK(WebApi::handle("GET", "/api/analyze", "", "").status == 405, "wrong method -> 405");

    HttpResponse sc = WebApi::handle("GET", "/api/scenarios", "", "");
    CHECK(sc.status == 200 && contains(sc.body, "\"scenarios\"") && contains(sc.body, "Multiple Violations"), "scenario list served");

    // Every scenario, sent as text through the API, gives the same cost as the engine itself.
    std::vector<Scenario> all = buildScenarios();
    for (const Scenario& s : all) {
        std::ostringstream cfg;
        ConfigIO::write(cfg, s.policy);
        HttpResponse r = WebApi::handle("POST", "/api/analyze", "", cfg.str());
        bool solvable = s.expectedCost >= 0;
        CHECK(r.status == 200 && contains(r.body, "\"ok\":true"), "scenario " << s.id << " analysed through the API");
        CHECK(contains(r.body, std::string("\"plan\":{\"solvable\":") + (solvable ? "true" : "false")), "scenario " << s.id << " solvability reported");
        if (solvable && s.expectedCost > 0)
            CHECK(contains(r.body, "\"totalCost\":" + std::to_string(s.expectedCost)), "scenario " << s.id << " cost " << s.expectedCost);
        CHECK(contains(r.body, solvable ? "\"validation\":{\"passed\":true" : "\"repaired\":null"), "scenario " << s.id << " validation / no repair");
    }

    HttpResponse bad = WebApi::handle("POST", "/api/parse", "", "user x\n  roles Nope\n");
    CHECK(bad.status == 400 && contains(bad.body, "unknown role") && contains(bad.body, "line 2"), "parse error reports the line");
    HttpResponse ok = WebApi::handle("POST", "/api/parse", "", "permission a risk=4 | Alpha\nrole R\n  grants a\nuser u\n  roles R\n");
    CHECK(ok.status == 200 && contains(ok.body, "\"risk\":4") && contains(ok.body, "\"effective\":[{\"permission\":\"a\""), "parse returns the structured policy");
    CHECK(WebApi::handle("POST", "/api/analyze", "", std::string(WebApi::kMaxBodyBytes + 1, 'x')).status == 400, "oversized body refused");
    CHECK(contains(WebApi::handle("POST", "/api/analyze", "", "").body, "\"ok\":true"), "empty configuration analyses fine");

    // Options in the query string reach the engine (uniform cost turns scenario 4 from 12 into 4).
    std::ostringstream s4;
    ConfigIO::write(s4, all[3].policy);
    CHECK(contains(WebApi::handle("POST", "/api/analyze", "uniform=1", s4.str()).body, "\"totalCost\":4"), "uniform=1 gives cost 4");
    CHECK(contains(WebApi::handle("POST", "/api/analyze", "pruning=0", s4.str()).body, "\"totalCost\":12"), "pruning=0 gives the same optimum");
    CHECK(contains(WebApi::handle("POST", "/api/analyze", "revoke=abc&grant=-5", s4.str()).body, "\"ok\":true"), "garbage numbers fall back to defaults");

    HttpResponse esc = WebApi::handle("POST", "/api/parse", "", "permission a | say \"hi\" <b>x</b>\n");
    CHECK(contains(esc.body, "say \\\"hi\\\" <b>x</b>"), "description is JSON-escaped");
}

int main() {
    testLinkedList();
    testPermissionSet();
    testSortingAndSearch();
    testScenarios();
    testOptimalityAgainstOracle();
    testSafetyProperties();
    testCostModel();
    testEdgeCases();
    testRiskWeights();
    testSeparationOfDuties();
    testRoleHierarchy();
    testComplianceScore();
    testRandomizedOptimality();
    testPruningScales();
    testConfigIO();
    testAuditLogger();
    testWebApi();
    std::cout << "\n==============================\nPassed: " << g_passed << "   Failed: " << g_failed << "\n";
    return g_failed == 0 ? 0 : 1;
}
