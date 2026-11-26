# Algorithms, Data Structures and Complexity

Notation: **U** users, **R** roles, **P** permissions, **r** roles held by one user,
**k** candidate repair actions for one user, **V** violations, **n** list/array length.

---

## 1. Pipeline

```
load config -> detect violations -> generate candidates -> BACKTRACKING search
          -> simulate on a COPY -> validate -> (confirm) -> apply -> report/log
```

## 2. Data structures

### 2.1 Custom doubly linked list (`LinkedList.h`)
```
head_ <-> [node] <-> [node] <-> [node] <-> tail_
```
Each node stores data plus `prev`/`next`. The list owns its nodes and frees them in the destructor
(RAII); copying is a deep copy (copy-and-swap assignment). Used for:

* `ViolationList` – detected violations (insert, traverse, linear search by user, sorted by rebuilding).
* `AuditLogger` – audit history (`pushBack`, traversal, keyword search, `removeIf` deletion).

| Operation | Time |
|-----------|------|
| pushFront / pushBack | O(1) |
| insertAt(i) / at(i) / removeAt(i) | O(n) (walk to position; the unlink itself is O(1)) |
| find (linear search) | best O(1), avg/worst O(n) |
| removeIf, forEach, toVector, clear | O(n) |

### 2.2 `PermissionSet` (set operations)
Wraps `std::set<std::string>` (sorted balanced tree).

| Operation | Time |
|-----------|------|
| contains / insert / erase | O(log m) |
| union, intersection, difference (`std::set_*` over sorted ranges) | O(m1 + m2) to merge, O((m1+m2) log) including inserting into the result set |
| subset (`std::includes`) | O(m1 + m2) |

They identify the three kinds of problem: unauthorized `(eff \ allowed) \ forbidden`, forbidden
`eff ∩ forbidden`, missing `required \ eff`.

### 2.3 Linear search (`LinearSearch.h`)
Scan from the front until the predicate matches; returns the index and the **number of comparisons**
so the real execution can be shown (menus 4 and 17).
Best O(1) (first element), average ≈ n/2 = O(n), worst O(n) (last element or not found).
Used for: finding users, roles, permissions (`SecurityPolicy`), auditing which permissions a user holds
(menu 4), searching linked-list nodes (violations, audit log).

### 2.4 Merge sort (`Sorting.h`)
Split in half, sort both halves recursively, merge by repeatedly taking the smaller front element.
**Stable**, O(n log n) in best, average and worst case, O(n) extra space. Chosen because its running time
does not degrade on already-sorted or reverse-sorted input, and stability keeps equal items in a
predictable order (important for repeatable reports). Used for violations (severity, descending),
candidate actions (cost, ascending – this also makes pruning more effective) and alternative solutions
(cost, then number of actions).

## 3. Violation detection
For every user: compute effective permissions (union of role grants and direct permissions), the
user's rules (union over roles), then the three set expressions above. For every role: check
`required ∩ forbidden`, `required \ allowed`, `grants ∩ forbidden`, `grants \ allowed`, `required \ grants`.
For every separation-of-duties rule: `|effective ∩ rule.permissions| > 1` (see §6).
Role sets are always *resolved* first, i.e. merged with everything inherited from ancestor roles (see §7).
Findings go into the linked list and are merge-sorted: FORBIDDEN and SOD (critical) > UNAUTHORIZED/ROLE-POLICY (high) > MISSING (medium).

### Compliance score
Every user starts at 100 and loses points per violation: CRITICAL 25, HIGH 15, MEDIUM 8, LOW 3 (never below 0).
System score = average over users (100 if there are none). Rating: 100 SECURE, ≥90 LOW RISK, ≥70 MODERATE RISK, ≥50 HIGH RISK, else CRITICAL RISK.
Role-level advisories are not scored because the engine repairs users, not role definitions.
Example (scenario 4): dave has 2 critical + 1 medium = −58 → 42; eve 1 critical → 75; system score (42 + 75) / 2 = 58.5 (HIGH RISK); after repair 100.
Cost: O(violations) once the violations are known, i.e. the same as detection.

## 4. Repair actions and cost

Every permission carries a **risk weight 1-10** (default 1): how sensitive it is. Changing a sensitive
permission is a bigger change, so REVOKE and GRANT are multiplied by it.

| Action | Cost |
|--------|------|
| REVOKE direct permission | 1 × risk(permission) |
| GRANT direct permission | 1 × risk(permission) |
| REMOVE role | 2 |
| ADD role | 2 |

`Total cost = Σ cost(action)`. Uniform mode makes every action cost 1 ("number of changes") and ignores risk.
In the demo catalog: `grades.edit` risk 3, `accounts.manage` 5, `system.settings` 5, `audit.export` 3, `logs.read` 2, others 1.
Because costs are computed in `CostModel` only, the search code never changes when weights change.
Examples (tests `Risk weights` and `Cost model`): revoking `accounts.manage` costs 5 but revoking `attendance.mark` would cost 1;
with GRANT = 5 and ADD-ROLE = 1 the engine switches from granting `attendance.mark` to adding role `AttendanceClerk`.
Risk can also decide *which* combination wins: removing a role (2) is cheaper than revoking a risk-9 permission (9).

**Candidate generation** (only actions that touch a violated permission):
1. REVOKE every direct permission that is forbidden/unauthorized, or caught in a broken SoD rule.
2. REMOVE every role whose resolved (inherited) grants include such a permission.
3. For each permission that is missing *or would be lost by a role removal*: GRANT it directly (if not
   forbidden and in allowed/required), and ADD each other role that provides it (if that role grants nothing the user is forbidden to have).

Unsafe candidates are generated on purpose; they are rejected later by validation, never silently applied.

## 5. Backtracking search (the core)

For one user with candidates a0 … a(k−1) (sorted by cost), a solution is any **subset**. The recursion
decides one action at a time:

```
explore(i, cost):
    if pruning:
        if cost > bestCost                       -> backtrack      (cost bound)
        if some violated permission can no longer be fixed,
           or cost + cheapest-still-needed-fix > bestCost -> backtrack (lower bound)
    if i == k:   evaluate subset on a COPY of the user; if valid, keep if cheaper
                 (tie: fewer actions); return
    include a_i  : explore(i+1, cost + cost(a_i));  then UNDO the choice
    exclude a_i  : explore(i+1, cost)
```

**Both pruning rules are safe**: they only cut branches that cannot contain a solution cheaper than
the best one already found, so the result is the true minimum of the search space. With pruning
switched off (menu 16) the same code enumerates all 2^k subsets – the pure brute-force baseline.

### Worked example (scenario 4, user `dave`)
`dave` has role Assistant (allowed {grades.view, attendance.mark}, forbidden {grades.edit,
accounts.manage, system.settings}, required {grades.view, attendance.mark}) and direct permissions
{grades.edit, accounts.manage}. Violations: forbidden `grades.edit`, forbidden `accounts.manage`,
missing `attendance.mark`.

Candidates, sorted by cost (risk: attendance.mark 1, grades.edit 3, accounts.manage 5):

| | Action | Cost |
|-|--------|------|
| a0 | GRANT attendance.mark | 1 × 1 = 1 |
| a1 | ADD role AttendanceClerk | 2 |
| a2 | REVOKE grades.edit | 1 × 3 = 3 |
| a3 | REVOKE accounts.manage | 1 × 5 = 5 |

2^4 = 16 subsets. Validation accepts only subsets containing both revokes (otherwise a forbidden
permission remains) plus a0 or a1 (otherwise a required permission is missing):

| subset | cost | valid? |
|--------|------|--------|
| {a0,a2,a3} | 9 | **valid – minimum** |
| {a1,a2,a3} | 10 | valid (not met by the pruned search: it can no longer beat 9) |
| {a0,a1,a2,a3} | 11 | valid |
| any other of the 13 | – | rejected (forbidden still held / outside allowed set) |

Measured: brute force evaluates all 16 (13 rejected, 3 valid). Backtracking with pruning evaluates
8 (6 rejected, 2 valid) – it meets the cost-11 solution first (include-everything branch), then the cost-9 one,
and prunes the rest. Both return cost 9. `eve` (one candidate, revoke `grades.edit`) costs 3, so the minimum total is **12**.

### Why the validation rules reject "clever" but unsafe repairs

| Rule | Prevents |
|------|----------|
| forbidden removed / allowed satisfied | leaving the original problem in place |
| required present | breaking operational access |
| **separation of duties satisfied** | leaving a conflicting pair of permissions in place |
| no new violations | fixing one problem by creating another |
| **legitimate access preserved** (`eff ∩ allowed \ forbidden` before ⊆ effective after; for a broken SoD rule at least one of its permissions must survive) | "fixing" a teacher by removing the whole Teacher role, or "fixing" SoD by removing all access |
| **no escalation** (`effAfter \ effBefore ⊆ required`) | "fixing" a missing permission by handing out Admin |

### Impossible repairs (scenario 5)
Role Auditor both requires and forbids `audit.export`. All 8 combinations of the 3 candidates are
rejected or pruned; the reasons are counted ("forbidden permission still held", "legitimate access lost",
"privilege escalation", …) and the report prints the conflict and the tally. Nothing is simulated or applied.

## 6. Separation of duties (SoD)

A rule keeps a *set* of permissions apart: no user may hold more than one of them. Example (scenario 7):
`SOD-ADMIN-AUDIT {accounts.manage, audit.export}` — whoever can create accounts must not also export the audit trail.

* **Detection:** `held = effective ∩ rule.permissions`; violation iff `|held| > 1` (a set intersection and a size test).
* **Candidates:** every held conflicting permission becomes a removal candidate (revoke if direct, remove the role that grants it).
* **Search:** a broken rule is one "target" for the pruning lower bound: at least one removal must still be paid for.
  It cannot be fixed by GRANT actions, so those never count as a fixer.
* **Validation:** no rule may remain broken, and at least one permission of each originally-broken rule must be kept.
  Those permissions are exempt from the normal "legitimate access preserved" rule, because by definition some must go.
  The engine chooses *which* one by cost.
* **Scenario 7:** finn holds Admin and ComplianceOfficer. 8 candidates, 256 combinations, 93 evaluated. Best = REMOVE-ROLE ComplianceOfficer (2): he keeps `accounts.manage`, `system.settings`, `logs.read`, `grades.view`.
  Removing Admin would lose legitimate access that cannot be granted back (not allowed by ComplianceOfficer); removing both is rejected by the "keep one" rule.

## 7. Role hierarchy (graph algorithms)

Roles form a directed acyclic graph: an edge child → parent means "child inherits parent". A role's
effective grants, allowed, forbidden and required sets are the union over the role and **all its ancestors**
(so a parent's `forbidden` rule also applies to every child — scenario 8).

| Algorithm | Used for | Idea | Time |
|-----------|----------|------|------|
| Iterative DFS with a visited set (`resolvedRole`) | merge sets of all ancestors | explicit stack; visited set makes diamonds count once and guarantees termination even on a bad cycle | O(R + E) |
| DFS cycle check (`wouldCreateCycle`) | refuse an edge child → parent | the edge closes a loop iff `child` is reachable from `parent`; the DFS trail is printed (`A -> B -> C -> A`) | O(R + E) |
| Kahn's algorithm (`topologicalOrder`) | list parents before children; also used when saving (parents must be declared first); reports a cycle if not all roles get output | repeatedly output roles whose parents are all output | O(R + E) |
| DFS ancestors / depth (memoised) | queries, tests, display | | O(R + E) |
| Recursive tree render | console hierarchy view | | O(R + E) |

(R = roles, E = inheritance links.) Edges are only added through `RoleHierarchy::addInheritance`, which rejects unknown roles, self-inheritance,
duplicate links and cycles; the config parser requires parents to be declared first, so a file cannot contain a cycle either.
Repairs never change the hierarchy; removing a role removes everything it inherits as well.

## 8. Time complexity

### 8.1 Per algorithm

| Algorithm | Best | Average | Worst | Space |
|-----------|------|---------|-------|-------|
| Linear search | O(1) | O(n) | O(n) | O(1) |
| Linked list insert at head/tail | O(1) | O(1) | O(1) | O(1) |
| Linked list search / delete by position | O(1) | O(n) | O(n) | O(1) |
| Set membership | O(log m) | O(log m) | O(log m) | – |
| Set union / intersection / difference | O(m) | O(m log m) | O(m log m) | O(m) |
| Merge sort | O(n log n) | O(n log n) | O(n log n) | O(n) |
| Resolve one role (DFS over ancestors, set unions) | O(P) | O(A·P log P) | O(R·P log P) | O(R) |
| Effective permissions of one user (with inheritance) | O(P) | O(r·A·P log P) | O(R²·P log P) | O(P) |
| SoD check (all rules, one user) | O(S) | O(S·m log P) | O(S·P log P) | O(P) |
| Compliance score (all users) | O(U) | O(U + V) | O(U + V) | O(U) |
| Cycle check / topological sort / hierarchy render | O(R) | O(R + E) | O(R + E) | O(R) |
| Detect violations (all users) | O(U) | O(U·r·P log P) | O(U·R·P log P) | O(V) |
| Sort violations | O(V log V) | O(V log V) | O(V log V) | O(V) |
| Candidate generation (one user) | O(P) | O(R·P log P) | O(R·P log P) | O(k) |
| Validate one candidate | O(P) | O(r·P log P) | O(R·P log P) | O(P) |
| **Backtracking (one user)** | O(k) (pruning cuts almost everything) | between | **O(2^k · validation)** | O(k) recursion depth |
| Whole plan (U users) | – | – | **O(Σ 2^{k_u})** | – |
| Simulate + final validate | O(U·P) | O(U·r·P log P) | O(U·R·P log P) | O(U·P) |

(The lookup of a role by name is a linear search, O(R) per held role; included in the R factors.
A = number of ancestors of a role, S = number of SoD rules, m = permissions per rule, V = violations.)

### 8.2 How performance scales
* **More users (U):** linear. Users are independent, so the search cost is a *sum* over violating users,
  not a product. This is why the engine searches users separately.
* **Deeper role hierarchies:** each role resolution walks its ancestors (O(R + E)); still polynomial, and independent of k.
* **More SoD rules (S):** one more set intersection per rule per user; also adds one pruning target per *broken* rule.
* **More roles (R) / permissions (P):** polynomial for detection and validation (set operations),
  but they also grow **k**, the number of candidate actions, which is what matters most.
* **More violations / role changes → bigger k:** the search space doubles with every extra candidate
  action (2^k). Hence the cap of 18 candidates per user (≈ 262 144 subsets).
* **Pruning:** best case O(k) nodes. Measured on one user with 14 forbidden direct permissions:

| mode | recursive calls | combinations evaluated | valid found |
|------|-----------------|------------------------|-------------|
| brute force | 32 767 | 16 384 | 1 |
| backtracking + pruning | 29 | 1 | 1 |

Same optimal cost (14) in both; see `tests/results/unit_tests.txt`.
