// RoleHierarchy.h
// Graph algorithms over the role-inheritance DAG (directed acyclic graph).
// Each role is a node; "child inherits parent" is an edge child -> parent.
//
//   addInheritance ...... DFS cycle check before adding an edge      O(R + E)
//   topologicalOrder .... Kahn's algorithm, parents listed first     O(R + E)
//   ancestors / depth ... DFS / DFS with memo                        O(R + E)
//   render .............. indented tree for the console              O(R + E)
// R = roles, E = inheritance links.
#pragma once

#include <string>
#include <vector>

#include "SecurityPolicy.h"

class RoleHierarchy {
public:
    // Adds "child inherits parent" unless it would be invalid. On failure `error` says why
    // (unknown role, self-inheritance, duplicate link, or the cycle path that would form).
    static bool addInheritance(SecurityPolicy& policy, const std::string& child,
                               const std::string& parent, std::string& error);

    // Would the edge child -> parent close a loop? (true if `child` is already an ancestor of
    // `parent`, or equal to it). `path` receives e.g. "A -> B -> C -> A".
    static bool wouldCreateCycle(const SecurityPolicy& policy, const std::string& child,
                                 const std::string& parent, std::string& path);

    // Parents before children. `ok` is false (and the list incomplete) if a cycle exists.
    static std::vector<std::string> topologicalOrder(const SecurityPolicy& policy, bool& ok);

    // All roles reachable by following parent links (excluding the role itself), sorted by name.
    static std::vector<std::string> ancestors(const SecurityPolicy& policy, const std::string& role);

    // Length of the longest parent chain above the role (0 for a root).
    static int depth(const SecurityPolicy& policy, const std::string& role);

    // Indented tree, roots first; roles with several parents appear under each parent.
    static std::string render(const SecurityPolicy& policy);
};
