// SecurityPolicy.h
// The whole configuration: permission catalog, roles and users.
// It is a plain value type, so copying it gives an independent snapshot -
// that is how repairs are simulated without touching the original.
#pragma once

#include <string>
#include <vector>

#include "LinearSearch.h"
#include "Permission.h"
#include "PermissionSet.h"
#include "Role.h"
#include "SodRule.h"
#include "User.h"

// The rules that apply to ONE user = union of the rules of all roles they hold.
struct EffectiveRules {
    PermissionSet allowed;
    PermissionSet forbidden;
    PermissionSet required;
};

// A role's sets after merging in everything inherited from its ancestors.
struct ResolvedRole {
    PermissionSet grants, allowed, forbidden, required;
};

class SecurityPolicy {
public:
    // ----- building -----
    bool addPermission(const Permission& p);  // false if the id already exists
    bool addRole(const Role& r);              // false if the name already exists
    bool addUser(const User& u);              // false if the name already exists
    bool removeUser(const std::string& name);
    bool addSodRule(const SodRule& rule);     // false if the name already exists

    // ----- read access -----
    const std::vector<Permission>& permissions() const { return permissions_; }
    const std::vector<Role>& roles() const { return roles_; }
    const std::vector<User>& users() const { return users_; }
    const std::vector<SodRule>& sodRules() const { return sodRules_; }
    bool empty() const { return users_.empty() && roles_.empty() && permissions_.empty(); }

    // ----- linear-search lookups (optional out-parameter = comparisons made) -----
    const Permission* findPermission(const std::string& id, std::size_t* comparisons = nullptr) const;
    const Role* findRole(const std::string& name, std::size_t* comparisons = nullptr) const;
    const User* findUser(const std::string& name, std::size_t* comparisons = nullptr) const;
    User* findUser(const std::string& name);
    Role* mutableRole(const std::string& name);  // for editing (e.g. adding a parent)

    // ----- role hierarchy -----
    // Own sets united with those of all ancestors (depth-first walk, safe even if a cycle sneaked in).
    ResolvedRole resolvedRole(const std::string& roleName) const;

    // ----- derived permission data -----
    PermissionSet effectivePermissions(const User& user) const;  // role grants ∪ direct
    EffectiveRules rulesFor(const User& user) const;
    // Which sources give `permission` to `user`? e.g. "direct assignment, role Admin"
    std::string sourcesOf(const User& user, const std::string& permission) const;

private:
    std::vector<Permission> permissions_;
    std::vector<Role> roles_;
    std::vector<User> users_;
    std::vector<SodRule> sodRules_;
};
