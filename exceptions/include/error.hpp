#ifndef ERROR_HPP
#define ERROR_HPP

#include <stdexcept>
#include <string>

// Base class for project-specific errors
class ProjectError : public std::runtime_error {
public:
    explicit ProjectError(const std::string& message)
        : std::runtime_error(message) {}
};

class IOError : public ProjectError {
public:
    explicit IOError(const std::string& message)
        : ProjectError("IOError: " + message) {}
};

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

class ConnectionError : public ProjectError {
public:
    explicit ConnectionError(const std::string& message)
        : ProjectError("ConnectionError: " + message) {}
};

class QueryError : public ProjectError {
public:
    explicit QueryError(const std::string& message)
        : ProjectError("QueryError: " + message) {}
};

#endif // ERROR_HPP