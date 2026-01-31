#pragma once

#include "StoreInterface.hpp"
#include "ConnectionPool.hpp"
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>

class MySQLStore : public StoreInterface {
public:
    explicit MySQLStore(std::size_t batchSize = 0, int batchIntervalMs = 0);
    ~MySQLStore() override;
    bool init(const MySQLConfig& config, const PoolConfig& poolConfig = PoolConfig());
    void shutdown();
    void append(const std::string& deviceId, const DataPoint& point) override;
    std::vector<DataPoint> queryLatest(const std::string& deviceId, std::size_t limit) const override;
    void appendBatch(const std::string& deviceId, const std::vector<DataPoint>& points) override;
    bool deviceExists(const std::string& deviceId) const;
    bool ensureDeviceRegistered(const std::string& deviceId);
    std::vector<DataPoint> queryByTimeRange(const std::string& deviceId, long long startTime, long long endTime, std::size_t limit) const;
    std::size_t deleteOldData(long long beforeTimestamp);
    std::size_t getDataCount(const std::string& deviceId) const;
private:
    static std::string metricsToJson(const std::unordered_map<std::string, double>& metrics);
    static std::unordered_map<std::string, double> jsonToMetrics(const std::string& json);
    void flushBatch();
    void flushThreadFunc();
    bool initialized_;
    std::size_t batchSize_;
    int batchIntervalMs_;
    mutable std::mutex batchMutex_;
    std::vector<std::pair<std::string, DataPoint>> batchBuffer_;
    std::thread flushThread_;
    std::atomic<bool> flushShutdown_{false};
};