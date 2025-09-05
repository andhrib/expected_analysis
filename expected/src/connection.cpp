#include "connection.hpp"
#include "query.hpp"
#include <thread>   
#include <chrono>
#include <cmath>  
#include <random> 
#include <iostream>

// Initialize static members
int NetworkResource::next_available_handle = 1;
ConnectionManager::FailureSimConfig ConnectionManager::primarySim;
ConnectionManager::FailureSimConfig ConnectionManager::backupSim;

NetworkResource::NetworkResource(NetworkResource&& other) noexcept : address(std::move(other.address)), handle(other.handle) {
    other.handle = -1;
}

NetworkResource& NetworkResource::operator=(NetworkResource&& other) noexcept {
    if (this != &other) {
        address = std::move(other.address);
        handle = other.handle;
        other.handle = -1;
    }
    return *this;
}

void ConnectionManager::setSimulatedFailureMode(const std::string& serverType, int failureCount, bool transient) {
    if (serverType == "primary") {
        primarySim.failureCount = failureCount;
        primarySim.isTransient = transient;
    }
    else if (serverType == "backup") {
        backupSim.failureCount = failureCount;
    }
}

std::expected<std::unique_ptr<NetworkResource>, ErrorInfo> ConnectionManager::connectToServer(
    const std::string& address, int port, int attemptNumber, int totalRetries) {

    std::string serverTypeForLog = (address == config.primaryServerAddress && port == config.primaryServerPort) ? "primary" : "backup";
    FailureSimConfig& simConfig = (serverTypeForLog == "primary") ? primarySim : backupSim;

    // Simulate network delay
    std::this_thread::sleep_for(std::chrono::milliseconds(5 + (rand() % 10)));

    if (simConfig.failureCount > totalRetries) {
		std::string errorType = simConfig.isTransient ? "transient" : "permanent";
        return std::unexpected(ErrorInfo{
		    simConfig.isTransient ? ErrorCode::TransientConnectionFailure : ErrorCode::PermanentConnectionFailure,
            "Simulated " + errorType + " connection failure to " + serverTypeForLog +
            " server " + address + ":" + std::to_string(port)
            });
    }

    try {
        auto resource = std::make_unique<NetworkResource>(address + ":" + std::to_string(port));
        return resource;
    }
    catch (const std::bad_alloc& ba) { // Example of an exception that std::expected doesn't typically handle
        std::cerr << ("Failed to allocate memory for NetworkResource: " + std::string(ba.what())) << std::endl;
        throw;
    }
    catch (const std::exception& e) { // Catch any other potential exceptions from NetworkResource or unique_ptr construction
        return std::unexpected(ErrorInfo{
            ErrorCode::NetworkResourceAcquisitionFailed,
            "Failed to acquire network resource for " + address + ":" + std::to_string(port) + " - " + e.what() 
            });
    }
}

std::expected<void, ErrorInfo> ConnectionManager::establishConnection() {
    if (currentMode != ConnectionMode::DISCONNECTED) {
        return {}; 
    }

    // Initial state reset
    currentMode = ConnectionMode::DISCONNECTED;
    activeConnection.reset();
    const int baseDelayMs = 50;

    // Lambda to attempt primary connection with retries
    auto attemptPrimary = [&]() -> std::expected<void, ErrorInfo> {
        ErrorInfo lastPrimaryError; // Store the last error from primary attempts
        for (int i = 0; i <= config.connectionRetries; ++i) {
            auto primaryConnectResult = connectToServer(config.primaryServerAddress, config.primaryServerPort, i + 1, config.connectionRetries + 1);

            if (primaryConnectResult) {
                activeConnection = std::move(primaryConnectResult.value());
                currentMode = ConnectionMode::PRIMARY;
                return {}; // Successful connection to primary
            }

            // Primary connection attempt failed
            lastPrimaryError = primaryConnectResult.error();

            if (i < config.connectionRetries) { // If more retries are left
                if (lastPrimaryError.code == ErrorCode::TransientConnectionFailure) {
                    long long delay = static_cast<long long>(baseDelayMs * std::pow(2, i));
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    // Add jitter: +/- 20% of the delay
                    std::uniform_int_distribution<> distrib(-static_cast<int>(delay / 5), static_cast<int>(delay / 5));
                    delay += distrib(gen);

                    if (delay < baseDelayMs) delay = baseDelayMs;
                    const long long maxDelayMs = 1000;
                    if (delay > maxDelayMs) delay = maxDelayMs;

                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                }
                else {
                    break; // Don't retry non-transient errors, break to try backup
                }
            }

        }
        return std::unexpected(lastPrimaryError); // All primary attempts failed or a non-transient error occurred.
        };

    // Lambda to attempt backup connection
    // This is called by or_else, so it receives the error from the previous step (primary)
    auto attemptBackup = [&](const ErrorInfo& primaryError) -> std::expected<void, ErrorInfo> {
        return connectToServer(config.backupServerAddress, config.backupServerPort, 1, 1)
            .transform([this](std::unique_ptr<NetworkResource> conn) { // Executed on successful backup connection
                activeConnection = std::move(conn);
                currentMode = ConnectionMode::BACKUP;
                // The transform lambda for std::expected<void, E> should return void.
            })
            .transform_error([&](ErrorInfo backupError) { // Executed on failed backup connection
                return backupError; // Propagate the backup error
            });
        };

    // Lambda for fallback to offline cache mode
    // This is called by or_else if both primary and backup attempts fail.
    auto fallbackToOffline = [&](const ErrorInfo& lastConnectionError) -> std::expected<void, ErrorInfo> {
        currentMode = ConnectionMode::DISCONNECTED;
        activeConnection.reset(); // Ensure no active connection in offline mode
        return std::unexpected(ErrorInfo{
            ErrorCode::ConnectionFailed,
            "All primary and backup server connection attempts failed. Offline mode. Last error: " + lastConnectionError.message,
            lastConnectionError.lineNumber
            });
        };


    return attemptPrimary()
        .or_else(attemptBackup)
        .or_else(fallbackToOffline);
}

bool ConnectionManager::isConnected() const {
    return (currentMode == ConnectionMode::PRIMARY || currentMode == ConnectionMode::BACKUP) && activeConnection && activeConnection->isValid();
}

ConnectionMode ConnectionManager::getCurrentMode() const {
    return currentMode;
}

std::string ConnectionManager::getCurrentServerAddress() const { 
    if (activeConnection && activeConnection->isValid()) {
        return activeConnection->getAddress();
    }
    switch (currentMode) {
    case ConnectionMode::PRIMARY: return config.primaryServerAddress + ":" + std::to_string(config.primaryServerPort);
    case ConnectionMode::BACKUP:  return config.backupServerAddress + ":" + std::to_string(config.backupServerPort);
    case ConnectionMode::DISCONNECTED: return "Disconnected";
    default: return "Unknown";
    }
}

QueryResult ConnectionManager::executeRemoteQuery(const Query& query, int depth) {
    if (!isConnected()) {
        return QueryResult{
            query.id,
            std::unexpected(ErrorInfo{
                ErrorCode::NoActiveConnectionForQuery,
                "No active connection for executing query ID " + std::to_string(query.id)
            }),
            std::chrono::milliseconds(0)
        };
    }

    return server.processCommand(query, depth);
}