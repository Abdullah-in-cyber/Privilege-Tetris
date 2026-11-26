#include "Input.h"

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include "ConfigIO.h"

namespace {
std::string trim(const std::string& s) {
    std::size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}
}  // namespace

std::string Input::line(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string text;
    if (!std::getline(std::cin, text)) throw InputClosed();
    return trim(text);
}

int Input::integer(const std::string& prompt, int lo, int hi) {
    while (true) {
        std::string text = line(prompt);
        char* end = nullptr;
        errno = 0;
        long value = std::strtol(text.c_str(), &end, 10);
        if (!text.empty() && *end == '\0' && errno == 0 && value >= lo && value <= hi) {
            return static_cast<int>(value);
        }
        std::cout << "  [!] Please enter a whole number between " << lo << " and " << hi << ".\n";
    }
}

bool Input::yesNo(const std::string& prompt) {
    while (true) {
        std::string t = line(prompt + " (y/n): ");
        if (t == "y" || t == "Y" || t == "yes" || t == "YES") return true;
        if (t == "n" || t == "N" || t == "no" || t == "NO") return false;
        std::cout << "  [!] Please answer y or n.\n";
    }
}

std::string Input::name(const std::string& prompt) {
    while (true) {
        std::string t = line(prompt);
        if (ConfigIO::isValidName(t)) return t;
        std::cout << "  [!] Use 1-40 characters: letters, digits, '.', '_' or '-' (no spaces).\n";
    }
}

std::vector<std::string> Input::words(const std::string& prompt) {
    std::string t = line(prompt);
    for (char& c : t) if (c == ',') c = ' ';
    std::istringstream in(t);
    std::vector<std::string> out;
    std::string w;
    while (in >> w) out.push_back(w);
    return out;
}
