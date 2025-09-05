#include "server.hpp"
#include "query.hpp"

QueryResult Server::processCommand(const Query& query, int depth) {
    QueryResult result;

    try {
		result = processWork(query, depth);
    }
    catch (const QueryError& qe) {
        result.success = false;
        result.errorMessage = qe.what();
    }
    catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = "Unexpected error: " + std::string(e.what());
	}
    
    return result;
}

QueryResult Server::processWork(const Query& query, int depth)
{
    if (depth > 0) {
        auto tmp = processCommand(query, depth - 1);
        static volatile int sink = 0;
        sink += 1;
        return tmp;
    }
    std::lock_guard<std::mutex> lock(storeMutex);

    QueryResult result;
    result.queryId = query.id;

    switch (query.type) {
    case Query::Type::GET: {
        if (auto it = keyValueStore.find(query.key); it != keyValueStore.end()) {
            result.success = true;
            result.data = "GET successful. Value: '" + it->second + "'";
        }
        else {
            throw QueryError("Key not found for GET: '" + query.key + "'");
        }
        break;
    }
    case Query::Type::SET: {
        keyValueStore[query.key] = query.value.value_or("");
        result.success = true;
        result.data = "SET successful for key '" + query.key + "'";
        break;
    }
    case Query::Type::DELETE: {
        if (keyValueStore.erase(query.key) > 0) {
            result.success = true;
            result.data = "DELETE successful for key '" + query.key + "'";
        }
        else {
            throw QueryError("Key not found for DELETE: '" + query.key + "'");
        }
        break;
    }
    }

	return result;
}

