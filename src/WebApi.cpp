#include "WebApi.h"

#include <cstdlib>
#include <map>
#include <sstream>

#include "ComplianceScorer.h"
#include "ConfigIO.h"
#include "RoleHierarchy.h"
#include "Scenarios.h"
#include "ViolationDetector.h"

std::string webPageHtml();  // WebPage.cpp

namespace {

using Query = std::map<std::string, std::string>;

Query parseQuery(const std::string& q) {
    Query out;
    std::size_t pos = 0;
    while (pos <= q.size()) {
        std::size_t amp = q.find('&', pos);
        if (amp == std::string::npos) amp = q.size();
        std::string pair = q.substr(pos, amp - pos);
        std::size_t eq = pair.find('=');
        if (!pair.empty()) {
            if (eq == std::string::npos) out[pair] = "";
            else out[pair.substr(0, eq)] = pair.substr(eq + 1);
        }
        pos = amp + 1;
    }
    return out;
}

bool flag(const Query& q, const std::string& key, bool fallback) {
    auto it = q.find(key);
    if (it == q.end()) return fallback;
    return it->second == "1" || it->second == "true";
}

int number(const Query& q, const std::string& key, int fallback) {
    auto it = q.find(key);
    if (it == q.end() || it->second.empty()) return fallback;
    char* end = nullptr;
    long v = std::strtol(it->second.c_str(), &end, 10);
    if (*end != '\0') return fallback;
    if (v < 1) v = 1;
    if (v > 1000) v = 1000;
    return static_cast<int>(v);
}

std::string str(const std::string& s) { return WebApi::jsonString(s); }

std::string setJson(const PermissionSet& s) {
    std::string out = "[";
    bool first = true;
    for (const std::string& p : s.items()) {
        if (!first) out += ",";
        first = false;
        out += str(p);
    }
    return out + "]";
}

std::string listJson(const std::vector<std::string>& v) {
    std::string out = "[";
    for (std::size_t i = 0; i < v.size(); ++i) out += (i ? "," : "") + str(v[i]);
    return out + "]";
}

std::string namesJson(const std::set<std::string>& s) {
    return listJson(std::vector<std::string>(s.begin(), s.end()));
}

std::string policyJson(const SecurityPolicy& p) {
    std::ostringstream o;
    o << "{\"permissions\":[";
    for (std::size_t i = 0; i < p.permissions().size(); ++i) {
        const Permission& x = p.permissions()[i];
        o << (i ? "," : "") << "{\"id\":" << str(x.id()) << ",\"description\":" << str(x.description())
          << ",\"risk\":" << x.riskWeight() << "}";
    }
    o << "],\"sod\":[";
    for (std::size_t i = 0; i < p.sodRules().size(); ++i) {
        const SodRule& r = p.sodRules()[i];
        o << (i ? "," : "") << "{\"name\":" << str(r.name()) << ",\"permissions\":" << setJson(r.exclusive()) << "}";
    }
    o << "],\"roles\":[";
    for (std::size_t i = 0; i < p.roles().size(); ++i) {
        const Role& r = p.roles()[i];
        ResolvedRole res = p.resolvedRole(r.name());
        o << (i ? "," : "") << "{\"name\":" << str(r.name()) << ",\"parents\":" << namesJson(r.parents())
          << ",\"grants\":" << setJson(r.grants()) << ",\"allowed\":" << setJson(r.allowed())
          << ",\"forbidden\":" << setJson(r.forbidden()) << ",\"required\":" << setJson(r.required())
          << ",\"depth\":" << RoleHierarchy::depth(p, r.name())
          << ",\"resolved\":{\"grants\":" << setJson(res.grants) << ",\"allowed\":" << setJson(res.allowed)
          << ",\"forbidden\":" << setJson(res.forbidden) << ",\"required\":" << setJson(res.required) << "}}";
    }
    o << "],\"users\":[";
    for (std::size_t i = 0; i < p.users().size(); ++i) {
        const User& u = p.users()[i];
        PermissionSet eff = p.effectivePermissions(u);
        o << (i ? "," : "") << "{\"name\":" << str(u.name()) << ",\"roles\":" << namesJson(u.roles())
          << ",\"direct\":" << setJson(u.directPermissions()) << ",\"effective\":[";
        bool first = true;
        for (const std::string& perm : eff.items()) {
            o << (first ? "" : ",") << "{\"permission\":" << str(perm) << ",\"sources\":"
              << str(p.sourcesOf(u, perm)) << "}";
            first = false;
        }
        o << "]}";
    }
    bool ok = true;
    std::vector<std::string> topo = RoleHierarchy::topologicalOrder(p, ok);
    o << "],\"hierarchyTree\":" << str(RoleHierarchy::render(p)) << ",\"topologicalOrder\":" << listJson(topo) << "}";
    return o.str();
}

std::string configText(const SecurityPolicy& p) {
    std::ostringstream o;
    ConfigIO::write(o, p);
    return o.str();
}

std::string complianceJson(const SecurityPolicy& p) {
    ComplianceReport c = ComplianceScorer::evaluate(p);
    std::ostringstream o;
    o << "{\"overall\":" << c.overall << ",\"rating\":" << str(c.rating) << ",\"users\":[";
    for (std::size_t i = 0; i < c.users.size(); ++i)
        o << (i ? "," : "") << "{\"name\":" << str(c.users[i].userName) << ",\"score\":" << c.users[i].score
          << ",\"violations\":" << c.users[i].violations << "}";
    return o.str() + "]}";
}

std::string actionJson(const RepairAction& a) {
    return "{\"type\":" + str(RepairAction::typeName(a.type())) + ",\"user\":" + str(a.userName()) +
           ",\"target\":" + str(a.target()) + ",\"cost\":" + std::to_string(a.cost()) +
           ",\"text\":" + str(a.describe()) + "}";
}

std::string actionsJson(const std::vector<RepairAction>& v) {
    std::string out = "[";
    for (std::size_t i = 0; i < v.size(); ++i) out += (i ? "," : "") + actionJson(v[i]);
    return out + "]";
}

std::string statsJson(const SearchStats& s) {
    return "{\"generated\":" + std::to_string(s.generated) + ",\"searchSpace\":" + std::to_string(s.searchSpace) +
           ",\"nodes\":" + std::to_string(s.nodesVisited) + ",\"explored\":" + std::to_string(s.explored) +
           ",\"rejected\":" + std::to_string(s.rejected) + ",\"valid\":" + std::to_string(s.valid) +
           ",\"pruned\":" + std::to_string(s.pruned) + "}";
}

std::string violationsJson(const ViolationList& list) {
    std::string out = "[";
    bool first = true;
    for (const Violation& v : list) {
        out += std::string(first ? "" : ",") + "{\"type\":" + str(Violation::typeName(v.type())) +
               ",\"severity\":" + str(Violation::severityName(v.severity())) + ",\"user\":" + str(v.userName()) +
               ",\"role\":" + str(v.roleName()) + ",\"permission\":" + str(v.permission()) +
               ",\"description\":" + str(v.description()) + ",\"roleLevel\":" + (v.isRoleLevel() ? "true" : "false") + "}";
        first = false;
    }
    return out + "]";
}

std::string planJson(const RepairPlan& plan) {
    std::ostringstream o;
    o << "{\"solvable\":" << (plan.solvable ? "true" : "false") << ",\"totalCost\":" << plan.totalCost
      << ",\"totals\":" << statsJson(plan.totals) << ",\"perUser\":[";
    for (std::size_t i = 0; i < plan.perUser.size(); ++i) {
        const UserRepairResult& r = plan.perUser[i];
        o << (i ? "," : "") << "{\"user\":" << str(r.userName) << ",\"solvable\":" << (r.solvable ? "true" : "false")
          << ",\"tooLarge\":" << (r.searchTooLarge ? "true" : "false") << ",\"cost\":" << (r.solvable ? r.best.cost : -1)
          << ",\"stats\":" << statsJson(r.stats) << ",\"candidates\":" << actionsJson(r.candidates)
          << ",\"best\":" << (r.solvable ? actionsJson(r.best.actions) : "[]") << ",\"alternatives\":[";
        for (std::size_t j = 0; j < r.alternatives.size(); ++j)
            o << (j ? "," : "") << "{\"cost\":" << r.alternatives[j].cost << ",\"actions\":"
              << actionsJson(r.alternatives[j].actions) << "}";
        o << "],\"rejections\":[";
        bool first = true;
        for (const auto& kv : r.rejectionReasons) {
            o << (first ? "" : ",") << "{\"reason\":" << str(kv.first) << ",\"count\":" << kv.second << "}";
            first = false;
        }
        o << "],\"explanation\":" << str(r.explanation) << "}";
    }
    o << "]}";
    return o.str();
}

std::string validationJson(const ValidationReport& v) {
    std::ostringstream o;
    o << "{\"passed\":" << (v.passed ? "true" : "false") << ",\"remainingViolations\":" << v.remainingViolations
      << ",\"checks\":[";
    for (std::size_t i = 0; i < v.checks.size(); ++i)
        o << (i ? "," : "") << "{\"name\":" << str(v.checks[i].name) << ",\"passed\":"
          << (v.checks[i].passed ? "true" : "false") << ",\"details\":" << str(v.checks[i].details) << "}";
    return o.str() + "]}";
}

HttpResponse jsonError(int status, const std::string& message) {
    HttpResponse r;
    r.status = status;
    r.body = "{\"ok\":false,\"error\":" + str(message) + "}";
    return r;
}

bool parseBody(const std::string& body, SecurityPolicy& policy, std::string& error) {
    if (body.size() > WebApi::kMaxBodyBytes) { error = "configuration is too large"; return false; }
    std::istringstream in(body);
    return ConfigIO::parse(in, policy, error);
}

HttpResponse scenarios() {
    std::ostringstream o;
    o << "{\"ok\":true,\"scenarios\":[";
    std::vector<Scenario> all = buildScenarios();
    for (std::size_t i = 0; i < all.size(); ++i) {
        const Scenario& s = all[i];
        o << (i ? "," : "") << "{\"id\":" << s.id << ",\"title\":" << str(s.title)
          << ",\"description\":" << str(s.description) << ",\"expected\":" << str(s.expectedBehavior)
          << ",\"expectedCost\":" << s.expectedCost << ",\"expectedViolations\":" << s.expectedViolations
          << ",\"config\":" << str(configText(s.policy)) << "}";
    }
    o << "]}";
    HttpResponse r;
    r.body = o.str();
    return r;
}

HttpResponse parse(const std::string& body) {
    SecurityPolicy p;
    std::string error;
    if (!parseBody(body, p, error)) return jsonError(400, error);
    HttpResponse r;
    r.body = "{\"ok\":true,\"policy\":" + policyJson(p) + "}";
    return r;
}

HttpResponse analyze(const Query& q, const std::string& body) {
    SecurityPolicy p;
    std::string error;
    if (!parseBody(body, p, error)) return jsonError(400, error);

    CostModel cost;
    cost.setUniform(flag(q, "uniform", false));
    cost.setWeight(RepairType::REVOKE_DIRECT, number(q, "revoke", 1));
    cost.setWeight(RepairType::GRANT_DIRECT, number(q, "grant", 1));
    cost.setWeight(RepairType::REMOVE_ROLE, number(q, "removerole", 2));
    cost.setWeight(RepairType::ADD_ROLE, number(q, "addrole", 2));
    bool pruning = flag(q, "pruning", true);

    PipelineResult res = runPipeline(p, cost, pruning);
    ViolationList violations = ViolationDetector::detectAll(p);

    std::ostringstream o;
    o << "{\"ok\":true,\"policy\":" << policyJson(p) << ",\"costModel\":" << str(cost.describe())
      << ",\"pruning\":" << (pruning ? "true" : "false") << ",\"violations\":" << violationsJson(violations)
      << ",\"userViolations\":" << res.userViolations << ",\"roleIssues\":" << res.roleIssues
      << ",\"compliance\":" << complianceJson(p) << ",\"plan\":" << planJson(res.plan) << ",\"repaired\":";
    if (res.simulated) {
        o << "{\"policy\":" << policyJson(res.repaired) << ",\"config\":" << str(configText(res.repaired))
          << ",\"compliance\":" << complianceJson(res.repaired) << ",\"actions\":"
          << actionsJson(res.plan.allActions()) << ",\"validation\":" << validationJson(res.validation) << "}";
    } else {
        o << "null";
    }
    o << "}";
    HttpResponse r;
    r.body = o.str();
    return r;
}

}  // namespace

std::string WebApi::jsonString(const std::string& s) {
    std::string out = "\"";
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    static const char* hex = "0123456789abcdef";
                    out += "\\u00";
                    out += hex[c >> 4];
                    out += hex[c & 15];
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out + "\"";
}

HttpResponse WebApi::handle(const std::string& method, const std::string& path,
                            const std::string& query, const std::string& body) {
    if (path == "/" || path == "/index.html") {
        if (method != "GET") return jsonError(405, "use GET");
        HttpResponse r;
        r.contentType = "text/html; charset=utf-8";
        r.body = webPageHtml();
        return r;
    }
    if (path == "/api/scenarios") return method == "GET" ? scenarios() : jsonError(405, "use GET");
    if (path == "/api/parse") return method == "POST" ? parse(body) : jsonError(405, "use POST");
    if (path == "/api/analyze") return method == "POST" ? analyze(parseQuery(query), body) : jsonError(405, "use POST");
    return jsonError(404, "unknown path " + path);
}
