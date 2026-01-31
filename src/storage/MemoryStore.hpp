#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <shared_mutex>
#include "StoreInterface.hpp"

/**
 * 内存存储实现
 * 使用 unordered_map + vector 存储设备数据
 * 线程安全，使用读写锁保护
 */
class MemoryStore : public StoreInterface {
public:
    // 写入一条数据
    void append(const std::string& deviceId, const DataPoint& point) override;

    // 查询指定设备最近的 limit 条数据
    std::vector<DataPoint> queryLatest(const std::string& deviceId, std::size_t limit) const override;

private:
    using Series = std::vector<DataPoint>;

    mutable std::shared_mutex mtx_;
    std::unordered_map<std::string, Series> data_;
};
