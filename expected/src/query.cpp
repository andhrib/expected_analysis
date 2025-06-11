#include "query.hpp"
#include <thread>   
#include <numeric> 
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

// Initialize static member for QueryResource
int QueryResource::next_handle = 0;

void QueryResult::print() const {
    if (result) {
        std::cout << ("Query ID " + std::to_string(queryId) + " executed successfully: " + result.value()) << std::endl;
    }
    else {
        std::cerr << ("Query ID " + std::to_string(queryId) + " failed: " + result.error().message) << std::endl;
    }
}

// Helper function for executeSingleQuery to structure error handling
// Returns a success indicator (e.g., data string) or an error.
static std::expected<std::string, ErrorInfo> performQueryWork(ConnectionManager& connManager, const Query& query) {
    // Simulate query execution work
    std::this_thread::sleep_for(std::chrono::milliseconds(5 + (rand() % 10)));

    if (!query.simulateSuccess) {
        return std::unexpected(ErrorInfo{
            ErrorCode::SimulatedQueryFailure,
            query.simulatedErrorMsg + " (Query ID: " + std::to_string(query.id) });
    }

    // Simulate sending data via connection manager
    auto sendDataResult = connManager.sendData("QUERY:" + query.queryString);
    if (!sendDataResult) {
        // Propagate the error from sendData, possibly mapping it to a Query-specific error context
        ErrorInfo& originalError = sendDataResult.error();
        return std::unexpected(ErrorInfo{
            ErrorCode::ConnectionErrorDuringQuery,
            "ConnectionError during query ID " + std::to_string(query.id) + ": " + originalError.message,
            originalError.lineNumber
            });
    }

    return "Result data for query ID " + std::to_string(query.id) + ": '" + query.queryString + "'";
}


QueryResult QueryEngine::executeSingleQuery(const Query& query) {
    QueryResource qResource(query.id);

    auto startTime = std::chrono::high_resolution_clock::now();
    QueryResult result;
    result.queryId = query.id;
    result.result = performQueryWork(connectionManager, query);
    auto endTime = std::chrono::high_resolution_clock::now();
    result.executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    return result;
}

std::vector<Query> QueryEngine::parseQueriesFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Error: Could not open file " + filePath);
    }

    std::vector<Query> queries;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        lineNumber++;
        std::stringstream ss(line);
        std::string token;
        Query q;

        // 1. Parse ID
        if (!std::getline(ss, token, ',')) {
            std::cerr << ("Skipping malformed line " + std::to_string(lineNumber) + ". Missing ID!");
            continue;
        }
        q.id = std::stoi(token);

        // 2. Parse Query String
        if (!std::getline(ss, token, ',')) {
            std::cerr << ("Skipping malformed line " + std::to_string(lineNumber) + ". Missing query string!");
            continue;
        }
        q.queryString = token;

        // 3. Parse Simulate Success flag
        if (!std::getline(ss, token, ',')) {
            std::cerr << ("Skipping malformed line " + std::to_string(lineNumber) + ". Missing simulate success flag!");
            continue;
        }
        q.simulateSuccess = token == "true";

        // 4. Parse optional Simulated Error Message
        if (std::getline(ss, token)) {
            if (!q.simulateSuccess && !token.empty()) {
                q.simulatedErrorMsg = token;
            }
        }

        queries.push_back(q);
    }

    file.close();
    return queries;
}

std::vector<QueryResult> QueryEngine::executeQueries(const std::vector<Query>& queries) {
    if (queries.empty()) {
        return {};
    }
    std::vector<std::future<QueryResult>> futures;
    std::vector<QueryResult> results;

    for (const auto& query : queries) {
        futures.push_back(std::async(std::launch::async, [this, query]() {
            return executeSingleQuery(query);
            // If executeSingleQuery itself could throw non-std::expected critical errors (e.g. bad_alloc),
            // they would be caught by future.get()
            }));
    }

    for (size_t i = 0; i < futures.size(); ++i) {
        try {
            results.push_back(futures[i].get());
        }
        catch (const std::exception& e) {
            // This catch block is now primarily for unexpected exceptions not converted to ErrorInfo
            // by executeSingleQuery's internal logic (e.g., std::bad_alloc from async infrastructure or deeper).
            // Recoverable errors handled by std::expected within performQueryWork should already be in QueryResult.
            QueryResult errorResult;
            errorResult.queryId = (i < queries.size()) ? queries[i].id : -1;
			errorResult.result = std::unexpected(ErrorInfo{
				ErrorCode::UnknownError,
				"Future resolution failed due to unexpected exception: " + std::string(e.what()),
				0 // No specific line number for this error
				});
            errorResult.executionTime = std::chrono::milliseconds(0);
            results.push_back(errorResult);
        }
    }

    return results;
}