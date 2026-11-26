// Role.h
// A role bundles what its holders RECEIVE (grants) with the policy rules that
// apply to them (allowed / forbidden / required permissions).
//   grants    : permissions a holder actually gets from this role
//   allowed   : the most a holder may ever have (upper bound)
//   forbidden : permissions a holder must never have
//   required  : permissions a holder must always have (operational access)
//   parents   : roles this role INHERITS from (role hierarchy, a DAG).
//               The role gets the union of its own and all ancestors' sets.
//               Use SecurityPolicy::resolvedRole() to obtain the combined sets.
#pragma once

#include <set>
#include <string>

#include "PermissionSet.h"

class Role {
public:
    explicit Role(std::string name) : name_(std::move(name)) {}

    const std::string& name() const { return name_; }
    const PermissionSet& grants() const { return grants_; }
    const PermissionSet& allowed() const { return allowed_; }
    const PermissionSet& forbidden() const { return forbidden_; }
    const PermissionSet& required() const { return required_; }
    const std::set<std::string>& parents() const { return parents_; }

    void addGrant(const std::string& p) { grants_.insert(p); }
    void addAllowed(const std::string& p) { allowed_.insert(p); }
    void addForbidden(const std::string& p) { forbidden_.insert(p); }
    void addRequired(const std::string& p) { required_.insert(p); }
    // Low level: does NOT check for cycles. Use RoleHierarchy::addInheritance() instead.
    bool addParent(const std::string& role) { return parents_.insert(role).second; }

private:
    std::string name_;
    PermissionSet grants_, allowed_, forbidden_, required_;
    std::set<std::string> parents_;
};
