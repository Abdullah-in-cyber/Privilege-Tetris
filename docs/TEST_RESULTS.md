# Test Results

All results below were produced by actually running the program (GCC 14.2, `-std=c++17 -Wall -Wextra -Wpedantic`,
zero compiler warnings). Raw output is saved in `tests/results/`. Reproduce with `make test`,
`make asan`, `./privilege_tetris --scenarios`, or `./privilege_tetris < tests/inputs/<file>.txt`.

## 1. Summary

| Check | Result |
|-------|--------|
| Full project compiles, 0 warnings | PASS |
| Automated suite (`make test`) | **285 passed, 0 failed** (`tests/results/unit_tests.txt`), includes 44 checks of the web API |
| Web server over real HTTP (`sh tests/web_smoke.sh`) | all checks pass (`tests/results/web_smoke.txt`); also passes against the ASan/UBSan build |
| Web page JavaScript | **not machine-tested** (no browser/Node here); server side only |
| Same suite under AddressSanitizer + LeakSanitizer + UBSan (`make asan`) | 241 passed, no sanitizer output (`tests/results/unit_tests_asan.txt`) |
| 5 scripted menu sessions under the sanitizer build | exit 0, no sanitizer output (no leaks, no memory errors) |
| 8 predefined scenarios, actual vs expected | **8/8 match** |
| Engine optimum vs independent oracle: 8 scenarios + 494 random users | **0 mismatches** |
| CI workflow (`.github/workflows/ci.yml`) | YAML parses (2 jobs); every command in it was run locally with g++. **Not executed on GitHub; the clang++ leg was not run (clang not installed here).** |

Memory: Valgrind is not installed on this machine, so leak checking used LeakSanitizer (part of `-fsanitize=address`).

## 2. Predefined scenarios (`tests/results/scenario_summary.txt`)

| # | Scenario | Violations (actual / expected) | Minimum cost (actual / expected) | Compliance score | Result |
|---|----------|------|------|-----|------|
| 1 | Student excessive privileges | 1 / 1 | 3 / 3 | 87.5 → 100.0 | PASS |
| 2 | Teacher administrative privileges | 1 / 1 | 5 / 5 | 91.7 → 100.0 | PASS |
| 3 | Missing required permission | 1 / 1 | 1 / 1 | 96.0 → 100.0 | PASS |
| 4 | Multiple violations | 4 / 4 | 12 / 12 | 58.5 → 100.0 | PASS |
| 5 | Impossible repair | 1 / 1 | no repair / no repair | 87.5 → n/a | PASS |
| 6 | Already secure | 0 / 0 | 0 / 0 | 100.0 → 100.0 | PASS |
| 7 | Separation of duties (extension) | 1 / 1 | 2 / 2 | 87.5 → 100.0 | PASS |
| 8 | Role hierarchy (extension) | 1 / 1 | 5 / 5 | 87.5 → 100.0 | PASS |

(Role-level advisories such as "Assistant requires but does not grant attendance.mark" are reported
separately; they are neither counted as user violations nor scored.)

Costs use the default weights: revoke/grant = 1 × the permission's risk weight (grades.edit 3, accounts.manage 5,
attendance.mark 1), role changes = 2.

### Search statistics per scenario (backtracking with pruning)

| # | Candidates | Search space (2^n) | Nodes visited | Explored | Rejected | Valid | Pruned |
|---|-----------|--------------------|---------------|----------|----------|-------|--------|
| 1 | 1 | 2 | 3 | 1 | 0 | 1 | 1 |
| 2 | 1 | 2 | 3 | 1 | 0 | 1 | 1 |
| 3 | 2 | 4 | 5 | 2 | 0 | 2 | 1 |
| 4 | 5 (dave 4, eve 1) | 18 | 26 | 9 | 6 | 3 | 5 |
| 5 | 3 | 8 | 15 | 6 | 6 | 0 | 2 |
| 6 | 0 | – | – | – | – | – | – |
| 7 | 8 | 256 | 233 | 93 | 87 | 6 | 24 |
| 8 | 1 | 2 | 3 | 1 | 0 | 1 | 1 |

### Scenario details

* **S1** – alice: `{courses.enroll, grades.edit, grades.view}` → `{courses.enroll, grades.view}` by `REVOKE grades.edit` (1 × risk 3 = 3).
* **S2** – tom: only `accounts.manage` removed (1 × risk 5 = 5); `grades.edit`, `grades.view`, `attendance.mark` intact.
* **S3** – sam is missing `attendance.mark`. Best = `GRANT` (cost 1). Another valid solution, `GRANT + ADD-ROLE AttendanceClerk`, costs 3. Adding role Teacher was never offered because Teacher grants `grades.edit`, which Assistant forbids.
* **S4** – dave: revoke `accounts.manage` (5) + revoke `grades.edit` (3) + grant `attendance.mark` (1) = 9; eve: revoke `grades.edit` = 3; **total 12**. Cheapest other valid solution seen for dave: 11 (a cost-10 solution exists but pruning never evaluated it). Rejections: "forbidden permission still held" ×6, "permission outside allowed set" ×6. Scores: dave 42, eve 75 → 58.5 (HIGH RISK) → 100.
* **S5** – zara: all 6 evaluated combinations rejected (forbidden still held ×2, legitimate access lost ×1, new violation ×1, outside allowed set ×1, privilege escalation ×4; one combination can fail several checks); 2 more branches pruned. Report: "`audit.export` is both REQUIRED and FORBIDDEN … No safe repair exists" and final status `[NOT SECURE]`.
* **S6** – "No violations -> nothing to repair. Minimum repair cost: 0"; validation `[SECURE]`.
* **S7** – finn holds Admin + ComplianceOfficer: `[CRITICAL] SOD … holds {accounts.manage, audit.export} together`. 93 of 256 combinations evaluated, 87 rejected (reasons include "separation-of-duties rule still broken" ×38, "legitimate access lost" ×43, "privilege escalation" ×50). Best = `REMOVE-ROLE ComplianceOfficer` (2); finn keeps `accounts.manage`, `system.settings`, `logs.read`, `grades.view`. Next best seen: 3.
* **S8** – hierarchy tree printed (`Teacher` → `+- HeadTeacher`), topological order `Student Teacher Admin HeadTeacher`. hana's `accounts.manage` is flagged FORBIDDEN only because HeadTeacher **inherits** Teacher's forbidden set; hugo, who has the same role but no extra permission, is clean and receives Teacher's grants. Repair: revoke `accounts.manage` (5).

## 3. Optimality verification
1. **Scenario oracle** – for every violating user of every scenario, the test enumerates all 2^k subsets with a bit-mask,
   validates each independently of the backtracking code, and compares the cheapest valid cost with the engine's answer.
   All match (solvability and cost). For each scenario brute force (pruning off) and pruned backtracking return identical
   totals, and pruning never explores more combinations.
2. **Randomized oracle** – a seeded generator builds 400 random policies (random roles, hierarchy edges, SoD rules, risk weights,
   direct permissions). 354 contained violations; for the 494 users checked (252 solvable, 242 unsolvable, 110 with an SoD conflict) the pruned engine
   **and** the unpruned engine both equal the oracle (solvability and cost). 0 mismatches.

### Pruning vs brute force

Scenario 4 (`tests/results/bruteforce_vs_backtracking.txt`), both give total cost 12:

| mode | nodes | combinations evaluated | rejected | valid |
|------|-------|-------------------------|----------|-------|
| backtracking + pruning | 26 | 9 | 6 | 3 |
| brute force | 34 | 18 | 14 | 4 |

Stress case (1 user, 14 forbidden direct permissions, in `unit_tests.txt`), both give cost 14:

| mode | nodes | evaluated | valid |
|------|-------|-----------|-------|
| brute force | 32 767 | 16 384 | 1 |
| backtracking + pruning | 29 | 1 | 1 |

## 4. Requirement checks

| Requirement | Test | Result |
|-------------|------|--------|
| Required permissions stay intact | every solvable scenario: required ⊆ effective for every user after repair | PASS |
| Original not modified by simulation | `tom` still has `accounts.manage` in the original after simulation | PASS |
| Unsafe repair is caught | removing role Teacher from tom is rejected by validation | PASS |
| Weighted cost works | GRANT=5, ADD-ROLE=1 → engine chooses ADD-ROLE (cost 1); default → GRANT | PASS |
| Risk weights | revoke grades.edit = 3, accounts.manage = 5, attendance.mark grant = 1; uniform mode ignores risk; risk clamps to 1..10; risk alone changes which combination wins (11 vs 2 in uniform mode) | PASS |
| Uniform cost = number of changes | scenario 4 → 4 | PASS |
| SoD | set logic; detection; scenario 7 repair; validation rejects "no change" and "remove both sides"; two-role case solvable (cost 2); one role requiring both sides → no repair + explanation | PASS |
| Role hierarchy | diamond resolves each ancestor once; inherited forbidden rules; ancestors, depth, topological order; cycle `Base -> Top -> … -> Base` rejected with its path; self, unknown, duplicate links rejected and leave the graph unchanged; forced cycle terminates resolution/ordering/rendering | PASS |
| Compliance score | secure = 100 (SECURE); S1 = 87.5; S4 = 58.5 HIGH RISK; floor at 0; all rating thresholds; empty configuration = 100; 100 after every validated repair | PASS |
| Too-large search is refused | user with 25 candidates → "search space too large", no hang | PASS |
| Empty configuration | 0 violations, cost 0, all menu views print "(no …)" (`tests/results/edit_and_empty_run.txt`) | PASS |
| User with no roles + direct permission | 1 violation, revoke it, cost 1 | PASS |

## 5. Data structure tests (all PASS)
Linked list: pushFront/pushBack order, insertAt front/middle/back, out-of-range rejection, linear
search comparison count (hit after 5; miss examines all nodes), removeAt head/tail/out of range, deep copy
independence, `removeIf`, `clear`, removing the only node.
Sets: union, intersection, both differences, membership, subset, empty sets, printing.
Merge sort: sorted result, **stability**, empty vector, 1000 elements. Linear search: hit (3 comparisons), miss (4).
Audit logger: case-insensitive search scans all nodes; category deletion removes the right nodes.

## 6. Configuration files
Every scenario (including 7 and 8) round-trips (save → parse → same violations and cost). Risk weights, SoD rules and inheritance
survive a round trip, even when a child role is stored before its parent in memory (the writer uses the topological order).
Rejected with a line-numbered message and **no change to the current configuration**: unknown permission, unknown role, duplicate permission,
unexpected keyword, invalid id, risk 0 / 11 / non-numeric, extra words on a permission line, SoD rule with one permission / duplicate permission / unknown permission /
duplicate name, `inherits` of an undeclared parent. An empty file loads as an empty configuration; a missing file reports "cannot open file".

## 7. Invalid input and robustness (scripted sessions in `tests/inputs/`)

| Script | Input | Observed |
|--------|-------|----------|
| `invalid_input.txt` | letters, negative, out-of-range, blank, decimal and multi-token menu choices; names with spaces/symbols; duplicate permission; unknown user; unwritable save path; missing load file; blank file name; out-of-range settings values | every case prints a `[!]` message and re-prompts; exit 0 |
| `edit_and_empty.txt` | unknown permission in new role, unknown role in new user, direct permission for non-existent user, clear all, then every view/audit/search/simulate/validate/apply/report option on the empty configuration | "Unknown permission … Role not created", "Unknown role … User not created", "No such user", "Nothing to audit", "No violations", "Nothing to apply"; exit 0 |
| `full_flow.txt` | simulate → apply (y) → re-detect → view audit log | apply works; log lists SIMULATED then APPLIED entries; re-detect shows only the role-definition advisory; exit 0 |
| `new_features.txt` | risk prompt with `abc` and `99` then 7; new role with unknown parent (rejected) then with parent Teacher; inheritance `Teacher ← Dean` (rejected: "would create an inheritance cycle: Teacher -> Dean -> Teacher"), `Dean ← Dean` ("cannot inherit from itself"), `Admin ← Student` (accepted); SoD rule with unknown permission, with one permission, valid, duplicate name; view roles (hierarchy tree); save and reload the file; detect | every bad input rejected with a message; valid ones accepted; reload gives "Loaded 2 user(s), 6 role(s)" and the same score 58.5; exit 0 |
| `bruteforce_compare.txt` | toggle pruning and run the search twice | both modes print the same minimum cost (12) |
| End of input in the middle of a prompt (`printf '14\n3\nuser1\n' \| ./privilege_tetris`) | stdin closes | "Input closed. Goodbye.", exit 0 |

Observed behaviour worth knowing: after a repair is applied, option 5 still lists the role-definition
advisory ("role Assistant requires but does not grant attendance.mark") because repairs change users, not role
definitions (see README §11). Validation of the repaired *users* passes.

## 8. Continuous integration
`.github/workflows/ci.yml` defines two jobs: (1) build + `make test` + `--scenarios` + all scripted sessions with g++ and clang++, then archive
the scenario reports as an artifact; (2) `make asan` and the scripted sessions under the sanitizers. I validated the YAML and ran the same commands locally
with g++; **the pipeline itself has not run on GitHub** and clang++ is not installed on the development machine, so the clang leg is untested.
`make test` exits non-zero on any failed check and `--scenarios` exits non-zero on any mismatch, which is what makes them usable as CI gates.

## 9. Known limitations (not hidden)
* Optimal only within the defined search space and up to 18 candidate actions per user.
* Role definitions and inheritance links are never repaired automatically.
* With pruning on, the list of alternative solutions is incomplete by design (pruned branches are not listed).
* A parent role's inconsistency is also reported for each of its children.
* The sanitizer test run is slow (a few minutes) because of the randomized oracle test.
* Tests are console/assert based; there is no external test framework.
