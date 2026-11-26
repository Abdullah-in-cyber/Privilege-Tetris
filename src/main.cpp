// main.cpp - entry point only. All logic lives in the other modules.
//
// Usage:
//   privilege_tetris                  interactive menu
//   privilege_tetris --scenarios      run the 6 scenarios, print a summary table
//   privilege_tetris --run N          print the full report for scenario N (1-6)
//   privilege_tetris --export DIR     write the scenarios as DIR/scenarioN.txt
//   privilege_tetris --web [PORT]     start the browser UI on http://localhost:PORT (default 8080)
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "ConfigIO.h"
#include "Input.h"
#include "Menu.h"
#include "Scenarios.h"
#include "WebServer.h"

int main(int argc, char** argv) {
    CostModel cost;
    if (argc >= 2) {
        std::string arg = argv[1];
        if (arg == "--scenarios") return printScenarioSummary(std::cout, cost, true) == 0 ? 0 : 1;
        if (arg == "--run" && argc >= 3) {
            int id = std::atoi(argv[2]);
            if (!printScenarioReport(std::cout, id, cost, true)) {
                std::cerr << "No such scenario (use 1-6).\n";
                return 2;
            }
            return 0;
        }
        if (arg == "--export" && argc >= 3) {
            for (const Scenario& s : buildScenarios()) {
                std::string path = std::string(argv[2]) + "/scenario" + std::to_string(s.id) + ".txt";
                std::string error;
                if (!ConfigIO::save(path, s.policy, error)) { std::cerr << error << "\n"; return 2; }
                std::cout << "wrote " << path << "\n";
            }
            return 0;
        }
        if (arg == "--web") {
            int port = argc >= 3 ? std::atoi(argv[2]) : 8080;
            if (port < 1 || port > 65535) { std::cerr << "Port must be 1-65535.\n"; return 2; }
            std::string error;
            WebServer server(port);
            if (!server.run(error)) { std::cerr << error << "\n"; return 2; }
            return 0;
        }
        std::cerr << "Usage: " << argv[0] << " [--scenarios | --run N | --export DIR | --web [PORT]]\n";
        return 2;
    }

    try {
        Application app;
        app.run();
    } catch (const InputClosed&) {
        std::cout << "\nInput closed. Goodbye.\n";
    }
    return 0;
}
