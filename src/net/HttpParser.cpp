#include "HttpParser.hpp"
#include <sstream>
#include <algorithm>

bool HttpParser::parseRequest(const std::string& raw, HttpRequest& req) {
    std::istringstream iss(raw);
    std::string line;
    
    // 解析请求行
    if (!std::getline(iss, line)) return false;
    
    size_t pos1 = line.find(' ');
    if (pos1 == std::string::npos) return false;
    req.method = line.substr(0, pos1);
    
    size_t pos2 = line.find(' ', pos1 + 1);
    if (pos2 == std::string::npos) return false;
    
    std::string pathAndQuery = line.substr(pos1 + 1, pos2 - pos1 - 1);
    size_t queryPos = pathAndQuery.find('?');
    if (queryPos != std::string::npos) {
        req.path = pathAndQuery.substr(0, queryPos);
        req.query = pathAndQuery.substr(queryPos + 1);
    } else {
        req.path = pathAndQuery;
    }
    
    // 解析头部
    while (std::getline(iss, line) && line != "\r" && !line.empty()) {
        if (line.back() == '\r') line.pop_back();
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            // 去除前导空格
            while (!value.empty() && value[0] == ' ') {
                value = value.substr(1);
            }
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            req.headers[key] = value;
        }
    }
    
    // 读取 body
    if (req.headers.find("content-length") != req.headers.end()) {
        size_t contentLength = std::stoull(req.headers["content-length"]);
        req.body.resize(contentLength);
        iss.read(&req.body[0], contentLength);
    }
    
    return true;
}

std::string HttpParser::buildResponse(int statusCode, const std::string& body, 
                                      const std::string& contentType) {
    std::ostringstream oss;
    const char* statusText = "OK";
    if (statusCode == 400) statusText = "Bad Request";
    else if (statusCode == 404) statusText = "Not Found";
    else if (statusCode == 500) statusText = "Internal Server Error";
    
    oss << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    oss << "Content-Type: " << contentType << "; charset=utf-8\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Connection: keep-alive\r\n";
    oss << "\r\n";
    oss << body;
    
    return oss.str();
}
