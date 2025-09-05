#include "server.hpp"
#include "query.hpp"

QueryResult Server::processCommand(const Query& query, int depth) {
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
				result.result = it->second;
            }
            else {
                result.result = std::unexpected(ErrorInfo{
                    ErrorCode::QueryExecutionError,
                    "Key not found for GET: '" + query.key + "'"
					});
            }
            break;
        }
        case Query::Type::SET: {
            keyValueStore[query.key] = query.value.value_or("");
			result.result = "SET successful for key '" + query.key + "'";
            break;
        }
        case Query::Type::DELETE: {
            if (keyValueStore.erase(query.key) > 0) {
				result.result = "DELETE successful for key '" + query.key + "'";
            }
            else {
                result.result = std::unexpected(ErrorInfo{
                    ErrorCode::QueryExecutionError,
					"Key not found for DELETE: '" + query.key + "'"
				});
            }
            break;
        }
    }
    return result;
}