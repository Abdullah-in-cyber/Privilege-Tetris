// CostModel.h
// Defines what each kind of repair action costs.
//   Weighted (default): base weights revoke = 1, grant = 1, remove role = 2, add role = 2.
//                       REVOKE and GRANT are multiplied by the RISK WEIGHT (1-10) of the
//                       permission they touch, so changing a sensitive permission costs more.
//   Uniform           : every action costs 1  ("Total cost = number of changes"); risk is ignored
// Weights live ONLY here, so changing the cost function never touches the
// search algorithm.
#pragma once

#include <string>

#include "RepairAction.h"

class CostModel {
public:
    CostModel() : weights_{1, 1, 2, 2}, uniform_(false) {}

    // `permissionRisk` only matters for REVOKE_DIRECT / GRANT_DIRECT.
    int costOf(RepairType t, int permissionRisk = 1) const {
        if (uniform_) return 1;
        int base = weights_[static_cast<int>(t)];
        bool touchesPermission = (t == RepairType::REVOKE_DIRECT || t == RepairType::GRANT_DIRECT);
        return touchesPermission ? base * (permissionRisk < 1 ? 1 : permissionRisk) : base;
    }
    void setWeight(RepairType t, int w) { weights_[static_cast<int>(t)] = w < 1 ? 1 : w; }
    void setUniform(bool u) { uniform_ = u; }
    bool isUniform() const { return uniform_; }

    std::string describe() const {
        if (uniform_) return "Uniform (every change costs 1)";
        return "Weighted (revoke=" + std::to_string(weights_[0]) + " x risk, grant=" +
               std::to_string(weights_[1]) + " x risk, remove-role=" + std::to_string(weights_[2]) +
               ", add-role=" + std::to_string(weights_[3]) + ")";
    }

private:
    int weights_[4];
    bool uniform_;
};
