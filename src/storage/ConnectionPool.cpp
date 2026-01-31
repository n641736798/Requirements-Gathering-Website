#include "ConnectionPool.hpp"
#include "utils/Logger.hpp"
#include <chrono>

MySQLConnection::MySQLConnection() : conn_(nullptr), lastUsedTime_(0), connected_(false) {}

MySQLConnection::~MySQLConnection() { disconnect(); }

MySQLConnection::MySQLConnection(MySQLConnection&& other) noexcept : conn_(other.conn_), lastUsedTime_(other.lastUsedTime_), connected_(other.connected_) {
    other.conn_ = nullptr;
    other.connected_ = false;
}

MySQLConnection& MySQLConnection::operator=(MySQLConnection&& other) noexcept {
    if (this != &other) {
        disconnect();
        conn_ = other.conn_;
        lastUsedTime_ = other.lastUsedTime_;
        connected_ = other.connected_;
        other.conn_ = nullptr;
        other.connected_ = false;
    }
    return *this;
}

bool MySQLConnection::connect(const MySQLConfig& config) {
    if (connected_) disconnect();
    conn_ = mysql_init(nullptr);
    if (!conn_) { LOG_ERROR("mysql_init failed"); return false; }
    unsigned int timeout = static_cast<unsigned int>(config.connectTimeout);
    mysql_options(conn_, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    unsigned int readTimeout = static_cast<unsigned int>(config.readTimeout);
    mysql_options(conn_, MYSQL_OPT_READ_TIMEOUT, &readTimeout);
    unsigned int writeTimeout = static_cast<unsigned int>(config.writeTimeout);
    mysql_options(conn_, MYSQL_OPT_WRITE_TIMEOUT, &writeTimeout);
    bool reconnect = true;
    mysql_options(conn_, MYSQL_OPT_RECONNECT, &reconnect);
    mysql_options(conn_, MYSQL_SET_CHARSET_NAME, config.charset.c_str());
    if (!mysql_real_connect(conn_, config.host.c_str(), config.user.c_str(), config.password.c_str(), config.database.c_str(), config.port, nullptr, CLIENT_MULTI_STATEMENTS)) {
        LOG_ERROR("mysql_real_connect failed: " + std::string(mysql_error(conn_)));
        mysql_close(conn_);
        conn_ = nullptr;
        return false;
    }
    connected_ = true;
    updateLastUsedTime();
    LOG_INFO("MySQL connection established");
    return true;
}

void MySQLConnection::disconnect() {
    if (conn_) { mysql_close(conn_); conn_ = nullptr; }
    connected_ = false;
}

bool MySQLConnection::isValid() const { return conn_ != nullptr && connected_; }

bool MySQLConnection::ping() { return conn_ && mysql_ping(conn_) == 0; }

bool MySQLConnection::execute(const std::string& sql) {
    if (!isValid()) { LOG_ERROR("Connection is not valid"); return false; }
    if (mysql_query(conn_, sql.c_str()) != 0) { LOG_ERROR("mysql_query failed"); return false; }
    while (mysql_more_results(conn_)) { mysql_next_result(conn_); MYSQL_RES* r = mysql_store_result(conn_); if (r) mysql_free_result(r); }
    updateLastUsedTime();
    return true;
}

MYSQL_RES* MySQLConnection::query(const std::string& sql) {
    if (!isValid()) { LOG_ERROR("Connection is not valid"); return nullptr; }
    if (mysql_query(conn_, sql.c_str()) != 0) { LOG_ERROR("mysql_query failed"); return nullptr; }
    updateLastUsedTime();
    return mysql_store_result(conn_);
}

std::string MySQLConnection::getLastError() const { return conn_ ? mysql_error(conn_) : "Connection is null"; }

unsigned long long MySQLConnection::getLastInsertId() const { return conn_ ? mysql_insert_id(conn_) : 0; }

unsigned long long MySQLConnection::getAffectedRows() const { return conn_ ? mysql_affected_rows(conn_) : 0; }

std::string MySQLConnection::escapeString(const std::string& str) {
    if (!conn_ || str.empty()) return str;
    std::string escaped;
    escaped.resize(str.length() * 2 + 1);
    unsigned long len = mysql_real_escape_string(conn_, &escaped[0], str.c_str(), str.length());
    escaped.resize(len);
    return escaped;
}

void MySQLConnection::updateLastUsedTime() { lastUsedTime_ = std::time(nullptr); }

ConnectionPool::ConnectionPool() : totalCount_(0), activeCount_(0), initialized_(false), shutdown_(false) {}

ConnectionPool::~ConnectionPool() { shutdown(); }

ConnectionPool& ConnectionPool::getInstance() { static ConnectionPool instance; return instance; }

bool ConnectionPool::init(const MySQLConfig& mysqlConfig, const PoolConfig& poolConfig) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) { LOG_WARNING("ConnectionPool already initialized"); return true; }
    mysqlConfig_ = mysqlConfig;
    poolConfig_ = poolConfig;
    shutdown_ = false;
    if (mysql_library_init(0, nullptr, nullptr) != 0) { LOG_ERROR("mysql_library_init failed"); return false; }
    for (int i = 0; i < poolConfig_.minSize; ++i) {
        auto conn = createConnection();
        if (conn) { pool_.push(conn); ++totalCount_; } else { LOG_ERROR("Failed to create initial connection"); }
    }
    if (pool_.empty()) { LOG_ERROR("Failed to create any connection"); mysql_library_end(); return false; }
    initialized_ = true;
    LOG_INFO("ConnectionPool initialized");
    return true;
}

void ConnectionPool::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) return;
    shutdown_ = true;
    while (!pool_.empty()) pool_.pop();
    totalCount_ = 0;
    activeCount_ = 0;
    initialized_ = false;
    mysql_library_end();
    LOG_INFO("ConnectionPool shutdown");
}

std::shared_ptr<MySQLConnection> ConnectionPool::getConnection(int timeoutMs) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!initialized_ || shutdown_) { LOG_ERROR("ConnectionPool is not available"); return nullptr; }
    auto waitUntil = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (pool_.empty()) {
        if (totalCount_ < poolConfig_.maxSize) {
            auto conn = createConnection();
            if (conn) { ++totalCount_; ++activeCount_; conn->updateLastUsedTime(); return conn; }
        }
        if (timeoutMs < 0) cv_.wait(lock);
        else if (cv_.wait_until(lock, waitUntil) == std::cv_status::timeout) { LOG_WARNING("Get connection timeout"); return nullptr; }
        if (shutdown_) return nullptr;
    }
    auto conn = pool_.front();
    pool_.pop();
    ++activeCount_;
    if (!conn->ping()) {
        --totalCount_; --activeCount_;
        conn = createConnection();
        if (conn) { ++totalCount_; ++activeCount_; }
    }
    if (conn) conn->updateLastUsedTime();
    return conn;
}

void ConnectionPool::releaseConnection(std::shared_ptr<MySQLConnection> conn) {
    if (!conn) return;
    std::lock_guard<std::mutex> lock(mutex_);
    --activeCount_;
    if (shutdown_) { --totalCount_; return; }
    if (conn->isValid()) { conn->updateLastUsedTime(); pool_.push(conn); cv_.notify_one(); }
    else { --totalCount_; LOG_WARNING("Released invalid connection"); }
}

std::shared_ptr<MySQLConnection> ConnectionPool::createConnection() {
    auto conn = std::make_shared<MySQLConnection>();
    if (!conn->connect(mysqlConfig_)) return nullptr;
    return conn;
}

int ConnectionPool::getPoolSize() const { std::lock_guard<std::mutex> lock(mutex_); return static_cast<int>(pool_.size()); }

int ConnectionPool::getActiveCount() const { return activeCount_.load(); }

void ConnectionPool::cleanupInvalidConnections() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::queue<std::shared_ptr<MySQLConnection>> validConnections;
    while (!pool_.empty()) {
        auto conn = pool_.front();
        pool_.pop();
        if (conn->ping()) validConnections.push(conn);
        else { --totalCount_; LOG_WARNING("Removed invalid connection"); }
    }
    pool_ = std::move(validConnections);
}

ConnectionGuard::ConnectionGuard(std::shared_ptr<MySQLConnection> conn) : conn_(std::move(conn)) {}

ConnectionGuard::~ConnectionGuard() { if (conn_) ConnectionPool::getInstance().releaseConnection(conn_); }

ConnectionGuard::ConnectionGuard(ConnectionGuard&& other) noexcept : conn_(std::move(other.conn_)) {}

ConnectionGuard& ConnectionGuard::operator=(ConnectionGuard&& other) noexcept {
    if (this != &other) {
        if (conn_) ConnectionPool::getInstance().releaseConnection(conn_);
        conn_ = std::move(other.conn_);
    }
    return *this;
}