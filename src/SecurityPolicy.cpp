#include "SecurityPolicy.h"

#include <set>

namespace {
// Generic helper: linear search by name, returning a pointer or nullptr.
template <typename T, typename NameFn>
const T* findByName(const std::vector<T>& items, const std::string& name, NameFn nameOf,
                    std::size_t* comparisons) {
    SearchResult r = linearSearch(items, [&](const T& item) { return nameOf(item) == name; });
    if (comparisons) *comparisons = r.comparisons;
    return r.found() ? &items[static_cast<std::size_t>(r.index)] : nullptr;
}
}  // namespace

const Permission* SecurityPolicy::findPermission(const std::string& id, std::size_t* c) const {
    return findByName(permissions_, id, [](const Permission& p) { return p.id(); }, c);
}
const Role* SecurityPolicy::findRole(const std::string& name, std::size_t* c) const {
    return findByName(roles_, name, [](const Role& r) { return r.name(); }, c);
}
const User* SecurityPolicy::findUser(const std::string& name, std::size_t* c) const {
    return findByName(users_, name, [](const User& u) { return u.name(); }, c);
}
User* SecurityPolicy::findUser(const std::string& name) {
    return const_cast<User*>(static_cast<const SecurityPolicy*>(this)->findUser(name, nullptr));
}

Role* SecurityPolicy::mutableRole(const std::string& name) {
    return const_cast<Role*>(static_cast<const SecurityPolicy*>(this)->findRole(name, nullptr));
}

bool SecurityPolicy::addSodRule(const SodRule& rule) {
    SearchResult r = linearSearch(sodRules_, [&](const SodRule& x) { return x.name() == rule.name(); });
    if (r.found()) return false;
    sodRules_.push_back(rule);
    return true;
}

ResolvedRole SecurityPolicy::resolvedRole(const std::string& roleName) const {
    ResolvedRole result;
    std::set<std::string> visited;       // guards against revisiting (diamonds) and cycles
    std::vector<std::string> pending{roleName};   // explicit stack = iterative depth-first search
    while (!pending.empty()) {
        std::string current = pending.back();
        pending.pop_back();
        if (!visited.insert(current).second) continue;
        const Role* role = findRole(current);
        if (!role) continue;
        result.grants = result.grants.unionWith(role->grants());
        result.allowed = result.allowed.unionWith(role->allowed());
        result.forbidden = result.forbidden.unionWith(role->forbidden());
        result.required = result.required.unionWith(role->required());
        for (const std::string& parent : role->parents()) pending.push_back(parent);
    }
    return result;
}

bool SecurityPolicy::addPermission(const Permission& p) {
    if (findPermission(p.id())) return false;
    permissions_.push_back(p);
    return true;
}
bool SecurityPolicy::addRole(const Role& r) {
    if (findRole(r.name())) return false;
    roles_.push_back(r);
    return true;
}
bool SecurityPolicy::addUser(const User& u) {
    if (findUser(u.name(), nullptr)) return false;
    users_.push_back(u);
    return true;
}
bool SecurityPolicy::removeUser(const std::string& name) {
    SearchResult r = linearSearch(users_, [&](const User& u) { return u.name() == name; });
    if (!r.found()) return false;
    users_.erase(users_.begin() + r.index);
    return true;
}

PermissionSet SecurityPolicy::effectivePermissions(const User& user) const {
    PermissionSet result = user.directPermissions();
    for (const std::string& roleName : user.roles()) {
        result = result.unionWith(resolvedRole(roleName).grants);  // includes inherited grants
    }
    return result;
}

EffectiveRules SecurityPolicy::rulesFor(const User& user) const {
    EffectiveRules rules;
    for (const std::string& roleName : user.roles()) {
        ResolvedRole role = resolvedRole(roleName);
        rules.allowed = rules.allowed.unionWith(role.allowed);
        rules.forbidden = rules.forbidden.unionWith(role.forbidden);
        rules.required = rules.required.unionWith(role.required);
    }
    return rules;
}

std::string SecurityPolicy::sourcesOf(const User& user, const std::string& permission) const {
    std::string out;
    auto add = [&](const std::string& s) { out += (out.empty() ? "" : ", ") + s; };
    if (user.directPermissions().contains(permission)) add("direct assignment");
    for (const std::string& roleName : user.roles()) {
        const Role* role = findRole(roleName);
        if (!role) continue;
        if (role->grants().contains(permission)) add("role " + roleName);
        else if (resolvedRole(roleName).grants.contains(permission)) add("role " + roleName + " (inherited)");
    }
    return out.empty() ? "none" : out;
}
