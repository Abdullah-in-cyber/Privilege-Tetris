// Scenarios.h
// The predefined university scenarios (1-6 from the proposal, 7 = separation of duties, 8 = role hierarchy) plus a reusable "pipeline" that runs
// detection -> repair search -> simulation -> validation in one call.
#pragma once

#include <string>
#include <vector>

#include "CostModel.h"
#include "RepairEngine.h"
#include "SecurityPolicy.h"
#include "ValidationEngine.h"

struct Scenario {
    int id;
    std::string title;
    std::string description;
    std::string expectedBehavior;
    int expectedViolations;  // user-level violations expected
    int expectedCost;        // minimum repair cost, or -1 when no valid repair exists
    SecurityPolicy policy;
};

std::vector<Scenario> buildScenarios();

struct PipelineResult {
    int userViolations = 0;
    int roleIssues = 0;
    RepairPlan plan;
    bool simulated = false;           // true when plan was solvable and a copy was repaired
    SecurityPolicy repaired;          // the simulated copy (valid only if `simulated`)
    ValidationReport validation;      // valid only if `simulated`
    double scoreBefore = 100.0;       // compliance score of the original configuration
    double scoreAfter = -1.0;         // compliance score after the simulated repair (-1 = no repair)
};

PipelineResult runPipeline(const SecurityPolicy& policy, const CostModel& cost, bool pruning);
