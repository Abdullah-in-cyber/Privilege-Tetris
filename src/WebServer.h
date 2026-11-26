// WebServer.h
// A tiny single-threaded HTTP/1.1 server (POSIX sockets, no libraries) that hands every
// request to WebApi. It listens on 127.0.0.1 ONLY, so the UI is reachable from this
// computer and nobody else. Requests are served one at a time; that is plenty for a
// local tool and keeps the code easy to read.
#pragma once

#include <string>

class WebServer {
public:
    explicit WebServer(int port) : port_(port) {}

    // Blocks serving requests until the process is stopped (Ctrl+C).
    // Returns false with `error` set if the server could not start.
    bool run(std::string& error);

private:
    int port_;
};
