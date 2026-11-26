// SodRule.h
// A SEPARATION-OF-DUTIES rule: no single user may hold more than one of the
// listed permissions. Example: "accounts.manage" and "audit.export" must not be
// combined, otherwise one person could create an account AND hide the traces.
// Compliance frameworks (SOX, ISO 27001) are built around rules like this.
#pragma once

#include <string>

#include "PermissionSet.h"

class SodRule {
public:
    SodRule(std::string name, PermissionSet exclusive)
        : name_(std::move(name)), exclusive_(std::move(exclusive)) {}

    const std::string& name() const { return name_; }
    const PermissionSet& exclusive() const { return exclusive_; }

    // The conflicting permissions this user holds, or an empty set if the rule is respected.
    // Violated when MORE THAN ONE exclusive permission is held (set intersection).
    PermissionSet heldConflict(const PermissionSet& effective) const {
        PermissionSet held = effective.intersectionWith(exclusive_);
        return held.size() > 1 ? held : PermissionSet();
    }

private:
    std::string name_;
    PermissionSet exclusive_;
};
