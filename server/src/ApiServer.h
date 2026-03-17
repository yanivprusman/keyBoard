#pragma once
#include <string>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>

class ApiServer {
public:
    using CommandHandler = std::function<nlohmann::json(const nlohmann::json&)>;

    ~ApiServer();

    // Create UDS listener
    bool init(const std::string& socketPath);

    // Shutdown and clean up
    void shutdown();

    // Register a command handler
    void registerCommand(const std::string& name, CommandHandler handler);

    // Accept a new client connection, returns client fd or -1
    int acceptClient();

    // Process data from a client fd
    // Returns true if response was sent (client can be closed)
    bool processClient(int clientFd);

    int listenerFd() const { return m_listenerFd; }
    const std::string& socketPath() const { return m_socketPath; }

    // Track connected clients
    std::vector<int>& clients() { return m_clients; }
    void removeClient(int fd);

private:
    nlohmann::json dispatch(const nlohmann::json& request);

    int m_listenerFd = -1;
    std::string m_socketPath;
    std::map<std::string, CommandHandler> m_handlers;
    std::vector<int> m_clients;
    std::map<int, std::string> m_clientBuffers; // partial reads
};
