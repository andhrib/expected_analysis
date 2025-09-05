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

QueryResult QueryEngine::executeSingleQuery(const Query& query, int depth) {
    QueryResource qResource(query.id);

    auto startTime = std::chrono::high_resolution_clock::now();
    QueryResult result = connectionManager.executeRemoteQuery(query, depth);
    auto endTime = std::chrono::high_resolution_clock::now();
    if (result.result)
        result.executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    return result;
}

std::vector<Query> QueryEngine::parseQueriesFromFile(const std::string& filePath) {
    auto parseLine = [](const std::string& line, int lineNumber) -> std::expected<Query, ErrorInfo> {
        std::stringstream ss(line);
        std::string id_token, command_token;
        Query q;
        if (!std::getline(ss, id_token, ','))
			return std::unexpected(ErrorInfo{ ErrorCode::ParseError, "Missing ID!", lineNumber });
        q.id = std::stoi(id_token);
        if (!std::getline(ss, command_token))
			return std::unexpected(ErrorInfo{ ErrorCode::ParseError, "Missing command!", lineNumber });
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
				return std::unexpected(ErrorInfo{ ErrorCode::ParseError, "Malformed SET", lineNumber });
            q.key = pair.substr(0, eq_pos);
            q.value = pair.substr(eq_pos + 1);
        }
        else if (type_str == "DELETE") {
            q.type = Query::Type::DELETE;
            command_ss >> q.key;
        }
        else {
			return std::unexpected(ErrorInfo{ ErrorCode::ParseError, "Invalid command type", lineNumber });
        }
        if (q.key.empty())
			return std::unexpected(ErrorInfo{ ErrorCode::ParseError, "Missing key", lineNumber });
        return q;
	}; 

    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Error: Could not open file " + filePath);
    }
    std::vector<Query> queries;
    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
		auto result = parseLine(line, ++lineNumber);
        if (result) {
            queries.push_back(result.value());
        }
        else {
            ErrorInfo err = result.error();
            std::cerr << "Skipping malformed line " << lineNumber << ": " << err.fullMessage() << std::endl;
        }
    }
    return queries;
}

std::vector<QueryResult> QueryEngine::executeQueries(const std::vector<Query>&queries, int depth) {
    if (queries.empty()) {
        return {};
    }
    std::vector<std::future<QueryResult>> futures;
    std::vector<QueryResult> results;

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
			errorResult.result = std::unexpected(ErrorInfo{
				ErrorCode::UnknownError,
				"Future resolution failed due to unexpected exception: " + std::string(e.what()),
				-1 // No specific line number for this error
				});
            errorResult.executionTime = std::chrono::milliseconds(0);
            results.push_back(errorResult);
        }
    }

    return results;
}