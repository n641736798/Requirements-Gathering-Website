#pragma once

#include <string>
#include <functional>
#include <memory>
#include <mutex>

class Connection {
public:
    using RequestHandler = std::function<void(const std::string& request, std::string& response)>;
    
    Connection(int fd);
    ~Connection();
    
    int fd() const { return fd_; }
    
    void setHandler(RequestHandler handler) { handler_ = handler; }
    
    void onReadable();
    void onWritable();
    
    bool isClosed() const { 
        std::lock_guard<std::mutex> lock(mtx_);
        return closed_; 
    }
    
    // 线程安全的方法：从readBuffer中提取完整请求
    std::string extractRequest();
    
    // 线程安全的方法：追加响应到writeBuffer
    void appendResponse(const std::string& response);
    
private:
    int fd_;
    mutable std::mutex mtx_;  // 保护以下成员
    std::string readBuffer_;
    std::string writeBuffer_;
    RequestHandler handler_;
    bool closed_;
    
    void close();
};
