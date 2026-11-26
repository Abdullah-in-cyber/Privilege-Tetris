#include "RepairAction.h"

const char* RepairAction::typeName(RepairType t) {
    switch (t) {
        case RepairType::REVOKE_DIRECT: return "REVOKE";
        case RepairType::GRANT_DIRECT: return "GRANT";
        case RepairType::REMOVE_ROLE: return "REMOVE-ROLE";
        case RepairType::ADD_ROLE: return "ADD-ROLE";
    }
    return "?";
}

std::string RepairAction::describe() const {
    std::string cost = " (cost " + std::to_string(cost_) + ")";
    switch (type_) {
        case RepairType::REVOKE_DIRECT:
            return "REVOKE permission '" + target_ + "' from user '" + user_ + "'" + cost;
        case RepairType::GRANT_DIRECT:
            return "GRANT permission '" + target_ + "' to user '" + user_ + "'" + cost;
        case RepairType::REMOVE_ROLE:
            return "REMOVE role '" + target_ + "' from user '" + user_ + "'" + cost;
        case RepairType::ADD_ROLE:
            return "ADD role '" + target_ + "' to user '" + user_ + "'" + cost;
    }
    return "?";
}

std::string RepairAction::key() const {
    return std::string(typeName(type_)) + "|" + user_ + "|" + target_;
}

void RepairAction::applyTo(User& user) const {
    switch (type_) {
        case RepairType::REVOKE_DIRECT: user.revokeDirect(target_); break;
        case RepairType::GRANT_DIRECT: user.grantDirect(target_); break;
        case RepairType::REMOVE_ROLE: user.removeRole(target_); break;
        case RepairType::ADD_ROLE: user.addRole(target_); break;
    }
}
