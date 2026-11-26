// ConfigIO.h
// Reads and writes a configuration in a simple human-readable text format:
//
//   # comment
//   permission grades.edit risk=3 | Edit student grades     (risk=1..10, default 1)
//   sod SOD-ADMIN-AUDIT accounts.manage audit.export       (a user may hold at most one of these)
//   role HeadTeacher
//     inherits  Teacher                                   (parent role must be declared earlier)
//     grants    logs.read
//     allowed   grades.view courses.enroll
//     forbidden grades.edit
//     required  grades.view
//   user alice
//     roles  Student
//     direct grades.edit
//
// Permissions must be declared before roles/sod rules use them; parent roles before children;
// roles before users.
#pragma once

#include <iosfwd>
#include <string>

#include "SecurityPolicy.h"

class ConfigIO {
public:
    // Returns true on success. On failure `error` explains the problem
    // (with a line number) and `policy` is left unchanged.
    static bool load(const std::string& path, SecurityPolicy& policy, std::string& error);
    static bool parse(std::istream& in, SecurityPolicy& policy, std::string& error);

    static bool save(const std::string& path, const SecurityPolicy& policy, std::string& error);
    static void write(std::ostream& out, const SecurityPolicy& policy);

    static bool isValidName(const std::string& name);  // letters, digits, . _ -
};
