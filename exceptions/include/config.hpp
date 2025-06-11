#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <unordered_map> 

// Structure to hold configuration parameters
struct AppConfig {
    std::string primaryServerAddress;
    int primaryServerPort;
    std::string backupServerAddress;
    int backupServerPort;
    int connectionRetries;
    int connectionTimeoutMs;

    // Default values (optional, but can be useful)
    AppConfig() : primaryServerPort(0), backupServerPort(0), connectionRetries(3), connectionTimeoutMs(5000) {}
};

class ConfigLoader {
public:
    // Loads, parses, and validates the configuration from the given file path.
    // Throws std::runtime_error if file cannot be opened.
    // Throws ParseError for syntax issues in the file.
    // Throws ValidationError for semantic issues with parameter values.
    static AppConfig loadConfig(const std::string& filePath);

private:
    // Parses a single line of the config file.
    // Expected format: key = value
    // Throws ParseError if the line is malformed.
    static std::pair<std::string, std::string> parseLine(const std::string& line, int lineNumber);

    // Validates the parsed key-value pairs and populates AppConfig.
    // Throws ValidationError if required parameters are missing or values are invalid.
    static void validateAndApply(AppConfig& config, const std::unordered_map<std::string, std::string>& rawConfig);
};

#endif // CONFIG_HPP