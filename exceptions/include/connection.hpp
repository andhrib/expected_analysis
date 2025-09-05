#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "config.hpp" 
#include "server.hpp"
#include <string>
#include <memory> 
#include <mutex> 


class NetworkResource {
public:
    NetworkResource(const std::string& serverAddress) : address(serverAddress), handle(next_available_handle++) {}

    ~NetworkResource() {
		handle = -1;
    }

    // Make it non-copyable, but movable
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
    static int next_available_handle; // Static counter for unique handles
};


// Represents the connection state and type
enum class ConnectionMode {
    DISCONNECTED,
    PRIMARY,
    BACKUP,
};

class ConnectionManager {
public:
    explicit ConnectionManager(const AppConfig& appConfig, Server& svr) : config(appConfig), server(svr), currentMode(ConnectionMode::DISCONNECTED) {}

    // Attempts to establish a connection, trying primary then backup, with retries.
    // Falls back to offline cache mode if all attempts fail.
    void establishConnection();

    bool isConnected() const;
    ConnectionMode getCurrentMode() const;
    std::string getCurrentServerAddress() const;

    QueryResult executeRemoteQuery(const Query& query, int depth);

    // Simulate different failure modes for testing
    static void setSimulatedFailureMode(const std::string& serverType, int failureCount = 0, bool transient = false);


private:
    // Attempts to connect to a specific server (primary or backup)
    // Throws ConnectionError on failure.
    // Returns a NetworkResource on success.
    std::unique_ptr<NetworkResource> connectToServer(const std::string& address, int port, int attemptNumber, int totalRetries);

	// Attempts to connect to a server with retries and exponential backoff
    std::unique_ptr<NetworkResource> connectToServerWithRetries(const std::string& address, int port, int maxRetries, int baseDelayMs, const std::string& serverType);

    const AppConfig& config;
    ConnectionMode currentMode;
    std::unique_ptr<NetworkResource> activeConnection;
    Server& server;

    // Simulation parameters (for testing)
    struct FailureSimConfig {
		int failureCount = 0;
        bool isTransient = false;
    };
    static FailureSimConfig primarySim;
    static FailureSimConfig backupSim;
};

#endif // CONNECTION_HPP