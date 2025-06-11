#ifndef UTILITY_HPP
#define UTILITY_HPP

#define ASSIGN_OR_RETURN_ERROR(lhs, expr) \
    do { \
        auto _res = (expr); \
        if (!_res) return std::unexpected(_res.error()); \
        lhs = *_res; \
    } while (false);

// Helper function to trim whitespace from both ends of a string
std::string trim(const std::string & str) {
    const static std::string whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return ""; // Return empty string if only whitespace
    }
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, (end - start + 1));
}

#endif