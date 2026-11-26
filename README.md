# Privilege Tetris
### An Intelligent Minimum-Change Cybersecurity Repair Engine (C++17)

Privilege Tetris audits a Role-Based Access Control (RBAC) system for a university,
detects permission violations, and uses **backtracking** to find the **cheapest repair that is
still safe**. Repairs are first simulated on a copy, then validated, and only applied to the live
configuration after confirmation.

Beyond the proposal's core requirements it also offers **risk-weighted costs**, **separation-of-duties
rules**, a **compliance score**, a **role hierarchy** (inheritance with cycle detection) and a
**CI pipeline** (see §8).

Pure C++17, standard library only. Console application. No databases, GUIs or external services.

---

## 1. Build and run

Requirements: `g++` (C++17) and `make`. Tested with GCC 14 on Linux.

```bash
make              # builds ./privilege_tetris
./privilege_tetris            # interactive console menu
./privilege_tetris --web      # browser UI at http://localhost:8080  (see section 3a)

make test         # builds and runs the automated test suite (285 checks)
sh tests/web_smoke.sh   # starts the real web server and checks it over HTTP (needs curl, python3)
make asan         # same tests + program under AddressSanitizer/UBSan (leak + UB check; takes a few minutes)
make clean
```

Non-interactive switches (handy for demos and for the results in `tests/results/`):

```bash
./privilege_tetris --scenarios      # table: all 8 scenarios, actual vs expected, compliance score before->after
./privilege_tetris --run 4          # full report for scenario 4
./privilege_tetris --export data    # (re)write data/scenarioN.txt
./privilege_tetris --web 9000       # web UI on another port
```

Windows / Code::Blocks: the console program has no platform-specific calls: add every file in `src/` except `WebServer.cpp`
to one project (C++17 on), or use MinGW with the same `g++` command the Makefile uses. The web server uses POSIX sockets, so on
Windows `--web` prints "not available" (the file compiles to a stub there); use WSL to get the web UI.

## 2. Folder structure

```
Makefile
README.md
.github/workflows/ci.yml   continuous-integration pipeline
src/        all source code (.h/.cpp); WebApi / WebServer / WebPage = the browser UI
data/       sample configuration files (scenario1.txt ... scenario8.txt)
docs/       ALGORITHMS.md  VIVA.md  TEST_RESULTS.md
tests/      test_main.cpp (automated tests), inputs/ (scripted menu sessions), results/ (real captured output)
```

## 3. Menu

| # | Option | What it does |
|---|--------|--------------|
| 1-3 | View configuration / users & roles / permission assignments | Read-only views (risk weights, SoD rules, role hierarchy tree) |
| 4 | Run Security Audit | Privilege matrix built with **linear search** (shows comparison counts) + violation detection + compliance score |
| 5 | Detect Permission Violations | Set-operation detection, severity-sorted linked list, compliance score |
| 6 | Generate Repair Candidates | All candidate actions per user, sorted by cost |
| 7 | Find Minimum-Cost Repair | Backtracking; statistics, best solution, alternatives, why it wins |
| 8 | Simulate Repair | Applies the plan to a **copy**; live data untouched |
| 9 | Validate Repaired Configuration | 7 checks + remaining-violation count |
| 10 | View Audit Logs | View / filter / keyword-search (linear) / delete entries of the linked list |
| 11 | Generate Security Report | Full report incl. compliance score before/after; optional save to a text file |
| 12 | Run Predefined Test Scenarios | Run any of the 8, or all with an expected-vs-actual table |
| 13 | Apply Validated Repair | Asks for confirmation; refuses if validation failed |
| 14 | Edit Configuration | Add permission (with risk), role (with parents), user; give/remove roles and direct permissions; delete user; **make a role inherit another (cycle-checked)**; **add a separation-of-duties rule**; clear all |
| 15 | Load / Save Configuration File | Text format below |
| 16 | Settings | Weighted ↔ uniform cost, custom base weights, pruning on/off (brute-force mode) |
| 17 | Linear Search Demo | Search users / roles / permissions / violations and see the comparisons |

Options that need earlier results compute them automatically (e.g. option 8 runs 5 and 7 first).
All input is validated; bad input prints a message and re-prompts. End-of-input exits cleanly.

## 3a. Web UI (`./privilege_tetris --web`)

Open `http://localhost:8080`. Everything the console menu does is available in the browser, and you can build **your own scenarios**:

* **Build your configuration** (left): tabs for Permissions (with risk 1-10), Roles (grants / allowed / forbidden / required sets as click-to-toggle
  chips, plus inheritance with cycle protection), Users (roles + direct permissions), Separation-of-duties rules, and a **Text / import** tab
  (paste or edit the same text format as `data/*.txt`). Open / Save a file, start empty, or pick one of the 8 predefined scenarios. **Undo** reverts any change.
* **Live analysis** (right): every edit re-runs detection and the repair search instantly: compliance score before/after, violations table,
  the candidate actions, the minimum-cost repair per user with its search statistics, the validation checks, and a "what changes" diff of each user's effective permissions.
* **Apply repair** changes your configuration only after validation passed and you confirm. Cost weights, uniform mode and pruning on/off are on the page.

How it is built: `WebServer` is a small single-threaded HTTP server on **127.0.0.1 only** (POSIX sockets, no libraries); it rejects foreign `Host`
and cross-origin requests and caps request size. `WebApi` maps requests to the same engine classes the console uses and answers with JSON. The API is
stateless: the browser keeps the configuration and sends it as text with each request, so nothing is stored on the server. `WebPage.cpp` embeds the
single HTML/JS page, so the binary needs no extra files and no internet connection.

## 4. Configuration file format

```
# comment
permission grades.edit risk=3 | Edit student grades      # risk=1..10 (default 1)
sod SOD-ADMIN-AUDIT accounts.manage audit.export         # a user may hold at most ONE of these
role Student
  grants    grades.view courses.enroll      # what holders actually receive
  allowed   grades.view courses.enroll      # the most they may ever have
  forbidden grades.edit                     # must never have
  required  grades.view                     # must always have
role HeadTeacher
  inherits  Teacher                         # parent roles must be declared first
  grants    logs.read
user alice
  roles  Student
  direct grades.edit                        # individual override on top of roles
```

## 5. The model in one paragraph

A **user** has any number of **roles** plus optional **direct permissions**.
A role may **inherit** other roles (a DAG): it receives the union of its own and all ancestors'
grants, allowed, forbidden and required sets. *Effective permissions* = union of all (inherited) role grants ∪ direct permissions.
A user's rules are the union of `allowed`, `forbidden` and `required` over their roles. Violations (set operations):

| Violation | Formula |
|-----------|---------|
| Forbidden | `effective ∩ forbidden` |
| Unauthorized (excessive) | `(effective \ allowed) \ forbidden` |
| Missing required | `required \ effective` |
| Separation of duties (SoD) | `|effective ∩ rule.permissions| > 1` |
| Role-policy inconsistency | e.g. a role that requires **and** forbids the same permission |

## 6. Repair actions and cost

| Action | Cost | Meaning |
|--------|------|---------|
| REVOKE | 1 × risk of the permission | remove a direct permission |
| GRANT | 1 × risk of the permission | give a direct permission |
| REMOVE-ROLE | 2 | take a role away |
| ADD-ROLE | 2 | give an additional role |

Every permission has a **risk weight 1-10** (default 1), so changing `accounts.manage` (risk 5) costs more
than changing `attendance.mark` (risk 1). Total repair cost = sum of the action costs.
Weights live only in `CostModel`; option 16 switches to **uniform** cost (every change = 1, risk ignored,
i.e. "number of changes") or changes the base weights.

A candidate repair is **valid** only if, for the affected user: no forbidden permission remains, every
required permission is present, nothing outside the allowed set is held, no SoD rule is broken, no new
violation appears, all legitimate access is kept, and nothing is gained except permissions that were required.
Details in `docs/ALGORITHMS.md`.

**Compliance score:** every user starts at 100 and loses 25 / 15 / 8 / 3 points per CRITICAL / HIGH / MEDIUM / LOW
violation (floor 0); the system score is the average. Ratings: 100 SECURE, ≥90 LOW, ≥70 MODERATE, ≥50 HIGH, else CRITICAL RISK.

## 7. Predefined scenarios (default weighted costs)

| # | Scenario | Expected | Actual (verified) | Score before → after |
|---|----------|----------|-------------------|----------------------|
| 1 | Student with grade-edit | revoke `grades.edit` (1 × risk 3) | cost 3 | 87.5 → 100 |
| 2 | Teacher with `accounts.manage` | revoke only that (1 × risk 5); teaching access kept | cost 5 | 91.7 → 100 |
| 3 | Assistant missing required `attendance.mark` | grant it (1) rather than add a role (2) | cost 1 | 96.0 → 100 |
| 4 | Several violations, two users | explore combinations, minimum total | cost 12 (dave 9 + eve 3) | 58.5 → 100 |
| 5 | Role both requires and forbids `audit.export` | detect conflict, **no** repair, explain | no repair, explained | 87.5 → n/a |
| 6 | Already secure | 0 violations, cost 0 | 0 / 0 | 100 → 100 |
| 7 *(extension)* | Admin + ComplianceOfficer break a separation-of-duties rule | remove ComplianceOfficer, keep admin access | cost 2 | 87.5 → 100 |
| 8 *(extension)* | HeadTeacher inherits Teacher; `accounts.manage` forbidden only via the inherited rule | revoke it (1 × risk 5) | cost 5 | 87.5 → 100 |

Scenarios 1-6 are the ones required by the proposal; 7 and 8 demonstrate the extensions.

## 8. Enterprise extensions

| Extension | What it adds | Where |
|-----------|-------------|-------|
| Risk weights | Revoke/grant cost scales with the permission's sensitivity | `Permission.h`, `CostModel.h` |
| Separation of duties | Rules like "never both `accounts.manage` and `audit.export`"; detection, repair search, pruning bound and validation all understand them | `SodRule.h`, `ViolationDetector.cpp`, `RepairEngine.cpp`, `ValidationEngine.cpp` |
| Compliance score | 0-100 score per user and per system, before and after repair | `ComplianceScorer.*` |
| Role hierarchy | Inheritance DAG; iterative DFS resolution, DFS cycle check that prints the offending path, Kahn's topological sort, tree rendering | `RoleHierarchy.*`, `SecurityPolicy::resolvedRole` |
| CI pipeline | GitHub Actions: build + tests with g++ and clang++, sanitizer job, scripted menu sessions, all scenarios must match, scenario reports uploaded as an artifact | `.github/workflows/ci.yml` |

The CI file is syntactically valid, and every command it runs was executed locally with g++ — but it has
not been executed on GitHub, and the `clang++` leg has not been run (clang is not installed on the development machine).

## 9. Where each required DSA concept lives

| Concept | Files | Demonstrated by |
|---------|-------|-----------------|
| OOP | all classes: `User`, `Role`, `Permission`, `SodRule`, `SecurityPolicy`, `Violation`, `RepairAction`, `CostModel`, `RepairEngine`, `ValidationEngine`, `ViolationDetector`, `ComplianceScorer`, `RoleHierarchy`, `AuditLogger`, `ReportGenerator`, `Application` | encapsulated state, value semantics (copy = snapshot) |
| Linked list | `LinkedList.h` (custom doubly linked list), used by `Violation.h` (`ViolationList`) and `AuditLogger` | menu 5 (violations), menu 10 (insert, traverse, keyword search, delete) |
| Linear search | `LinearSearch.h`, `SecurityPolicy.cpp` lookups, `LinkedList.h::find`, `AuditLogger::search` | menu 4 and 17 print comparison counts |
| Set operations | `PermissionSet.h/.cpp` | union, intersection, difference, membership, subset — used by detection, generation, validation and SoD |
| Brute force / backtracking | `RepairEngine.cpp` (`Backtracker`) | menu 7; menu 16 → pruning off = brute force |
| Sorting | `Sorting.h` (merge sort) | violations by severity, candidates by cost, alternatives by cost |
| Graph algorithms (extension) | `RoleHierarchy.cpp`, `SecurityPolicy.cpp` | DFS, cycle detection, topological sort |
| Complexity analysis | `docs/ALGORITHMS.md` §8 | measured node counts in `docs/TEST_RESULTS.md` |

## 10. Team split (suggested, for the viva)

| Member | Module | Files |
|--------|--------|-------|
| 1 | Data structures | `LinkedList.h`, `LinearSearch.h`, `Sorting.h`, `PermissionSet.*`, `AuditLogger.*`, `RoleHierarchy.*` |
| 2 | Model + detection | `User`, `Role`, `Permission`, `SodRule`, `SecurityPolicy.*`, `Violation.h`, `ViolationDetector.*`, `ComplianceScorer.*` |
| 3 | Repair engine | `RepairAction.*`, `CostModel.h`, `RepairEngine.*`, `ValidationEngine.*` |
| 4 | Interface, reporting, quality | `Menu.*`, `Input.*`, `ReportGenerator.*`, `ConfigIO.*`, `Scenarios.*`, `main.cpp`, `tests/`, CI |

## 11. Design decisions and honest limitations

* **Per-user search.** Every action touches one user and every validation rule is per user (SoD rules and
  role hierarchy are evaluated per user too), so users cannot affect each other. Searching each user separately
  and adding the costs is therefore *exactly* as optimal as one global search, but needs 2^k per user instead of 2^(sum of k).
* **Defined search space.** "Optimal" means optimal within the generated candidates: revoking direct
  permissions, removing roles that grant a bad permission (including through inheritance), granting missing/lost permissions
  directly, and adding roles that provide them. **Role definitions and inheritance links are never edited** by a repair, because that
  would change every holder of the role. A role-level inconsistency therefore remains listed as an advisory after a repair; advisories
  are not part of the compliance score. A parent's inconsistency is also reported for each child role (resolved sets are checked).
* **Candidate cap.** More than 18 candidate actions for one user is refused with a clear message (2^18
  combinations); the search never hangs.
* **Multiple roles = union of rules.** A user's allowed/forbidden/required sets are the unions over their
  roles. A `forbidden` rule from any role therefore wins over a `grants` from another.
* **Removing a role removes its requirements.** If a user holds two roles that each require something, dropping one role is a legal
  repair as long as legitimate access is preserved (a test covers this).
* **Separation of duties = "at most one".** A rule keeps a *set* of permissions apart; the user may keep any one of them.
  The engine decides which, by cost. At least one must survive, so a repair cannot "fix" SoD by removing all access.
* **Least privilege.** A repair may only *add* permissions the user was originally required to have;
  otherwise "swap to Admin" would count as a repair.
* **Alternatives list.** With pruning on, only alternatives encountered before pruning are listed; the
  optimal cost is still exact (checked against an independent oracle, including on 494 random users).
* **Memory checking.** Valgrind is not installed on the development machine, so leaks were checked with
  AddressSanitizer + LeakSanitizer (`make asan`): no errors reported.
* **Web UI.** The browser page is plain JavaScript with no libraries. The server side is tested (`make test`, `tests/web_smoke.sh`), but the JavaScript
  has **not** been exercised by an automated browser test (no browser or Node is installed on the development machine); click through the page once before a demo.
* **No external test framework.** The tests are a self-contained assert harness, in line with the "no unnecessary dependencies"
  rule; it returns a non-zero exit code on failure, which is what CI needs.
