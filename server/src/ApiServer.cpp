#include "ApiServer.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

ApiServer::~ApiServer() {
    shutdown();
}

bool ApiServer::init(const std::string& socketPath) {
    m_socketPath = socketPath;

    // Remove old socket
    unlink(socketPath.c_str());

    m_listenerFd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (m_listenerFd < 0) {
        fprintf(stderr, "[api] socket() failed: %s\n", strerror(errno));
        return false;
    }

    struct sockaddr_un addr = {};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(m_listenerFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "[api] bind(%s) failed: %s\n", socketPath.c_str(), strerror(errno));
        close(m_listenerFd);
        m_listenerFd = -1;
        return false;
    }

    // Make socket accessible by non-root processes (for Next.js API routes)
    chmod(socketPath.c_str(), 0666);

    if (listen(m_listenerFd, 8) < 0) {
        fprintf(stderr, "[api] listen() failed: %s\n", strerror(errno));
        close(m_listenerFd);
        m_listenerFd = -1;
        unlink(socketPath.c_str());
        return false;
    }

    fprintf(stderr, "[api] Listening on %s\n", socketPath.c_str());
    return true;
}

void ApiServer::shutdown() {
    for (int fd : m_clients)
        close(fd);
    m_clients.clear();
    m_clientBuffers.clear();

    if (m_listenerFd >= 0) {
        close(m_listenerFd);
        m_listenerFd = -1;
    }

    if (!m_socketPath.empty()) {
        unlink(m_socketPath.c_str());
        m_socketPath.clear();
    }
}

void ApiServer::registerCommand(const std::string& name, CommandHandler handler) {
    m_handlers[name] = handler;
}

int ApiServer::acceptClient() {
    int fd = accept4(m_listenerFd, nullptr, nullptr, SOCK_NONBLOCK);
    if (fd >= 0) {
        m_clients.push_back(fd);
        m_clientBuffers[fd] = "";
    }
    return fd;
}

bool ApiServer::processClient(int clientFd) {
    char buf[4096];
    int n = read(clientFd, buf, sizeof(buf));

    if (n <= 0) {
        // Client disconnected or error
        removeClient(clientFd);
        return true;
    }

    m_clientBuffers[clientFd].append(buf, n);
    std::string& buffer = m_clientBuffers[clientFd];

    // Check for complete message (newline-terminated JSON)
    size_t pos = buffer.find('\n');
    if (pos == std::string::npos) return false;

    std::string message = buffer.substr(0, pos);
    buffer.erase(0, pos + 1);

    // Parse and dispatch
    nlohmann::json response;
    try {
        auto request = nlohmann::json::parse(message);
        response = dispatch(request);
    } catch (const std::exception& e) {
        response = {{"error", std::string("Parse error: ") + e.what()}};
    }

    // Send response
    std::string responseStr = response.dump() + "\n";
    write(clientFd, responseStr.c_str(), responseStr.size());

    // Close client after response
    removeClient(clientFd);
    return true;
}

void ApiServer::removeClient(int fd) {
    close(fd);
    m_clientBuffers.erase(fd);
    auto it = std::find(m_clients.begin(), m_clients.end(), fd);
    if (it != m_clients.end())
        m_clients.erase(it);
}

nlohmann::json ApiServer::dispatch(const nlohmann::json& request) {
    if (!request.contains("command") || !request["command"].is_string()) {
        return {{"error", "Missing 'command' field"}};
    }

    std::string cmd = request["command"].get<std::string>();
    auto it = m_handlers.find(cmd);
    if (it == m_handlers.end()) {
        return {{"error", "Unknown command: " + cmd}};
    }

    try {
        return it->second(request);
    } catch (const std::exception& e) {
        return {{"error", std::string("Handler error: ") + e.what()}};
    }
}
