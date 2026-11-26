#include "RoleHierarchy.h"

#include <algorithm>
#include <map>
#include <set>

namespace {
// Depth-first search along parent links looking for `target`.
// Fills `trail` with the path from `from` to `target` when found.
bool dfsFind(const SecurityPolicy& policy, const std::string& from, const std::string& target,
             std::set<std::string>& visited, std::vector<std::string>& trail) {
    trail.push_back(from);
    if (from == target) return true;
    if (visited.insert(from).second) {
        if (const Role* role = policy.findRole(from)) {
            for (const std::string& parent : role->parents()) {
                if (dfsFind(policy, parent, target, visited, trail)) return true;
            }
        }
    }
    trail.pop_back();  // dead end: backtrack
    return false;
}

int depthOf(const SecurityPolicy& policy, const std::string& role, std::map<std::string, int>& memo,
            std::set<std::string>& onPath) {
    auto cached = memo.find(role);
    if (cached != memo.end()) return cached->second;
    if (!onPath.insert(role).second) return 0;  // cycle guard
    int best = 0;
    if (const Role* r = policy.findRole(role)) {
        for (const std::string& parent : r->parents()) {
            best = std::max(best, 1 + depthOf(policy, parent, memo, onPath));
        }
    }
    onPath.erase(role);
    memo[role] = best;
    return best;
}

void renderNode(const SecurityPolicy& policy, const std::string& name, int indent,
                const std::map<std::string, std::vector<std::string>>& children, std::string& out,
                std::set<std::string>& onPath) {
    out += std::string(static_cast<std::size_t>(indent) * 4 + 2, ' ') + (indent ? "+- " : "") + name + "\n";
    if (!onPath.insert(name).second) return;  // cycle guard
    auto it = children.find(name);
    if (it != children.end()) {
        for (const std::string& child : it->second) renderNode(policy, child, indent + 1, children, out, onPath);
    }
    onPath.erase(name);
}
}  // namespace

bool RoleHierarchy::wouldCreateCycle(const SecurityPolicy& policy, const std::string& child,
                                     const std::string& parent, std::string& path) {
    // The new edge child -> parent closes a loop iff `child` is reachable from `parent`.
    std::set<std::string> visited;
    std::vector<std::string> trail;  // parent ... child
    if (!dfsFind(policy, parent, child, visited, trail)) return false;
    path = child;
    for (const std::string& step : trail) path += " -> " + step;  // child -> parent -> ... -> child
    return true;
}

bool RoleHierarchy::addInheritance(SecurityPolicy& policy, const std::string& child,
                                   const std::string& parent, std::string& error) {
    const Role* c = policy.findRole(child);
    const Role* p = policy.findRole(parent);
    if (!c) { error = "unknown role '" + child + "'"; return false; }
    if (!p) { error = "unknown role '" + parent + "'"; return false; }
    if (child == parent) { error = "a role cannot inherit from itself"; return false; }
    if (c->parents().count(parent)) { error = "'" + child + "' already inherits '" + parent + "'"; return false; }
    std::string path;
    if (wouldCreateCycle(policy, child, parent, path)) {
        error = "this would create an inheritance cycle: " + path;
        return false;
    }
    policy.mutableRole(child)->addParent(parent);
    return true;
}

std::vector<std::string> RoleHierarchy::topologicalOrder(const SecurityPolicy& policy, bool& ok) {
    // Kahn's algorithm: repeatedly output roles whose parents have all been output.
    std::map<std::string, int> pendingParents;
    std::map<std::string, std::vector<std::string>> children;
    for (const Role& r : policy.roles()) {
        int count = 0;
        for (const std::string& parent : r.parents()) {
            if (policy.findRole(parent)) { ++count; children[parent].push_back(r.name()); }
        }
        pendingParents[r.name()] = count;
    }
    std::vector<std::string> ready, order;
    for (const Role& r : policy.roles()) if (pendingParents[r.name()] == 0) ready.push_back(r.name());
    while (!ready.empty()) {
        std::string next = ready.front();
        ready.erase(ready.begin());
        order.push_back(next);
        for (const std::string& child : children[next]) {
            if (--pendingParents[child] == 0) ready.push_back(child);
        }
    }
    ok = order.size() == policy.roles().size();  // leftover roles are stuck in a cycle
    return order;
}

std::vector<std::string> RoleHierarchy::ancestors(const SecurityPolicy& policy, const std::string& role) {
    std::set<std::string> seen;
    std::vector<std::string> stack{role};
    while (!stack.empty()) {
        std::string current = stack.back();
        stack.pop_back();
        if (const Role* r = policy.findRole(current)) {
            for (const std::string& parent : r->parents()) {
                if (seen.insert(parent).second) stack.push_back(parent);
            }
        }
    }
    seen.erase(role);
    return std::vector<std::string>(seen.begin(), seen.end());
}

int RoleHierarchy::depth(const SecurityPolicy& policy, const std::string& role) {
    std::map<std::string, int> memo;
    std::set<std::string> onPath;
    return depthOf(policy, role, memo, onPath);
}

std::string RoleHierarchy::render(const SecurityPolicy& policy) {
    std::map<std::string, std::vector<std::string>> children;
    std::vector<std::string> roots;
    for (const Role& r : policy.roles()) {
        bool hasKnownParent = false;
        for (const std::string& parent : r.parents()) {
            if (policy.findRole(parent)) { hasKnownParent = true; children[parent].push_back(r.name()); }
        }
        if (!hasKnownParent) roots.push_back(r.name());
    }
    std::string out;
    std::set<std::string> onPath;
    for (const std::string& root : roots) renderNode(policy, root, 0, children, out, onPath);
    return out;
}
