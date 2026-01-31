#include "TcpServer.hpp"
#include "thread/ThreadPool.hpp"
#include "utils/Logger.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <cstring>
#include <errno.h>
#include <mutex>

TcpServer::TcpServer() : listenFd_(-1), epollFd_(-1), threadPool_(nullptr), running_(false) {
}

TcpServer::~TcpServer() {
    stop();
    if (epollFd_ >= 0) close(epollFd_);
    if (listenFd_ >= 0) close(listenFd_);
}

static int setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

bool TcpServer::listen(const std::string& host, int port) {
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        LOG_ERROR("Failed to create socket: " + std::string(strerror(errno)));
        return false;
    }
    
    int opt = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    if (setNonBlocking(listenFd_) < 0) {
        LOG_ERROR("Failed to set non-blocking: " + std::string(strerror(errno)));
        close(listenFd_);
        return false;
    }
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (host.empty() || host == "0.0.0.0") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    }
    
    if (bind(listenFd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("Failed to bind: " + std::string(strerror(errno)));
        close(listenFd_);
        return false;
    }
    
    if (::listen(listenFd_, 1024) < 0) {
        LOG_ERROR("Failed to listen: " + std::string(strerror(errno)));
        close(listenFd_);
        return false;
    }
    
    setupEpoll();
    
    LOG_INFO("Server listening on " + host + ":" + std::to_string(port));
    return true;
}

void TcpServer::setupEpoll() {
    epollFd_ = epoll_create1(0);
    if (epollFd_ < 0) {
        LOG_ERROR("Failed to create epoll: " + std::string(strerror(errno)));
        return;
    }
    
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listenFd_;
    epoll_ctl(epollFd_, EPOLL_CTL_ADD, listenFd_, &ev);
}

void TcpServer::setRequestHandler(RequestHandler handler) {
    requestHandler_ = handler;
}

void TcpServer::setThreadPool(ThreadPool* threadPool) {
    threadPool_ = threadPool;
}

void TcpServer::handleAccept() {
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);
        int clientFd = accept(listenFd_, (sockaddr*)&clientAddr, &len);
        if (clientFd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            LOG_ERROR("Failed to accept: " + std::string(strerror(errno)));
            break;
        }
        
        if (setNonBlocking(clientFd) < 0) {
            close(clientFd);
            continue;
        }
        
        auto conn = std::make_unique<Connection>(clientFd);
        conn->setHandler(requestHandler_);
        
        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
        ev.data.fd = clientFd;
        if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, clientFd, &ev) < 0) {
            close(clientFd);
            continue;
        }
        
        {
            std::lock_guard<std::mutex> lock(connectionsMtx_);
            connections_[clientFd] = std::move(conn);
        }
    }
}

void TcpServer::handleEvent(int fd, uint32_t events) {
    if (fd == listenFd_) {
        handleAccept();
        return;
    }
    
    std::lock_guard<std::mutex> lock(connectionsMtx_);
    auto it = connections_.find(fd);
    if (it == connections_.end()) return;
    auto& conn = it->second;
    
    if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        connections_.erase(it);
        return;
    }
    
    if (events & EPOLLIN) {
        conn->onReadable();
        
        // 检查是否有完整请求需要处理
        std::string request = conn->extractRequest();
        if (!request.empty() && requestHandler_) {
            // 如果有线程池，将业务处理提交到线程池
            if (threadPool_) {
                // 捕获fd和request的副本，在线程池中处理
                int clientFd = fd;
                threadPool_->submit([this, clientFd, request]() {
                    processRequest(clientFd, request);
                });
            } else {
                // 没有线程池，直接在当前线程处理（保持向后兼容）
                processRequest(fd, request);
            }
        }
        
        if (conn->isClosed()) {
            connections_.erase(it);
            return;
        }
    }
    
    if (events & EPOLLOUT) {
        conn->onWritable();
        if (conn->isClosed()) {
            connections_.erase(it);
            return;
        }
    }
}

void TcpServer::processRequest(int fd, const std::string& request) {
    Connection* conn = nullptr;
    {
        std::lock_guard<std::mutex> lock(connectionsMtx_);
        auto it = connections_.find(fd);
        if (it == connections_.end()) {
            // 连接已关闭
            return;
        }
        if (it->second->isClosed()) {
            return;
        }
        // 获取Connection指针（Connection本身是线程安全的）
        conn = it->second.get();
    }
    
    // 执行业务处理（在锁外执行，避免阻塞其他连接）
    std::string response;
    requestHandler_(request, response);
    
    // 将响应追加到连接的写缓冲区
    conn->appendResponse(response);
    
    // 触发EPOLLOUT事件以发送响应（在ET模式下需要手动触发）
    triggerWrite(fd);
}

void TcpServer::triggerWrite(int fd) {
    // 修改epoll事件以触发EPOLLOUT
    // 在ET模式下，通过修改事件可以强制触发一次
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
    ev.data.fd = fd;
    epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
}

void TcpServer::run() {
    running_ = true;
    epoll_event events[MAX_EVENTS];
    
    while (running_) {
        int nfds = epoll_wait(epollFd_, events, MAX_EVENTS, 100);
        if (nfds < 0) {
            if (errno == EINTR) continue;
            LOG_ERROR("epoll_wait failed: " + std::string(strerror(errno)));
            break;
        }
        
        for (int i = 0; i < nfds; ++i) {
            handleEvent(events[i].data.fd, events[i].events);
        }
    }
}

void TcpServer::stop() {
    running_ = false;
    
    // 如果有线程池，等待所有正在执行的任务完成
    // 这样可以确保正在处理请求的任务能够正常完成，不会因为连接被清空而失败
    if (threadPool_) {
        threadPool_->waitForTasks();
    }
    
    // 等待所有任务完成后再清空连接
    std::lock_guard<std::mutex> lock(connectionsMtx_);
    connections_.clear();
}
