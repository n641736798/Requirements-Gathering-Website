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

private:
    bool initialized_;
};
