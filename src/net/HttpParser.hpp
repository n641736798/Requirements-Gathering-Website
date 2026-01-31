#pragma once

#include <string>
#include <unordered_map>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string query;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

class HttpParser {
public:
    static bool parseRequest(const std::string& raw, HttpRequest& req);
    static std::string buildResponse(int statusCode, const std::string& body, 
                                      const std::string& contentType = "application/json");
};
