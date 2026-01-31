#pragma once

#include <string>
#include <vector>
#include <unordered_map>

/**
 * 数据点结构
 * 包含时间戳和指标数据
 */
struct DataPoint {
    long long timestamp;
    std::unordered_map<std::string, double> metrics;
};

/**
 * 存储抽象接口
 * 定义数据存储的统一接口，支持内存存储和MySQL存储的切换
 */
class StoreInterface {
public:
    virtual ~StoreInterface() = default;

    /**
     * 写入一条数据
     * @param deviceId 设备ID
     * @param point 数据点
     */
    virtual void append(const std::string& deviceId, const DataPoint& point) = 0;

    /**
     * 查询指定设备最近的 limit 条数据
     * @param deviceId 设备ID
     * @param limit 返回条数上限
     * @return 数据点列表，按时间正序排列
     */
    virtual std::vector<DataPoint> queryLatest(const std::string& deviceId, 
                                                std::size_t limit) const = 0;

    /**
     * 批量写入数据（可选实现，默认循环调用append）
     * @param deviceId 设备ID
     * @param points 数据点列表
     */
    virtual void appendBatch(const std::string& deviceId, 
                             const std::vector<DataPoint>& points) {
        for (const auto& point : points) {
            append(deviceId, point);
        }
    }
};
