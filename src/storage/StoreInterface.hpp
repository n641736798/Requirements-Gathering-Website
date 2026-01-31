#pragma once

#include <string>
#include <vector>
#include <cstdint>

/**
 * 需求实体
 */
struct Requirement {
    int64_t id = 0;
    std::string title;
    std::string content;
    int willing_to_pay = -1;  // -1=空, 0=不愿意, 1=愿意
    std::string contact;
    std::string notes;
    std::string created_at;
    std::string updated_at;
};

/**
 * 需求查询结果（含分页信息）
 */
struct RequirementQueryResult {
    std::vector<Requirement> data;
    int64_t total = 0;
    int page = 1;
    int limit = 100;
};

/**
 * 存储抽象接口
 * 定义需求存储的统一接口，支持内存存储和MySQL存储的切换
 */
class StoreInterface {
public:
    virtual ~StoreInterface() = default;

    /**
     * 插入一条需求
     * @param req 需求实体（id 由存储层生成）
     */
    virtual void appendRequirement(const Requirement& req) = 0;

    /**
     * 分页查询需求列表
     * @param page 页码（从1开始）
     * @param limit 每页条数，默认100
     * @param willingToPay 筛选愿意付费：-1=不过滤, 0=不愿意, 1=愿意, 2=空/未填
     * @param keyword 关键词模糊搜索（title/content），空则不过滤
     * @return 查询结果（含分页信息）
     */
    virtual RequirementQueryResult queryRequirements(int page, int limit,
        int willingToPay, const std::string& keyword) const = 0;
};
