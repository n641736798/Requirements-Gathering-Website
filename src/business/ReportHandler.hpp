#pragma once

#include <string>
#include <unordered_map>
#include "storage/StoreInterface.hpp"
#include "business/DeviceManager.hpp"
#include "utils/JsonParser.hpp"

struct ReportRequest {
    std::string deviceId;
    long long timestamp;
    std::unordered_map<std::string, double> metrics;
};

struct QueryRequest {
    std::string deviceId;
    std::size_t limit;
};

/**
 * 业务处理类
 * 处理设备数据上报和查询请求
 * 支持多态存储（MemoryStore / MySQLStore）
 */
class ReportHandler {
public:
    /**
     * 构造函数
     * @param store 存储接口引用
     * @param deviceMgr 设备管理器引用
     */
    ReportHandler(StoreInterface& store, DeviceManager& deviceMgr);
    
    // 处理上报请求
    JsonValue handleReport(const ReportRequest& req);
    
    // 处理查询请求
    JsonValue handleQuery(const QueryRequest& req);
    
    // 从 JSON 解析上报请求
    static bool parseReportRequest(const JsonValue& json, ReportRequest& req);
    
    // 从 URL 参数解析查询请求
    static bool parseQueryRequest(const std::string& queryStr, QueryRequest& req);

private:
    StoreInterface& store_;
    DeviceManager& deviceMgr_;
};
