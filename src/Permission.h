// Permission.h
// One named right in the system, e.g. "grades.edit".
// riskWeight (1-10) says how sensitive the permission is. It scales the cost of
// revoking or granting it: touching a high-risk permission is a bigger change.
#pragma once

#include <string>

class Permission {
public:
    static constexpr int kMinRisk = 1;
    static constexpr int kMaxRisk = 10;

    Permission(std::string id, std::string description, int riskWeight = 1)
        : id_(std::move(id)), description_(std::move(description)),
          riskWeight_(riskWeight < kMinRisk ? kMinRisk : (riskWeight > kMaxRisk ? kMaxRisk : riskWeight)) {}

    const std::string& id() const { return id_; }
    const std::string& description() const { return description_; }
    int riskWeight() const { return riskWeight_; }

private:
    std::string id_;
    std::string description_;
    int riskWeight_;
};
