#pragma once

#include <string>
#include "storage/StoreInterface.hpp"
#include "utils/JsonParser.hpp"

struct RequirementReportRequest {
    std::string title;
    std::string content;
    int willing_to_pay = -1;  // -1=空, 0=不愿意, 1=愿意
    std::string contact;
    std::string notes;
};

struct RequirementQueryRequest {
    int page = 1;
    int limit = 100;
    int willing_to_pay = -1;  // -1=不过滤, 0=不愿意, 1=愿意, 2=空/未填
    std::string keyword;
};

/**
 * 业务处理类
 * 处理需求上报和查询请求
 * 支持多态存储（MemoryStore / MySQLStore）
 */
class ReportHandler {
public:
    explicit ReportHandler(StoreInterface& store);

    JsonValue handleRequirementReport(const RequirementReportRequest& req);
    JsonValue handleRequirementQuery(const RequirementQueryRequest& req);

    static bool parseRequirementReportRequest(const JsonValue& json, RequirementReportRequest& req);
    static bool parseRequirementQueryRequest(const std::string& queryStr, RequirementQueryRequest& req);

private:
    StoreInterface& store_;
};
