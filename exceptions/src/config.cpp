#include "config.hpp"
#include "error.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

// Helper function to trim whitespace from both ends of a string
std::string trim(const std::string& str) {
    const static std::string whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return ""; // Return empty string if only whitespace
    }
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, (end - start + 1));
}

AppConfig ConfigLoader::loadConfig(const std::string& filePath) {
    std::ifstream configFile(filePath);
    if (!configFile.is_open()) {
        throw IOError("Failed to open configuration file: " + filePath);
    }

    AppConfig config;
    std::unordered_map<std::string, std::string> rawConfig;
    std::string line;
    int lineNumber = 0;

    while (std::getline(configFile, line)) {
        lineNumber++;
        line = trim(line);
        if (line.empty() || line[0] == '#') { // Skip empty lines and comments
            continue;
        }

        auto pair = parseLine(line, lineNumber);
        rawConfig[pair.first] = pair.second;
    }

    validateAndApply(config, rawConfig);
    return config;
}

std::pair<std::string, std::string> ConfigLoader::parseLine(const std::string& line, int lineNumber) {
    size_t delimiterPos = line.find('=');

    if (delimiterPos == std::string::npos) {
        throw ParseError("Malformed line " + std::to_string(lineNumber) + ": Missing '=' delimiter. Line: '" + line + "'");
    }

    std::string key = trim(line.substr(0, delimiterPos));
    std::string value = trim(line.substr(delimiterPos + 1));

    if (key.empty()) {
        throw ParseError("Malformed line " + std::to_string(lineNumber) + ": Key is empty. Line: '" + line + "'");
    }

    return { std::move(key), std::move(value) };
}

void ConfigLoader::validateAndApply(AppConfig& config, const std::unordered_map<std::string, std::string>& rawConfig) {

    auto getValue = [&rawConfig](const std::string& key) { // Helper to get value or throw if missing
        if (rawConfig.find(key) == rawConfig.end() || rawConfig.at(key).empty()) {
            throw ValidationError("Missing or empty required parameter: " + key);
        }
        return rawConfig.at(key);
        };

    auto getIntValue = [&getValue](const std::string& key, int minValue = -1, int maxValue = -1) { // Helper for int conversion and validation
        std::string valStr = getValue(key);
        int valInt;
        try {
            valInt = std::stoi(valStr);
        }
        catch (const std::invalid_argument& ia) {
            throw ValidationError("Invalid integer value for parameter '" + key + "': " + valStr + ". " + ia.what());
        }
        catch (const std::out_of_range& oor) {
            throw ValidationError("Integer value out of range for parameter '" + key + "': " + valStr + ". " + oor.what());
        }
        if (minValue != -1 && valInt < minValue) {
            throw ValidationError("Parameter '" + key + "' value " + std::to_string(valInt) + " is less than minimum allowed " + std::to_string(minValue));
        }
        if (maxValue != -1 && valInt > maxValue) {
            throw ValidationError("Parameter '" + key + "' value " + std::to_string(valInt) + " is greater than maximum allowed " + std::to_string(maxValue));
        }
        return valInt;
        };

    config.primaryServerAddress = getValue("primary_server_address");
    config.primaryServerPort = getIntValue("primary_server_port", 1, 65535);

    config.backupServerAddress = getValue("backup_server_address");
    config.backupServerPort = getIntValue("backup_server_port", 1, 65535);

    if (rawConfig.count("connection_retries")) {
        config.connectionRetries = getIntValue("connection_retries", 0, 100);
    }

    if (rawConfig.count("connection_timeout_ms")) {
        config.connectionTimeoutMs = getIntValue("connection_timeout_ms", 100, 60000);
    }

    if (config.primaryServerAddress == config.backupServerAddress && config.primaryServerPort == config.backupServerPort) {
        throw ValidationError("Primary and backup server addresses and ports cannot be identical.");
    }
}