#pragma once

#include <vector>
#include <string>
#include <shared_mutex>
#include "StoreInterface.hpp"

/**
 * 内存存储实现
 * 使用 vector 存储需求数据
 * 线程安全，使用读写锁保护
 */
class MemoryStore : public StoreInterface {
public:
    void appendRequirement(const Requirement& req) override;
    RequirementQueryResult queryRequirements(int page, int limit,
        int willingToPay, const std::string& keyword) const override;

private:
    mutable std::shared_mutex mtx_;
    std::vector<Requirement> data_;
};
