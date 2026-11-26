#include "WebServer.h"

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>

#include "WebApi.h"

#ifdef _WIN32
bool WebServer::run(std::string& error) {
    error = "the web UI uses POSIX sockets and is not available on Windows (use WSL, or the console menu)";
    return false;
}
#else

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace {

constexpr std::size_t kMaxHeaderBytes = 16 * 1024;

const char* reasonPhrase(int status) {
    switch (status) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        default: return "Internal Server Error";
    }
}

std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string headerValue(const std::string& head, const std::string& name) {
    std::string lowered = lower(head);
    std::string needle = "\r\n" + lower(name) + ":";
    std::size_t pos = lowered.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    std::size_t end = head.find("\r\n", pos);
    std::string v = head.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
    std::size_t a = v.find_first_not_of(" \t"), b = v.find_last_not_of(" \t");
    return a == std::string::npos ? "" : v.substr(a, b - a + 1);
}

void sendAll(int fd, const std::string& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
        ssize_t n = ::send(fd, data.data() + sent, data.size() - sent, 0);
        if (n <= 0) return;  // client went away; nothing more to do
        sent += static_cast<std::size_t>(n);
    }
}

void respond(int fd, const HttpResponse& r) {
    std::string out = "HTTP/1.1 " + std::to_string(r.status) + " " + reasonPhrase(r.status) + "\r\n" +
                      "Content-Type: " + r.contentType + "\r\n" +
                      "Content-Length: " + std::to_string(r.body.size()) + "\r\n" +
                      "Cache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\n" +
                      "Connection: close\r\n\r\n" + r.body;
    sendAll(fd, out);
}

HttpResponse plainError(int status, const std::string& message) {
    HttpResponse r;
    r.status = status;
    r.body = "{\"ok\":false,\"error\":" + WebApi::jsonString(message) + "}";
    return r;
}

// Only accept requests addressed to this machine. This blocks DNS-rebinding and
// cross-site requests from other web pages.
bool hostAllowed(const std::string& host) {
    std::string h = lower(host);
    std::size_t colon = h.rfind(':');
    if (colon != std::string::npos) h = h.substr(0, colon);
    return h == "localhost" || h == "127.0.0.1";
}

void serve(int fd) {
    timeval tv{5, 0};  // a stalled client must not block the server
    ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);

    std::string data;
    char buf[4096];
    std::size_t headEnd = std::string::npos;
    while (headEnd == std::string::npos) {
        ssize_t n = ::recv(fd, buf, sizeof buf, 0);
        if (n <= 0) return;
        data.append(buf, static_cast<std::size_t>(n));
        headEnd = data.find("\r\n\r\n");
        if (headEnd == std::string::npos && data.size() > kMaxHeaderBytes) {
            respond(fd, plainError(400, "request headers too large"));
            return;
        }
    }

    std::string head = data.substr(0, headEnd);
    std::string body = data.substr(headEnd + 4);

    std::size_t lineEnd = head.find("\r\n");
    std::string requestLine = head.substr(0, lineEnd);
    std::size_t s1 = requestLine.find(' '), s2 = requestLine.find(' ', s1 + 1);
    if (s1 == std::string::npos || s2 == std::string::npos) { respond(fd, plainError(400, "malformed request")); return; }
    std::string method = requestLine.substr(0, s1);
    std::string target = requestLine.substr(s1 + 1, s2 - s1 - 1);

    if (!hostAllowed(headerValue(head, "Host"))) { respond(fd, plainError(403, "only localhost is allowed")); return; }
    std::string origin = headerValue(head, "Origin");
    if (!origin.empty() && origin != "http://" + headerValue(head, "Host")) {
        respond(fd, plainError(403, "cross-origin request refused"));
        return;
    }

    std::size_t contentLength = 0;
    std::string cl = headerValue(head, "Content-Length");
    if (!cl.empty()) {
        char* end = nullptr;
        unsigned long long v = std::strtoull(cl.c_str(), &end, 10);
        if (*end != '\0') { respond(fd, plainError(400, "bad Content-Length")); return; }
        if (v > WebApi::kMaxBodyBytes) { respond(fd, plainError(413, "request body too large")); return; }
        contentLength = static_cast<std::size_t>(v);
    }
    while (body.size() < contentLength) {
        ssize_t n = ::recv(fd, buf, sizeof buf, 0);
        if (n <= 0) { respond(fd, plainError(400, "incomplete request body")); return; }
        body.append(buf, static_cast<std::size_t>(n));
    }
    body.resize(contentLength);

    std::string path = target, query;
    std::size_t q = target.find('?');
    if (q != std::string::npos) { path = target.substr(0, q); query = target.substr(q + 1); }

    HttpResponse response;
    try {
        response = WebApi::handle(method, path, query, body);
    } catch (const std::exception& e) {
        response = plainError(500, std::string("internal error: ") + e.what());
    }
    respond(fd, response);
}

}  // namespace

bool WebServer::run(std::string& error) {
    ::signal(SIGPIPE, SIG_IGN);
    int listener = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) { error = std::string("socket: ") + std::strerror(errno); return false; }
    int yes = 1;
    ::setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // 127.0.0.1 only
    addr.sin_port = htons(static_cast<uint16_t>(port_));
    if (::bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof addr) < 0) {
        error = "cannot listen on port " + std::to_string(port_) + ": " + std::strerror(errno) +
                " (try another port: --web 8081)";
        ::close(listener);
        return false;
    }
    if (::listen(listener, 16) < 0) { error = std::string("listen: ") + std::strerror(errno); ::close(listener); return false; }

    std::cout << "Privilege Tetris web UI running at http://localhost:" << port_ << "\n"
              << "(listening on this computer only; press Ctrl+C to stop)\n" << std::flush;
    for (;;) {
        int fd = ::accept(listener, nullptr, nullptr);
        if (fd < 0) continue;
        serve(fd);
        ::close(fd);
    }
}

#endif
