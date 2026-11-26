#include "ConfigIO.h"

#include <cctype>
#include <fstream>
#include <sstream>

#include "RoleHierarchy.h"

namespace {
std::string trim(const std::string& s) {
    std::size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

std::vector<std::string> words(const std::string& s) {
    std::istringstream in(s);
    std::vector<std::string> out;
    std::string w;
    while (in >> w) out.push_back(w);
    return out;
}

std::string fail(std::size_t line, const std::string& msg) {
    return "line " + std::to_string(line) + ": " + msg;
}
}  // namespace

bool ConfigIO::isValidName(const std::string& name) {
    if (name.empty() || name.size() > 40) return false;
    for (unsigned char c : name) {
        if (!std::isalnum(c) && c != '.' && c != '_' && c != '-') return false;
    }
    return true;
}

bool ConfigIO::parse(std::istream& in, SecurityPolicy& policy, std::string& error) {
    SecurityPolicy result;       // parse into a scratch object; commit only on success
    enum class Block { NONE, ROLE, USER } block = Block::NONE;
    std::string line;
    std::size_t lineNo = 0;
    Role workRole("");           // the role/user currently being read; it is added to
    User workUser("");           // `result` when the next block starts (or at end of file)

    auto closeBlock = [&](std::string& err) -> bool {
        if (block == Block::ROLE) {
            if (!result.addRole(workRole)) { err = "duplicate role '" + workRole.name() + "'"; return false; }
        } else if (block == Block::USER) {
            if (!result.addUser(workUser)) { err = "duplicate user '" + workUser.name() + "'"; return false; }
        }
        block = Block::NONE;
        return true;
    };

    while (std::getline(in, line)) {
        ++lineNo;
        std::size_t hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        line = trim(line);
        if (line.empty()) continue;

        std::string err;
        std::vector<std::string> w = words(line);
        const std::string& key = w[0];

        if (key == "permission") {
            if (!closeBlock(err)) { error = fail(lineNo, err); return false; }
            std::string rest = trim(line.substr(key.size()));
            std::size_t bar = rest.find('|');
            std::vector<std::string> head = words(bar == std::string::npos ? rest : rest.substr(0, bar));
            std::string desc = bar == std::string::npos ? "" : trim(rest.substr(bar + 1));
            if (head.empty() || head.size() > 2 || !isValidName(head[0])) {
                error = fail(lineNo, "expected: permission <id> [risk=1..10] | <description>");
                return false;
            }
            int risk = 1;
            if (head.size() == 2) {
                const std::string& token = head[1];
                bool numeric = token.size() > 5 && token.compare(0, 5, "risk=") == 0;
                for (std::size_t i = 5; numeric && i < token.size(); ++i) numeric = std::isdigit(static_cast<unsigned char>(token[i]));
                if (!numeric || token.size() > 7) { error = fail(lineNo, "bad risk '" + token + "' (use risk=1..10)"); return false; }
                risk = std::stoi(token.substr(5));
                if (risk < Permission::kMinRisk || risk > Permission::kMaxRisk) {
                    error = fail(lineNo, "risk must be between 1 and 10");
                    return false;
                }
            }
            if (!result.addPermission(Permission(head[0], desc, risk))) {
                error = fail(lineNo, "duplicate permission '" + head[0] + "'");
                return false;
            }
        } else if (key == "sod") {
            if (!closeBlock(err)) { error = fail(lineNo, err); return false; }
            if (w.size() < 4 || !isValidName(w[1])) {
                error = fail(lineNo, "expected: sod <name> <permission> <permission> ...");
                return false;
            }
            PermissionSet exclusive;
            for (std::size_t i = 2; i < w.size(); ++i) {
                if (!result.findPermission(w[i])) { error = fail(lineNo, "unknown permission '" + w[i] + "'"); return false; }
                exclusive.insert(w[i]);
            }
            if (exclusive.size() < 2) { error = fail(lineNo, "a sod rule needs at least two different permissions"); return false; }
            if (!result.addSodRule(SodRule(w[1], exclusive))) { error = fail(lineNo, "duplicate sod rule '" + w[1] + "'"); return false; }
        } else if (key == "role" || key == "user") {
            if (!closeBlock(err)) { error = fail(lineNo, err); return false; }
            if (w.size() != 2 || !isValidName(w[1])) {
                error = fail(lineNo, "expected: " + key + " <name>");
                return false;
            }
            if (key == "role") { block = Block::ROLE; workRole = Role(w[1]); }
            else { block = Block::USER; workUser = User(w[1]); }
        } else if (block == Block::ROLE &&
                   (key == "grants" || key == "allowed" || key == "forbidden" || key == "required")) {
            for (std::size_t i = 1; i < w.size(); ++i) {
                if (!result.findPermission(w[i])) {
                    error = fail(lineNo, "unknown permission '" + w[i] + "'");
                    return false;
                }
                if (key == "grants") workRole.addGrant(w[i]);
                else if (key == "allowed") workRole.addAllowed(w[i]);
                else if (key == "forbidden") workRole.addForbidden(w[i]);
                else workRole.addRequired(w[i]);
            }
        } else if (block == Block::ROLE && key == "inherits") {
            for (std::size_t i = 1; i < w.size(); ++i) {
                if (!result.findRole(w[i])) {
                    error = fail(lineNo, "unknown parent role '" + w[i] + "' (declare parents before children)");
                    return false;
                }
                workRole.addParent(w[i]);
            }
        } else if (block == Block::USER && (key == "roles" || key == "direct")) {
            for (std::size_t i = 1; i < w.size(); ++i) {
                if (key == "roles") {
                    if (!result.findRole(w[i])) {
                        error = fail(lineNo, "unknown role '" + w[i] + "'");
                        return false;
                    }
                    workUser.addRole(w[i]);
                } else {
                    if (!result.findPermission(w[i])) {
                        error = fail(lineNo, "unknown permission '" + w[i] + "'");
                        return false;
                    }
                    workUser.grantDirect(w[i]);
                }
            }
        } else {
            error = fail(lineNo, "unexpected '" + key + "' here");
            return false;
        }
    }
    std::string err;
    if (!closeBlock(err)) { error = fail(lineNo, err); return false; }
    policy = result;
    return true;
}

bool ConfigIO::load(const std::string& path, SecurityPolicy& policy, std::string& error) {
    std::ifstream file(path);
    if (!file) { error = "cannot open file '" + path + "'"; return false; }
    return parse(file, policy, error);
}

void ConfigIO::write(std::ostream& out, const SecurityPolicy& policy) {
    auto list = [&](const char* label, const PermissionSet& s) {
        if (s.empty()) return;
        out << "  " << label;
        for (const std::string& p : s.items()) out << " " << p;
        out << "\n";
    };
    out << "# Privilege Tetris configuration\n";
    for (const Permission& p : policy.permissions()) {
        out << "permission " << p.id();
        if (p.riskWeight() != 1) out << " risk=" << p.riskWeight();
        out << " | " << p.description() << "\n";
    }
    for (const SodRule& rule : policy.sodRules()) {
        out << "sod " << rule.name();
        for (const std::string& p : rule.exclusive().items()) out << " " << p;
        out << "\n";
    }
    // Parents must be written before children so the file can be read back.
    bool acyclic = true;
    std::vector<std::string> order = RoleHierarchy::topologicalOrder(policy, acyclic);
    if (!acyclic) { order.clear(); for (const Role& r : policy.roles()) order.push_back(r.name()); }
    for (const std::string& roleName : order) {
        const Role* rp = policy.findRole(roleName);
        if (!rp) continue;
        const Role& r = *rp;
        out << "\nrole " << r.name() << "\n";
        if (!r.parents().empty()) {
            out << "  inherits";
            for (const std::string& parent : r.parents()) out << " " << parent;
            out << "\n";
        }
        list("grants   ", r.grants());
        list("allowed  ", r.allowed());
        list("forbidden", r.forbidden());
        list("required ", r.required());
    }
    for (const User& u : policy.users()) {
        out << "\nuser " << u.name() << "\n";
        if (!u.roles().empty()) {
            out << "  roles ";
            for (const std::string& r : u.roles()) out << " " << r;
            out << "\n";
        }
        list("direct   ", u.directPermissions());
    }
}

bool ConfigIO::save(const std::string& path, const SecurityPolicy& policy, std::string& error) {
    std::ofstream file(path);
    if (!file) { error = "cannot write file '" + path + "'"; return false; }
    write(file, policy);
    if (!file) { error = "write error on '" + path + "'"; return false; }
    return true;
}
