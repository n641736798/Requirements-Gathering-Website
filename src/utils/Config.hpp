#pragma once

#include <string>
#include <unordered_map>
#include <mutex>

/**
 * 存储模式枚举
 */
enum class StorageMode {
    MEMORY,     // 仅内存存储
    MYSQL,      // 仅 MySQL 存储
    HYBRID      // 混合模式：写入同时落内存和 MySQL，查询优先内存
};

/**
 * 配置管理类
 * 支持从 INI 文件和环境变量读取配置
 */
class Config {
public:
    static Config& getInstance();
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    Config(Config&&) = delete;
    Config& operator=(Config&&) = delete;
    bool loadFromFile(const std::string& filename);
    void loadFromEnv();
    std::string getString(const std::string& section, const std::string& key, const std::string& defaultValue = "") const;
    int getInt(const std::string& section, const std::string& key, int defaultValue = 0) const;
    bool getBool(const std::string& section, const std::string& key, bool defaultValue = false) const;
    double getDouble(const std::string& section, const std::string& key, double defaultValue = 0.0) const;
    void set(const std::string& section, const std::string& key, const std::string& value);
    bool has(const std::string& section, const std::string& key) const;
    StorageMode getStorageMode() const;
    std::string getMySQLHost() const { return getString("mysql", "host", "127.0.0.1"); }
    int getMySQLPort() const { return getInt("mysql", "port", 3306); }
    std::string getMySQLUser() const { return getString("mysql", "user", "root"); }
    std::string getMySQLPassword() const { return getString("mysql", "password", ""); }
    std::string getMySQLDatabase() const { return getString("mysql", "database", "device_data"); }
    int getPoolMinSize() const { return getInt("mysql", "pool_size_min", 5); }
    int getPoolMaxSize() const { return getInt("mysql", "pool_size_max", 20); }
    int getConnectTimeout() const { return getInt("mysql", "connect_timeout", 5); }
    int getServerPort() const { return getInt("server", "port", 8080); }
    int getThreadPoolSize() const { return getInt("server", "thread_pool_size", 4); }
    int getBatchSize() const { return getInt("storage", "batch_size", 0); }
    int getBatchIntervalMs() const { return getInt("storage", "batch_interval_ms", 1000); }
private:
    Config() = default;
    ~Config() = default;
    static std::string trim(const std::string& str);
    static std::string toUpper(const std::string& str);
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> data_;
};