// Input.h
// Safe console input. Every function re-prompts on bad input instead of crashing.
// If the input stream is closed (Ctrl-D / end of piped input) it throws
// InputClosed so the program can exit cleanly.
#pragma once

#include <string>
#include <vector>

struct InputClosed {};

class Input {
public:
    static std::string line(const std::string& prompt);
    static int integer(const std::string& prompt, int lo, int hi);
    static bool yesNo(const std::string& prompt);
    static std::string name(const std::string& prompt);              // validated identifier
    static std::vector<std::string> words(const std::string& prompt);  // whitespace/comma separated
};
