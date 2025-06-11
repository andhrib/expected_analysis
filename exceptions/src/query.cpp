#include "query.hpp"
#include <thread>  
#include <numeric>  
#include <algorithm>
#include <sstream>
#include <iostream>
#include <fstream>

// Initialize static member for QueryResource
int QueryResource::next_handle = 0;

void QueryResult::print() const {
    if (success) {
        std::cout << ("Query ID " + std::to_string(queryId) + " executed successfully: " + data) << std::endl;
    }
    else {
        std::cerr << ("Query ID " + std::to_string(queryId) + " failed: " + errorMessage) << std::endl;
    }
}

QueryResult QueryEngine::executeSingleQuery(const Query& query) {
    QueryResource qResource(query.id); 
    auto startTime = std::chrono::high_resolution_clock::now();
    QueryResult result;
    result.queryId = query.id;

    try {
        // Simulate query execution work
        std::this_thread::sleep_for(std::chrono::milliseconds(5 + (rand() % 10)));

        if (!query.simulateSuccess) {
            throw QueryError(query.simulatedErrorMsg + " (Query ID: " + std::to_string(query.id) + ")");
        }

        // Simulate sending data via connection manager
        connectionManager.sendData("QUERY:" + query.queryString);


        result.success = true;
        result.data = "Result data for query ID " + std::to_string(query.id) + ": '" + query.queryString + "'";
    }
    catch (const QueryError& qe) {
        result.success = false;
        result.errorMessage = qe.what();
    }
    catch (const ConnectionError& ce) {
        result.success = false;
        result.errorMessage = "ConnectionError during query ID " + std::to_string(query.id) + ": " + ce.what();
    }
    catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = "Unexpected error during query ID " + std::to_string(query.id) + ": " + e.what();
    }

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

        try {
            // 1. Parse ID
            if (!std::getline(ss, token, ',')) {
                throw std::invalid_argument("Missing ID!");
            }
            q.id = std::stoi(token);

            // 2. Parse Query String
            if (!std::getline(ss, token, ',')) {
                throw std::invalid_argument("Missing query string!");
            }
            q.queryString = token;

            // 3. Parse Simulate Success flag
            if (!std::getline(ss, token, ',')) {
                throw std::invalid_argument("Missing simulate success flag!");
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
        catch (const std::invalid_argument& ia) {
            std::cerr << ("Skipping malformed line " + std::to_string(lineNumber) + ". " + ia.what()) << std::endl;
        }
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

    // Launch queries asynchronously
    for (const auto& query : queries) {
        // std::async ensures that exceptions thrown in executeSingleQuery
        // are captured in the std::future and rethrown when future.get() is called.
        futures.push_back(std::async(std::launch::async, [this, query]() { 
            return executeSingleQuery(query);
            }));
    }

    for (size_t i = 0; i < futures.size(); ++i) {
        try {
            results.push_back(futures[i].get());
        }
        catch (const std::exception& e) {
            QueryResult errorResult;
            errorResult.queryId = (i < queries.size()) ? queries[i].id : -1; 
            errorResult.success = false;
            errorResult.errorMessage = "Future resolution failed: " + std::string(e.what());
            errorResult.executionTime = std::chrono::milliseconds(0);
            results.push_back(errorResult);
        }
    }

    return results;
}