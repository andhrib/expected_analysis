// config.hpp
#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <expected>
#include "error.hpp"

// Structure to hold configuration parameters
struct AppConfig {
    std::string primaryServerAddress;
    int primaryServerPort;
    std::string backupServerAddress;
    int backupServerPort;
    int connectionRetries;
    int connectionTimeoutMs;
    // Default values
    AppConfig() : primaryServerPort(0), backupServerPort(0), connectionRetries(3), connectionTimeoutMs(5000) {}
};

class ConfigLoader {
public:
    // Loads, parses, and validates the configuration from the given file path.
    static std::expected<AppConfig, ErrorInfo> loadConfig(const std::string& filePath);

private:
    // Parses a single line of the config file.
    static std::expected<std::pair<std::string, std::string>, ErrorInfo> parseLine(const std::string& line, int lineNumber);

    // Validates the parsed key-value pairs and populates AppConfig.
    static std::expected<void, ErrorInfo> validateAndApply(AppConfig& config, const std::unordered_map<std::string, std::string>& rawConfig);
};

#endif // CONFIG_HPP