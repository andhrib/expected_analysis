#include "connection.hpp"
#include "query.hpp"
#include "error.hpp"
#include <stdexcept> 
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

std::unique_ptr<NetworkResource> ConnectionManager::connectToServer(const std::string& address, int port, int attemptNumber, int totalRetries) {

    std::string serverTypeForLog = (address == config.primaryServerAddress && port == config.primaryServerPort) ? "primary" : "backup";
    FailureSimConfig& simConfig = (serverTypeForLog == "primary") ? primarySim : backupSim;

    // Simulate network delay
    std::this_thread::sleep_for(std::chrono::milliseconds(5 + (rand() % 10))); 

    if (simConfig.failureCount > totalRetries) {
        std::string errorType = simConfig.isTransient ? "transient" : "permanent";
        std::string errorMessage = "Simulated " + errorType + " connection failure to " + serverTypeForLog + " server " + address + ":" + std::to_string(port);

        throw ConnectionError(errorMessage + (simConfig.isTransient ? " (transient)" : " (permanent)"));
    }

    // Simulate successful connection and resource acquisition
    try {
        auto resource = std::make_unique<NetworkResource>(address + ":" + std::to_string(port));
        return resource;
    }
    catch (const std::bad_alloc& ba) {
        std::cerr << ("Failed to allocate memory for NetworkResource: " + std::string(ba.what())) << std::endl;
        throw;
    }
    catch (const std::exception& e) { // Catch potential exceptions from NetworkResource constructor
        throw ConnectionError("Failed to acquire network resource for " + address + ":" + std::to_string(port) + " - " + e.what());
    }
}

void ConnectionManager::establishConnection() {
    if (currentMode != ConnectionMode::DISCONNECTED) {
        return;
    }

    currentMode = ConnectionMode::DISCONNECTED; // Reset mode
    activeConnection.reset(); // Release any previous connection explicitly

    // Try Primary Server
    try {
        activeConnection = connectToServerWithRetries(
            config.primaryServerAddress,
            config.primaryServerPort,
            config.connectionRetries,
            50, 
            "PRIMARY"
        );
        currentMode = ConnectionMode::PRIMARY;
        return; // Success
    }
	catch (const ConnectionError& _) {
        // Try Backup Server if Primary failed
        try {
            activeConnection = connectToServerWithRetries(
                config.backupServerAddress,
                config.backupServerPort,
                0, 
                0, 
                "BACKUP"
            );
            currentMode = ConnectionMode::BACKUP;
            return; // Success
        }
        catch (const ConnectionError& _) {
            currentMode = ConnectionMode::DISCONNECTED;
            activeConnection.reset();
        }
	}


}

std::unique_ptr<NetworkResource> ConnectionManager::connectToServerWithRetries(const std::string& address, int port, int maxRetries, int baseDelayMs, const std::string& serverType) {
    for (int i = 0; i <= maxRetries; ++i) {
        try {
            std::unique_ptr<NetworkResource> connection = connectToServer(address, port, i + 1, maxRetries + 1);
            return connection; 
        }
        catch (const ConnectionError& e) {
            if (i < maxRetries) {
                if (std::string(e.what()).find("(transient)") != std::string::npos) {
                    long long delay = static_cast<long long>(baseDelayMs * std::pow(2, i));
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_int_distribution<> distrib(-delay / 5, delay / 5);
                    delay += distrib(gen);
                    if (delay < baseDelayMs) delay = baseDelayMs;
                    if (delay > 1000) delay = 1000;

                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                }
                else {
                    break; // Don't retry permanent errors
                }
            }
        }
        catch (const std::exception& e_std) {
            std::cerr << ("Unexpected standard exception during " + serverType + " connection attempt " + std::to_string(i + 1) + ": " + e_std.what());
            if (i == maxRetries) {
                std::cerr << ("All " + serverType + " attempts failed due to unexpected errors.") << std::endl;
            }
        }
    }

	// If we reach here, all attempts failed
	throw ConnectionError("Failed to connect to " + serverType + " server after " + std::to_string(maxRetries + 1) + " attempts.");
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
            false,
            "",
            "No active connection for executing query ID " + std::to_string(query.id),
            std::chrono::milliseconds(0)
        };
    }

    return server.processCommand(query, depth);
}