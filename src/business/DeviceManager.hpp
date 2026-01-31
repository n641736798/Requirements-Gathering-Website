#pragma once

#include <string>
#include <unordered_set>
#include <mutex>
#include <memory>

// 前向声明
class MySQLStore;

/**
 * 设备管理模式
 */
enum class DeviceManagerMode {
    MEMORY,     // 仅内存管理
    MYSQL,      // 仅 MySQL 管理
    HYBRID      // 混合模式：内存 + MySQL
};

/**
 * 设备管理类
 * 支持内存模式和 MySQL 模式
 */
class DeviceManager {
public:
    /**
     * 构造函数
     * @param mode 管理模式
     */
    explicit DeviceManager(DeviceManagerMode mode = DeviceManagerMode::MEMORY);

    /**
     * 设置 MySQL 存储（用于 MySQL 或混合模式）
     * @param store MySQL 存储指针
     */
    void setMySQLStore(MySQLStore* store);

    /**
     * 检查设备是否存在
     * @param deviceId 设备ID
     * @return 是否存在
     */
    bool exists(const std::string& deviceId);

    /**
     * 确保设备已注册（如不存在则注册）
     * @param deviceId 设备ID
     */
    void ensureRegistered(const std::string& deviceId);

    /**
     * 获取已注册设备数量
     * @return 设备数量
     */
    std::size_t getDeviceCount() const;

    /**
     * 清空内存中的设备记录
     */
    void clearMemoryCache();

    /**
     * 获取当前模式
     */
    DeviceManagerMode getMode() const { return mode_; }

private:
    DeviceManagerMode mode_;
    MySQLStore* mysqlStore_;
    
    mutable std::mutex mtx_;
    std::unordered_set<std::string> devices_;
};
