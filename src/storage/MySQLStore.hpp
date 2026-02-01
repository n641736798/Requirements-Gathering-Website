#pragma once

#include "StoreInterface.hpp"
#include "ConnectionPool.hpp"
#include <memory>
#include <mutex>

class MySQLStore : public StoreInterface {
public:
    MySQLStore();
    ~MySQLStore() override;
    bool init(const MySQLConfig& config, const PoolConfig& poolConfig = PoolConfig());
    void shutdown();
    void appendRequirement(const Requirement& req) override;
    RequirementQueryResult queryRequirements(int page, int limit,
        int willingToPay, const std::string& keyword) const override;

    /** 检查设备是否已注册 */
    bool deviceExists(const std::string& deviceId) const;
    /** 确保设备已注册（不存在则插入） */
    void ensureDeviceRegistered(const std::string& deviceId);

private:
    bool initialized_;
};
