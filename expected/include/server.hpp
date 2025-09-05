#ifndef SERVER_HPP
#define SERVER_HPP

#include "error.hpp"
#include <string>
#include <unordered_map>
#include <mutex>

struct QueryResult;
struct Query;

class Server {
public:
    QueryResult processCommand(const Query& query, int depth);

private:
    std::unordered_map<std::string, std::string> keyValueStore;
    std::mutex storeMutex;
};

#endif // SERVER_HPP