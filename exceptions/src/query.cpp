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

QueryResult QueryEngine::executeSingleQuery(const Query& query, int depth) {
    auto startTime = std::chrono::high_resolution_clock::now();
    QueryResult result;
    result.queryId = query.id;
    result = connectionManager.executeRemoteQuery(query, depth);
    auto endTime = std::chrono::high_resolution_clock::now();
    if (result.success)
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
        std::string id_token, command_token;
        Query q;
        try {
            if (!std::getline(ss, id_token, ',')) 
                throw ParseError("Missing ID!");
            q.id = std::stoi(id_token);
            if (!std::getline(ss, command_token)) 
                throw ParseError("Missing command!");
            q.rawCommand = command_token;
            std::stringstream command_ss(q.rawCommand);
            std::string type_str;
            command_ss >> type_str;
            if (type_str == "GET") {
                q.type = Query::Type::GET;
                command_ss >> q.key;
            }
            else if (type_str == "SET") {
                q.type = Query::Type::SET;
                std::string pair;
                command_ss >> pair;
                size_t eq_pos = pair.find('=');
                if (eq_pos == std::string::npos) 
                    throw ParseError("Malformed SET");
                q.key = pair.substr(0, eq_pos);
                q.value = pair.substr(eq_pos + 1);
            }
            else if (type_str == "DELETE") {
                q.type = Query::Type::DELETE;
                command_ss >> q.key;
            }
            else {
				throw ParseError("Invalid command type");
            }
            if (q.key.empty()) 
                throw ParseError("Missing key");
            queries.push_back(q);
        }
        catch (const ParseError& err) {
            std::cerr << "Skipping malformed line " << lineNumber << ": " << err.what() << std::endl;
        }
    }
    return queries;
}


std::vector<QueryResult> QueryEngine::executeQueries(const std::vector<Query>& queries, int depth) {
    if (queries.empty()) {
        return {};
    }

    std::vector<std::future<QueryResult>> futures;
    std::vector<QueryResult> results;

    // Launch queries asynchronously
    for (const auto& query : queries) {
        futures.push_back(std::async(std::launch::async, [this, query, depth]() { 
            return executeSingleQuery(query, depth);
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