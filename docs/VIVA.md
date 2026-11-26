# Viva / Interview Questions and Answers

## A. Project and design

**1. What problem does Privilege Tetris solve?**
Over time users accumulate permissions they should not have (or lose ones they need). Stripping everything
forbidden can break legitimate work. The tool detects violations and finds the *cheapest* set of changes
that fixes them while preserving required access, and proves the result by re-validating a simulated copy.

**2. Why the name "Tetris"?** Pieces (permissions/roles) must be fitted so every row (user) is complete (required) with no
overflow (forbidden/excess) — and you want the fewest moves.

**3. Why not just revoke every forbidden permission (greedy)?** A permission can come from a role.
Revoking it directly is impossible; removing the role may destroy legitimate access, forcing re-grants that policy may not allow.
Choosing among such interacting options needs search, not a one-pass rule. Scenario 5 shows a case where *no* choice works.

**4. How is the project structured (OOP)?** One class per concept (`User`, `Role`, `Permission`, `SecurityPolicy`,
`Violation`, `RepairAction`, `CostModel`, `RepairEngine`, `ValidationEngine`, `ViolationDetector`, `AuditLogger`,
`ReportGenerator`, `Application`). Each has one responsibility; data members are private; `ReportGenerator` only reads;
`CostModel` is the only place weights exist.

**5. Why is `SecurityPolicy` copyable?** Copying gives an independent snapshot — this is how repairs are simulated
without touching the real configuration.

## B. Linked list

**6. Why a custom list, and where is it used?** To demonstrate the structure. It holds detected violations and the audit history.
Violations are added at runtime in unknown number, appended in O(1).

**7. Singly or doubly linked? Why?** Doubly: each node knows `prev` and `next`, so deleting a node we are standing on needs no
second pointer scan. Costs one extra pointer per node.

**8. How do you avoid memory leaks?** The list owns its nodes; the destructor walks and deletes them (RAII). Copy is deep;
assignment is copy-and-swap. `removeIf` saves `next` before deleting. Verified with LeakSanitizer: no leaks.

**9. Complexity of insert / search / delete?** pushBack/pushFront O(1); search O(n); delete by position O(n) to reach it, O(1) to unlink.

**10. Where do you demonstrate deletion?** Menu 10 → "Delete all entries of a category" (`removeIf`), and `removeAt`.

## C. Linear search and sorting

**11. Where is linear search used and why not binary search?** Lookups of users/roles/permissions in unsorted vectors, the privilege audit
matrix, list traversals. Data is unsorted and small; binary search needs sorted random-access data and a linked list cannot be binary searched efficiently.
Menus 4 and 17 print the comparison count.

**12. Best/average/worst case of linear search?** Best O(1), average about n/2 → O(n), worst O(n).

**13. Which sort did you use and why?** Merge sort — O(n log n) in all cases, **stable**, predictable. Stability keeps equal-severity
violations in detection order, so reports are repeatable.

**14. What do you sort?** Violations by severity (descending), candidate actions by cost (ascending), alternative solutions by cost.

**15. Why sort the candidates by cost before searching?** Cheap actions first lets the first complete solutions be cheap,
so a good "best" is known early and pruning cuts more.

## D. Sets

**16. Which set operations, and for what?**
Forbidden `eff ∩ forbidden`; unauthorized `(eff \ allowed) \ forbidden`; missing `required \ eff`;
effective permissions = union of grants; subset tests in validation; membership everywhere.

**17. Why `std::set`?** Sorted, no duplicates, O(log n) membership, and the `std::set_union/intersection/difference` algorithms work on sorted ranges.

**18. How do you treat several roles?** Rules are unions over the roles. If any role forbids a permission it is forbidden for that user.

## E. Backtracking

**19. Explain the algorithm in two sentences.** Each candidate action is either included or not; the recursion tries both,
and at the end of a path applies the chosen actions to a copy, validates it, and keeps it if it is the cheapest valid one. When a partial path
cannot beat the best (or can no longer fix a violation) it is abandoned — that is the "back" in backtracking.

**20. Brute force vs backtracking here?** Brute force evaluates all 2^k subsets. Backtracking uses the same recursion but prunes.
Menu 16 toggles pruning. Measured: 14 forbidden permissions → 32 767 vs 29 recursive calls, same optimal cost.

**21. Is the result really optimal? Prove it.** Pruning discards only branches that provably cannot beat the best cost:
(a) cost already above best — costs are positive so extending never lowers it; (b) cost + the cheapest fix still *required* for an unfixed violation
is already above best (a lower bound); (c) no remaining action can fix a violation (infeasible). So every discarded branch has no better solution.
Also the tests check the engine against an independent bit-mask brute-force oracle for every user in every scenario.

**22. Optimal over what?** Over the defined search space: revoke direct permissions, remove roles that grant a bad permission, grant missing/lost
permissions directly, add roles that provide them. Role definitions are not edited.

**23. Why can users be solved separately?** Every action touches one user and every validation rule is per user, so users do not interact.
Summing per-user optima is the global optimum, and the search is Σ2^{k_u} instead of 2^{Σk_u}.

**24. Worst-case complexity?** O(2^k · V) per user where V is validation cost per candidate. Capped at k ≤ 18.

**25. What does a candidate "valid" mean?** Seven checks: no forbidden held, all required present, nothing outside allowed, no separation-of-duties rule broken,
no new violation, legitimate access kept, no privilege escalation.

**26. Why the "no escalation" rule?** Without it, "replace Auditor with Admin" would satisfy every other rule. A repair may only add permissions
the user was originally required to have.

**27. How does it detect an impossible repair?** Every combination is evaluated or pruned and none passes validation. The failure reasons are counted
and a conflict (e.g. required ∩ forbidden ≠ ∅) is named. Nothing is simulated or applied.

**28. Give a worked example.** See `docs/ALGORITHMS.md` §5: dave, 4 candidates, 16 subsets, 3 valid (costs 3, 4, 5), minimum 3.

## F. Cost, validation, safety

**29. How is cost calculated and how can weights change?** Sum of action costs: revoke and grant are 1 × the permission's risk weight (1-10), remove role 2, add role 2 (default). `CostModel` holds
the weights; uniform mode gives cost = number of changes and ignores risk. Changing weights changes the chosen repair without changing the search (tested).

**30. How do you make sure the original is not changed early?** Search works on copies of the user; simulation copies the whole policy;
the live configuration changes only in option 13, after validation passed and the user confirms.

**31. What does post-repair validation check?** Forbidden removed, required preserved, allowed satisfied, separation of duties satisfied, no new violations, legitimate access preserved,
no escalation, and zero remaining violations. The repair is marked successful only if all pass.

**32. Is validation checked against a bad repair?** Yes: a test removes the whole Teacher role from a teacher; validation rejects it (legitimate access lost).

## G. Complexity and scaling

**33. How does performance change with more users, roles, permissions?** Users: linear (independent). Roles/permissions: polynomial (set operations) for detection
and validation, but they increase k, the number of candidate actions, which doubles the search space per extra action.

**34. Detection complexity?** O(U · r · P log P): per user, build effective permissions and do a constant number of set operations.

**35. Why a cap of 18 candidates?** 2^18 = 262 144 subsets is still instant; 2^30 would not be. The program refuses larger cases with an explanation instead of hanging.

## H. Testing and honesty

**36. How did you test?** 285 automated checks (data structures, scenarios, optimality oracle plus a seeded random-policy oracle over 494 users, safety, cost model, risk, SoD, hierarchy, scoring, edge cases, file parsing),
eight predefined scenarios, five scripted menu sessions including invalid input, and everything under AddressSanitizer/LeakSanitizer/UBSan.

**37. What are the limitations?** Optimal only inside the defined candidate space; role definitions are not repaired; max 18 candidates per user;
alternatives list is incomplete when pruning is on. All are documented in README §10.

**38. How would you extend it?** Role-edit actions (change a role's grants, with impact analysis for all its holders); time-limited permissions with expiry (a priority queue);
attack-path search (BFS/DFS over user → role → permission graphs); undo/rollback with a stack; a stronger branch-and-bound to raise the 18-candidate cap; JSON/CSV import and export.

## I. Extensions (risk, SoD, score, hierarchy, CI)

**39. What are risk weights and why add them?** Each permission has a sensitivity 1-10. Revoking or granting it costs `base × risk`, so touching `accounts.manage` (5) is a bigger change than
`attendance.mark` (1). It makes the cost function reflect operational impact instead of just counting changes. Uniform mode switches it off. Role changes are not scaled.

**40. Does risk change the algorithm?** No. Only `CostModel` and the candidate generator (which asks the model for each action's cost) know about it. The search, pruning and validation are unchanged;
a test shows risk alone can change which combination wins (remove role 2 vs revoke risk-9 permission 9).

**41. What is separation of duties?** A rule that no user may hold more than one permission from a set, e.g. `accounts.manage` and `audit.export`, so one person cannot both create accounts and erase the evidence.
Detection is `|effective ∩ set| > 1`.

**42. How does the engine repair an SoD conflict?** Every held conflicting permission becomes a removal candidate; the backtracking search picks the cheapest valid combination. Validation requires the rule to be satisfied
*and* at least one of the conflicting permissions to survive, so the repair cannot "solve" SoD by removing all access. Scenario 7: remove role ComplianceOfficer (cost 2).

**43. Why does SoD need its own pruning target?** The lower bound says "every unresolved problem needs at least one fixing action". A broken SoD rule is one such problem; only a revoke of a held permission or a role change can fix it, never a grant.
Without it the bound would be unsound for SoD users; the random oracle test (110 users with SoD conflicts) confirms the optimum stays exact.

**44. How is the compliance score computed?** Each user starts at 100 and loses 25/15/8/3 for CRITICAL/HIGH/MEDIUM/LOW violations (floor 0); the system score is the average; the rating is derived from thresholds.
It is deliberately simple and explainable. Scenario 4: dave 42, eve 75 → 58.5 (HIGH RISK); after repair 100.

**45. Why aren't role advisories scored?** They describe role *definitions*, which the engine never edits. Scoring them would stop a successfully repaired system from reaching 100.

**46. What is the role hierarchy and which algorithms does it use?** A role can inherit other roles; the structure is a directed acyclic graph. Resolving a role merges the sets of all ancestors with an iterative depth-first search and a visited set.
Adding a link first runs a DFS to see if the child is already reachable from the parent (which would close a cycle) and prints the cycle path. Kahn's algorithm gives a topological order (parents first), which is also how the config file is written so it can be read back.

**47. Why must the graph be acyclic?** A cycle would make "inherits" circular and meaningless, and naive resolution would loop forever. The edge-adding function rejects cycles; the parser requires parents to be declared first; and resolution still terminates thanks to the visited set.

**48. What does inheritance change for violations?** The parent's grants *and rules* apply to the child. In scenario 8 `accounts.manage` is forbidden only because HeadTeacher inherits Teacher's forbidden set; without the link it would merely be "unauthorized" (a test checks both).

**49. Why can't a repair edit the hierarchy or roles?** Changing a role affects every holder, so it is a policy decision, not a per-user repair. The engine only touches users; role problems are reported as advisories.

**50. What does the CI pipeline do?** On every push it builds and runs the 285 tests with g++ and clang++, runs all scenarios (non-zero exit on any mismatch), replays the scripted menu sessions, builds under AddressSanitizer/UBSan, and archives the scenario reports.
Honest status: the YAML is valid and every command ran locally with g++, but it has not been run on GitHub and clang++ is untested here.

**51. Why no GoogleTest/Catch2?** The brief says no unnecessary dependencies. The self-contained assert harness is small, returns a failing exit code (what CI needs), and keeps the project buildable with just `g++` and `make`.

## J. Web UI

**52. How does the web UI work without any library?** `--web` starts a tiny HTTP server written with POSIX sockets (`socket/bind/listen/accept/recv/send`). It parses the request line, headers and body by hand and passes
(method, path, query, body) to `WebApi`, which calls the same engine as the console and returns JSON. One embedded HTML page with plain JavaScript draws everything.

**53. Is the web UI a second implementation of the algorithm?** No. The browser only edits data and displays results. Detection, the backtracking repair and the validation all run in the C++ engine; the web layer is a thin adapter (tests check that every scenario gives the same cost through the API as directly).

**54. Why is the API stateless?** The browser sends the whole configuration (as the text format) with each request, so the server keeps no sessions, no files and no shared state. It makes the server trivial to reason about and to test, and "Undo" is just a list of earlier configurations in the page.

**55. Is it safe to run a server?** It binds to 127.0.0.1 only, rejects requests whose `Host` is not localhost or whose `Origin` differs (DNS-rebinding / cross-site), caps the body at 1 MiB, times out stalled clients after 5 s, and the page builds its DOM with text nodes so names cannot inject HTML. It is a local teaching tool, not a hardened internet service.

**56. How can users create their own scenarios?** In the page: add permissions (with risk), roles (with inheritance), users and SoD rules by clicking chips, or paste the text format. The analysis updates live, and the scenario can be saved to a file and re-opened later (also by the console menu).

