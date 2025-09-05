#ifndef QUERY_HPP
#define QUERY_HPP

#include "connection.hpp" 
#include "error.hpp"     
#include <string>
#include <vector>
#include <future>       
#include <memory>         
#include <chrono>
#include <optional>

// Represents a single query to be executed
struct Query {
    int id;
    enum class Type { GET, SET, DELETE };

    Type type;
    std::string rawCommand;
    std::string key;
    std::optional<std::string> value;
};

// Represents the result of a single query
struct QueryResult {
    int queryId;
    bool success;
    std::string data;         // Result data if successful
    std::string errorMessage; // Error message if failed
    std::chrono::milliseconds executionTime;

    void print() const;
};


class QueryResource {
public:
    QueryResource(int queryId) : qId(queryId), resourceHandle(++next_handle) {}

    QueryResource(const QueryResource&) = delete;
    QueryResource& operator=(const QueryResource&) = delete;

    int getHandle() const { return resourceHandle; }

private:
    int qId;
    int resourceHandle;
    static int next_handle;
};


class QueryEngine {
public:
    // ConnectionManager is passed by reference as it's managed externally
    explicit QueryEngine(ConnectionManager& connManager) : connectionManager(connManager) {}

    std::vector<Query> parseQueriesFromFile(const std::string& filePath);
    std::vector<QueryResult> executeQueries(const std::vector<Query>& queries, int depth);

private:
    QueryResult executeSingleQuery(const Query& query, int depth);

    ConnectionManager& connectionManager;
};

#endif // QUERY_HPP