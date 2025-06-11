#ifndef QUERY_HPP
#define QUERY_HPP

#include "connection.hpp" 
#include "error.hpp"         
#include <string>
#include <vector>
#include <future>         
#include <memory>         
#include <chrono>         
#include <expected> 

// Represents a single query to be executed
struct Query {
    int id;
    std::string queryString;
    bool simulateSuccess = true;
    std::string simulatedErrorMsg = "Simulated query failure";
};

// Represents the result of a single query
struct QueryResult {
    int queryId;
	std::expected<std::string, ErrorInfo> result;
    std::chrono::milliseconds executionTime;

    void print() const;
};

// RAII wrapper for a simulated query-specific resource
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
    explicit QueryEngine(ConnectionManager& connManager) : connectionManager(connManager) {}

    std::vector<Query> parseQueriesFromFile(const std::string& filePath);

    // Executes a batch of queries, potentially in parallel
    // Returns a vector of QueryResult. Each QueryResult indicates success/failure.
    std::vector<QueryResult> executeQueries(const std::vector<Query>& queries);

private:
    // Executes a single query.
    // This function will internally handle std::expected from ConnectionManager
    // and populate QueryResult accordingly.
    QueryResult executeSingleQuery(const Query& query);

    ConnectionManager& connectionManager;
};

#endif // QUERY_HPP