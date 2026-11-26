// RepairEngine.h
// Generates candidate repair actions and finds the MINIMUM-COST valid
// combination with BACKTRACKING (see RepairEngine.cpp for the algorithm).
//
// Why per user? Every action touches exactly one user and every validation rule
// is evaluated per user, so users never influence each other. Solving each user
// separately and adding the costs therefore gives the exact global optimum,
// while keeping the search space small (2^k per user instead of 2^(sum k)).
#pragma once

#include <map>
#include <string>
#include <vector>

#include "CostModel.h"
#include "RepairAction.h"
#include "SecurityPolicy.h"

struct RepairCandidate {
    std::vector<RepairAction> actions;
    int cost = 0;
};

struct SearchStats {
    std::size_t generated = 0;      // candidate repair actions generated
    unsigned long long searchSpace = 0;  // 2^generated possible combinations
    std::size_t nodesVisited = 0;   // recursive calls made
    std::size_t explored = 0;       // complete combinations evaluated
    std::size_t rejected = 0;       // evaluated but invalid
    std::size_t valid = 0;          // evaluated and valid
    std::size_t pruned = 0;         // branches cut off without full evaluation

    void add(const SearchStats& o) {
        generated += o.generated; searchSpace += o.searchSpace;
        nodesVisited += o.nodesVisited; explored += o.explored;
        rejected += o.rejected; valid += o.valid; pruned += o.pruned;
    }
};

struct UserRepairResult {
    std::string userName;
    std::vector<RepairAction> candidates;       // all generated actions, sorted by cost
    SearchStats stats;
    bool solvable = false;
    bool searchTooLarge = false;
    RepairCandidate best;                       // valid only if solvable
    std::vector<RepairCandidate> alternatives;  // valid solutions seen, sorted by cost
    std::map<std::string, std::size_t> rejectionReasons;  // label -> how many candidates
    std::string explanation;                    // why unsolvable (empty when solvable)
};

struct RepairPlan {
    std::vector<UserRepairResult> perUser;  // only users that have violations
    bool solvable = true;                   // every violating user can be repaired
    int totalCost = 0;
    SearchStats totals;

    std::vector<RepairAction> allActions() const;  // chosen actions of all solvable users
};

class RepairEngine {
public:
    static constexpr std::size_t kMaxCandidatesPerUser = 18;  // guards against 2^n explosion

    explicit RepairEngine(CostModel cost = CostModel(), bool pruning = true)
        : cost_(cost), pruning_(pruning) {}

    void setPruning(bool on) { pruning_ = on; }
    bool pruning() const { return pruning_; }
    const CostModel& costModel() const { return cost_; }

    // Module 3: candidate actions for one user, sorted by cost (merge sort).
    std::vector<RepairAction> generateCandidates(const SecurityPolicy& policy, const User& user) const;

    // Module 4: backtracking search for one user / for the whole policy.
    UserRepairResult solveUser(const SecurityPolicy& policy, const User& user) const;
    RepairPlan findMinimumRepair(const SecurityPolicy& policy) const;

private:
    CostModel cost_;
    bool pruning_;
};
