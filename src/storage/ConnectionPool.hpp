#pragma once

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>

struct MySQLConfig {
    std::string host = "127.0.0.1";
    int port = 3306;
    std::string user = "root";
    std::string password = "";
    std::string database = "device_data";
    std::string charset = "utf8mb4";
    int connectTimeout = 5;
    int readTimeout = 30;
    int writeTimeout = 30;
};

struct PoolConfig {
    int minSize = 5;
    int maxSize = 20;
    int maxIdleTime = 300;
    int healthCheckInterval = 60;
};

class MySQLConnection {
public:
    MySQLConnection();
    ~MySQLConnection();
    MySQLConnection(const MySQLConnection&) = delete;
    MySQLConnection& operator=(const MySQLConnection&) = delete;
    MySQLConnection(MySQLConnection&& other) noexcept;
    MySQLConnection& operator=(MySQLConnection&& other) noexcept;
    bool connect(const MySQLConfig& config);
    void disconnect();
    bool isValid() const;
    bool ping();
    MYSQL* get() const { return conn_; }
    bool execute(const std::string& sql);
    MYSQL_RES* query(const std::string& sql);
    std::string getLastError() const;
    unsigned long long getLastInsertId() const;
    unsigned long long getAffectedRows() const;
    std::string escapeString(const std::string& str);
    void updateLastUsedTime();
    time_t getLastUsedTime() const { return lastUsedTime_; }
private:
    MYSQL* conn_;
    time_t lastUsedTime_;
    bool connected_;
};

class ConnectionPool {
public:
    static ConnectionPool& getInstance();
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;
    bool init(const MySQLConfig& mysqlConfig, const PoolConfig& poolConfig = PoolConfig());
    void shutdown();
    std::shared_ptr<MySQLConnection> getConnection(int timeoutMs = 5000);
    void releaseConnection(std::shared_ptr<MySQLConnection> conn);
    int getPoolSize() const;
    int getActiveCount() const;
    bool isInitialized() const { return initialized_; }
private:
    ConnectionPool();
    ~ConnectionPool();
    std::shared_ptr<MySQLConnection> createConnection();
    void cleanupInvalidConnections();
    MySQLConfig mysqlConfig_;
    PoolConfig poolConfig_;
    std::queue<std::shared_ptr<MySQLConnection>> pool_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<int> totalCount_;
    std::atomic<int> activeCount_;
    std::atomic<bool> initialized_;
    std::atomic<bool> shutdown_;
};

class ConnectionGuard {
public:
    explicit ConnectionGuard(std::shared_ptr<MySQLConnection> conn);
    ~ConnectionGuard();
    ConnectionGuard(const ConnectionGuard&) = delete;
    ConnectionGuard& operator=(const ConnectionGuard&) = delete;
    ConnectionGuard(ConnectionGuard&& other) noexcept;
    ConnectionGuard& operator=(ConnectionGuard&& other) noexcept;
    MySQLConnection* get() const { return conn_.get(); }
    MySQLConnection* operator->() const { return conn_.get(); }
    explicit operator bool() const { return conn_ != nullptr && conn_->isValid(); }
private:
    std::shared_ptr<MySQLConnection> conn_;
};