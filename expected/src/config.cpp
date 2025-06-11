#include "config.hpp"
#include "utility.hpp"
#include <fstream>
#include <sstream>
#include <algorithm> 

std::expected<AppConfig, ErrorInfo> ConfigLoader::loadConfig(const std::string& filePath) {
    std::ifstream configFile(filePath);
    if (!configFile.is_open()) {
        return std::unexpected(ErrorInfo{
            ErrorCode::FileOpenFailed,
            "Failed to open configuration file: " + filePath });
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

        auto parseResult = parseLine(line, lineNumber);
        if (!parseResult) {
            // Log and return the error
            return std::unexpected(parseResult.error());
        }
        rawConfig[parseResult.value().first] = parseResult.value().second;
    }

    auto validationResult = validateAndApply(config, rawConfig);
    if (!validationResult) {
        return std::unexpected(validationResult.error());
    }

    return config;
}

std::expected<std::pair<std::string, std::string>, ErrorInfo> ConfigLoader::parseLine(const std::string& line, int lineNumber) {
    size_t delimiterPos = line.find('=');
    if (delimiterPos == std::string::npos) {
        return std::unexpected(ErrorInfo{
            ErrorCode::MissingConfigDelimiter,
            "Malformed line: Missing '=' delimiter. Line: '" + line + "'",
            lineNumber });
    }

    std::string key = trim(line.substr(0, delimiterPos));
    std::string value = trim(line.substr(delimiterPos + 1));

    if (key.empty()) {
        return std::unexpected(ErrorInfo{
            ErrorCode::EmptyConfigKey,
            "Malformed line: Key is empty. Line: '" + line + "'",
            lineNumber });
    }
    // Value can be empty, validation for that should be in validateAndApply

    return std::make_pair(std::move(key), std::move(value));
}

std::expected<void, ErrorInfo> ConfigLoader::validateAndApply(AppConfig& config, const std::unordered_map<std::string, std::string>& rawConfig) {
    // Helper lambda to get value or return ErrorInfo
    auto getValue = [&](const std::string& key) -> std::expected<std::string, ErrorInfo> {
        auto it = rawConfig.find(key);
        if (it == rawConfig.end() || it->second.empty()) {
            return std::unexpected(ErrorInfo{
                ErrorCode::MissingRequiredParameter,
                "Missing or empty required parameter: " + key });
        }
        return it->second;
        };

    // Helper lambda for int conversion and validation
    auto getIntValue = [&](const std::string& key, int minValue = -1, int maxValue = -1) -> std::expected<int, ErrorInfo> {
        auto valStrExpected = getValue(key);
        if (!valStrExpected) {
            return std::unexpected(valStrExpected.error());
        }
        std::string valStr = valStrExpected.value();

        int valInt;
        try {
            valInt = std::stoi(valStr);
        }
        catch (const std::invalid_argument& ia) {
            return std::unexpected(ErrorInfo{
                ErrorCode::InvalidParameterValue,
                "Invalid integer value for parameter '" + key + "': " + valStr + ". " + ia.what() });
        }
        catch (const std::out_of_range& oor) {
            return std::unexpected(ErrorInfo{
                ErrorCode::ParameterValueOutOfRange,
                "Integer value out of range for parameter '" + key + "': " + valStr + ". " + oor.what() });
        }

        if (minValue != -1 && valInt < minValue) {
            return std::unexpected(ErrorInfo{
                ErrorCode::ParameterValueOutOfRange,
                "Parameter '" + key + "' value " + std::to_string(valInt) + " is less than minimum allowed " + std::to_string(minValue) });
        }
        if (maxValue != -1 && valInt > maxValue) {
            return std::unexpected(ErrorInfo{
                ErrorCode::ParameterValueOutOfRange,
                "Parameter '" + key + "' value " + std::to_string(valInt) + " is greater than maximum allowed " + std::to_string(maxValue) });
        }
        return valInt;
        };

    // Required parameters
    ASSIGN_OR_RETURN_ERROR(config.primaryServerAddress, getValue("primary_server_address"));
    ASSIGN_OR_RETURN_ERROR(config.primaryServerPort, getIntValue("primary_server_port", 1, 65535));

    ASSIGN_OR_RETURN_ERROR(config.backupServerAddress, getValue("backup_server_address"));
    ASSIGN_OR_RETURN_ERROR(config.backupServerPort, getIntValue("backup_server_port", 1, 65535));

    // Optional parameters
    if (rawConfig.count("connection_retries")) {
        ASSIGN_OR_RETURN_ERROR(config.connectionRetries, getIntValue("connection_retries", 0, 100));
    }

    if (rawConfig.count("connection_timeout_ms")) {
        ASSIGN_OR_RETURN_ERROR(config.connectionTimeoutMs, getIntValue("connection_timeout_ms", 100, 60000));
    }

    // Custom semantic validation
    if (config.primaryServerAddress == config.backupServerAddress && config.primaryServerPort == config.backupServerPort) {
        return std::unexpected(ErrorInfo{
            ErrorCode::IdenticalPrimaryBackupServers,
            "Primary and backup server addresses and ports cannot be identical." });
    }

    return {}; // Represents void success
}