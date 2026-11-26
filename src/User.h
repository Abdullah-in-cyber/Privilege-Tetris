// User.h
// A person in the system. A user can hold several roles and may also carry
// individual permission overrides ("direct" permissions) on top of them.
#pragma once

#include <set>
#include <string>

#include "PermissionSet.h"

class User {
public:
    explicit User(std::string name) : name_(std::move(name)) {}

    const std::string& name() const { return name_; }
    const std::set<std::string>& roles() const { return roles_; }
    const PermissionSet& directPermissions() const { return direct_; }

    bool hasRole(const std::string& role) const { return roles_.count(role) > 0; }
    bool addRole(const std::string& role) { return roles_.insert(role).second; }
    bool removeRole(const std::string& role) { return roles_.erase(role) > 0; }
    bool grantDirect(const std::string& p) { return direct_.insert(p); }
    bool revokeDirect(const std::string& p) { return direct_.erase(p); }

private:
    std::string name_;
    std::set<std::string> roles_;
    PermissionSet direct_;
};
