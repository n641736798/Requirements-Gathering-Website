#include "Connection.hpp"
#include "utils/Logger.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <cstring>
#include <algorithm>
#include <cctype>

Connection::Connection(int fd) : fd_(fd), closed_(false) {
}

Connection::~Connection() {
    close();
}

void Connection::close() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!closed_ && fd_ >= 0) {
        ::close(fd_);
        closed_ = true;
    }
}

void Connection::onReadable() {
    char buffer[4096];
    ssize_t n = recv(fd_, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        if (n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
            close();
        }
        return;
    }
    
    buffer[n] = '\0';
    {
        std::lock_guard<std::mutex> lock(mtx_);
        readBuffer_ += buffer;
    }
}

std::string Connection::extractRequest() {
    std::lock_guard<std::mutex> lock(mtx_);
    // 查找HTTP头部的结束标记
    size_t headerEnd = readBuffer_.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return "";
    }
    
    // 提取头部
    std::string header = readBuffer_.substr(0, headerEnd + 4);
    
    // 解析Content-Length头部（大小写不敏感）
    size_t contentLength = 0;
    std::string headerLower = header;
    std::transform(headerLower.begin(), headerLower.end(), headerLower.begin(), ::tolower);
    size_t contentLengthPos = headerLower.find("content-length:");
    if (contentLengthPos != std::string::npos) {
        size_t valueStart = contentLengthPos + 15; // "content-length:"的长度
        while (valueStart < header.size() && (header[valueStart] == ' ' || header[valueStart] == '\t')) {
            valueStart++;
        }
        size_t valueEnd = valueStart;
        while (valueEnd < header.size() && header[valueEnd] != '\r' && header[valueEnd] != '\n') {
            valueEnd++;
        }
        try {
            contentLength = std::stoull(header.substr(valueStart, valueEnd - valueStart));
        } catch (...) {
            // 如果解析失败，contentLength保持为0
        }
    }
    
    // 计算完整请求的大小：头部 + body
    size_t totalSize = headerEnd + 4 + contentLength;
    if (readBuffer_.size() < totalSize) {
        // 数据还没有接收完整，等待更多数据
        return "";
    }
    
    // 提取完整请求（头部 + body）
    std::string request = readBuffer_.substr(0, totalSize);
    readBuffer_.erase(0, totalSize);
    return request;
}

void Connection::appendResponse(const std::string& response) {
    std::lock_guard<std::mutex> lock(mtx_);
    writeBuffer_ += response;
}

void Connection::onWritable() {
    std::string dataToSend;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (writeBuffer_.empty()) return;
        dataToSend = writeBuffer_;
    }
    
    ssize_t n = send(fd_, dataToSend.c_str(), dataToSend.size(), 0);
    if (n < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            close();
        }
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(mtx_);
        writeBuffer_.erase(0, n);
    }
}
