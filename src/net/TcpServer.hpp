#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <mutex>

#include "Connection.hpp"

class ThreadPool;

class TcpServer {
public:
    using RequestHandler = Connection::RequestHandler;
    
    TcpServer();
    ~TcpServer();
    
    bool listen(const std::string& host, int port);
    void setRequestHandler(RequestHandler handler);
    void setThreadPool(ThreadPool* threadPool);  // 设置线程池
    void run();
    void stop();
    
private:
    void setupEpoll();
    void handleAccept();
    void handleEvent(int fd, uint32_t events);
    void processRequest(int fd, const std::string& request);  // 处理请求（在线程池中执行）
    void triggerWrite(int fd);  // 触发写事件（线程安全）
    
private:
    int listenFd_;
    int epollFd_;
    std::mutex connectionsMtx_;  // 保护connections_的并发访问
    std::unordered_map<int, std::unique_ptr<Connection>> connections_;
    RequestHandler requestHandler_;
    ThreadPool* threadPool_;  // 线程池指针（不拥有所有权）
    std::atomic<bool> running_;
    
    static const int MAX_EVENTS = 10000;
};
