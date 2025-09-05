// connection.hpp
#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "config.hpp" 
#include "error.hpp" 
#include "server.hpp"
#include <string>
#include <memory>        
#include <expected>  


class NetworkResource {
public:
    NetworkResource(const std::string& serverAddress) : address(serverAddress), handle(next_available_handle++) {}

    ~NetworkResource() {
        handle = -1;
    }

    NetworkResource(const NetworkResource&) = delete;
    NetworkResource& operator=(const NetworkResource&) = delete;
    NetworkResource(NetworkResource&& other) noexcept;
    NetworkResource& operator=(NetworkResource&& other) noexcept;

    bool isValid() const { return handle != -1; }
    std::string getAddress() const { return address; }
    int getHandle() const { return handle; }

private:
    std::string address;
    int handle;
    static int next_available_handle;
};


enum class ConnectionMode {
    DISCONNECTED,
    PRIMARY,
    BACKUP,
};

class ConnectionManager {
public:
    explicit ConnectionManager(const AppConfig& appConfig, Server& svr) : config(appConfig), server(svr), currentMode(ConnectionMode::DISCONNECTED) {}

    // Attempts to establish a connection. Returns void on success, ErrorInfo if it ends in OFFLINE_CACHE or DISCONNECTED after all attempts.
    // Note: The internal state (currentMode) reflects the outcome. This function's error primarily signals failure to get *any* server.
    std::expected<void, ErrorInfo> establishConnection();

    bool isConnected() const;
    ConnectionMode getCurrentMode() const;
    std::string getCurrentServerAddress() const;

    QueryResult executeRemoteQuery(const Query& query, int depth);

    static void setSimulatedFailureMode(const std::string& serverType, int failureCount = 0, bool transient = false);

private:
    // Attempts to connect to a specific server.
    // Returns a NetworkResource on success, or ErrorInfo on failure.
    std::expected<std::unique_ptr<NetworkResource>, ErrorInfo> connectToServer(
        const std::string& address, int port, int attemptNumber, int totalRetries);

    AppConfig config;
    ConnectionMode currentMode;
    std::unique_ptr<NetworkResource> activeConnection;
    Server& server;

    struct FailureSimConfig {
        int failureCount = 0;
        bool isTransient = false;
    };
    static FailureSimConfig primarySim;
    static FailureSimConfig backupSim;
};

#endif // CONNECTION_HPP