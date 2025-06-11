#ifndef QUERY_HPP
#define QUERY_HPP

#include "connection.hpp" // For ConnectionManager
#include "error.hpp"      // For QueryError
#include <string>
#include <vector>
#include <future>       
#include <memory>         
#include <chrono>        

// Represents a single query to be executed
struct Query {
    int id;
    std::string queryString;
    // Add other query-specific parameters if needed

    // For testing: instruct this query to succeed or fail
    bool simulateSuccess = true;
    std::string simulatedErrorMsg = "Simulated query failure";
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

    // Executes a batch of queries, potentially in parallel
    std::vector<QueryResult> executeQueries(const std::vector<Query>& queries);

private:
    // Executes a single query
    // This will be called by the parallel execution logic
    QueryResult executeSingleQuery(const Query& query);

    ConnectionManager& connectionManager;
};

#endif // QUERY_HPP