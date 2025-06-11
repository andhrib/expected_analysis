#ifndef ERROR_HPP
#define ERROR_HPP

#include <string>
#include <stdexcept>

// Enum to define specific error codes
enum class ErrorCode {
    // Configuration Errors
    FileOpenFailed,
    ParseError,
    MissingConfigDelimiter,
    EmptyConfigKey,
    MissingRequiredParameter,
    InvalidParameterValue,
    ParameterValueOutOfRange,
    IdenticalPrimaryBackupServers,

    // Connection Errors
    ConnectionFailed,       
    NetworkResourceAcquisitionFailed,
    NotConnected,           
    TransientConnectionFailure,
    PermanentConnectionFailure,

    // Query Errors
    QueryExecutionError,   
    SimulatedQueryFailure,
    ConnectionErrorDuringQuery,
    NoActiveConnectionForQuery,

    // General/Unknown
    UnknownError
};

// Structure to hold error details
struct ErrorInfo {
    ErrorCode code;
    std::string message;
    int lineNumber = 0; // Optional: for parsing errors

    // Helper to convert ErrorCode to string for logging
    static std::string codeToString(ErrorCode c) {
        switch (c) {
        case ErrorCode::FileOpenFailed: return "FileOpenFailed";
        case ErrorCode::ParseError: return "ParseError";
        case ErrorCode::MissingConfigDelimiter: return "MissingConfigDelimiter";
        case ErrorCode::EmptyConfigKey: return "EmptyConfigKey";
        case ErrorCode::MissingRequiredParameter: return "MissingRequiredParameter";
        case ErrorCode::InvalidParameterValue: return "InvalidParameterValue";
        case ErrorCode::ParameterValueOutOfRange: return "ParameterValueOutOfRange";
        case ErrorCode::IdenticalPrimaryBackupServers: return "IdenticalPrimaryBackupServers";
        case ErrorCode::ConnectionFailed: return "ConnectionFailed";
        case ErrorCode::NetworkResourceAcquisitionFailed: return "NetworkResourceAcquisitionFailed";
        case ErrorCode::NotConnected: return "NotConnected";
        case ErrorCode::TransientConnectionFailure: return "TransientConnectionFailure";
		case ErrorCode::PermanentConnectionFailure: return "PermanentConnectionFailure";
        case ErrorCode::QueryExecutionError: return "QueryExecutionError";
        case ErrorCode::SimulatedQueryFailure: return "SimulatedQueryFailure";
        case ErrorCode::ConnectionErrorDuringQuery: return "ConnectionErrorDuringQuery";
        case ErrorCode::NoActiveConnectionForQuery: return "NoActiveConnectionForQuery";
        case ErrorCode::UnknownError: return "UnknownError";
        default: return "UnknownErrorCode";
        }
    }

    std::string fullMessage() const {
        std::string msg = "[" + codeToString(code) + "]";
        if (lineNumber > 0) {
            msg += " (Line " + std::to_string(lineNumber) + ")";
        }
        msg += ": " + message;
        return msg;
    }
};

// The original exception classes can be kept for reference or specific unrecoverable scenarios
// if needed, but the primary flow will use std::expected<T, ErrorInfo>.

// Base class for project-specific errors (can be kept for truly exceptional, non-std::expected cases)
class ProjectError : public std::runtime_error {
public:
    explicit ProjectError(const std::string& message)
        : std::runtime_error(message) {}
};
// Configuration related errors (Original definitions for reference)
/*
class ParseError : public ProjectError {
public:
    explicit ParseError(const std::string& message)
        : ProjectError("ParseError: " + message) {}
};
class ValidationError : public ProjectError {
public:
    explicit ValidationError(const std::string& message)
        : ProjectError("ValidationError: " + message) {}
};
*/
// Connection related errors (Original definition for reference)
/*
class ConnectionError : public ProjectError {
public:
    explicit ConnectionError(const std::string& message)
        : ProjectError("ConnectionError: " + message) {}
};
*/
// Query related errors (Original definition for reference)
/*
class QueryError : public ProjectError {
public:
    explicit QueryError(const std::string& message)
        : ProjectError("QueryError: " + message) {}
};
*/

#endif // ERROR_HPP