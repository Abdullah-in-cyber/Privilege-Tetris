// RepairAction.h
// ONE elementary change to ONE user's configuration, together with its cost.
#pragma once

#include <string>

#include "User.h"

enum class RepairType {
    REVOKE_DIRECT,  // remove an individually assigned permission
    GRANT_DIRECT,   // give an individual permission
    REMOVE_ROLE,    // take a role away from the user
    ADD_ROLE        // give the user an additional role
};

class RepairAction {
public:
    RepairAction(RepairType type, std::string user, std::string target, int cost)
        : type_(type), user_(std::move(user)), target_(std::move(target)), cost_(cost) {}

    RepairType type() const { return type_; }
    const std::string& userName() const { return user_; }
    const std::string& target() const { return target_; }  // permission id or role name
    int cost() const { return cost_; }

    std::string describe() const;  // human readable sentence
    std::string key() const;       // unique id, used to avoid duplicate candidates
    void applyTo(User& user) const;  // performs the change on a (copy of a) user

    static const char* typeName(RepairType t);

private:
    RepairType type_;
    std::string user_;
    std::string target_;
    int cost_;
};
