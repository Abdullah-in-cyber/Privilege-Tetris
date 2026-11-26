// WebApi.h
// The HTTP-independent part of the web UI: turns a request (method, path, query, body)
// into a response. No sockets here, so it can be unit-tested directly.
//
// The API is STATELESS. The browser owns the configuration and sends it as text
// (the same format as the data/*.txt files) with every request; the server parses it,
// runs the engine on a copy and answers with JSON. Nothing is stored on the server.
//
//   GET  /                      the single-page UI
//   GET  /api/scenarios         the predefined scenarios (with their configuration text)
//   POST /api/parse             body = config text  -> structured policy JSON, or an error
//   POST /api/analyze           body = config text  -> violations, score, repair plan,
//                               simulated result and validation. Query options:
//                               pruning=0|1  uniform=0|1  revoke=N grant=N removerole=N addrole=N
#pragma once

#include <string>

struct HttpResponse {
    int status = 200;
    std::string contentType = "application/json; charset=utf-8";
    std::string body;
};

class WebApi {
public:
    static constexpr std::size_t kMaxBodyBytes = 1 << 20;  // 1 MiB is far more than any policy

    static HttpResponse handle(const std::string& method, const std::string& path,
                               const std::string& query, const std::string& body);

    // JSON string literal (with quotes), escaping control characters. Exposed for tests.
    static std::string jsonString(const std::string& s);
};
