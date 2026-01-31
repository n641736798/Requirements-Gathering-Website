#include "ReportHandler.hpp"
#include "utils/Logger.hpp"
#include <sstream>

ReportHandler::ReportHandler(StoreInterface& store, DeviceManager& deviceMgr)
    : store_(store), deviceMgr_(deviceMgr) {
}

bool ReportHandler::parseReportRequest(const JsonValue& json, ReportRequest& req) {
    if (!json.isObject()) {
        return false;
    }
    
    if (!json.has("device_id") || !json.get("device_id").isString()) {
        return false;
    }
    req.deviceId = json.get("device_id").asString();
    if (req.deviceId.empty()) {
        return false;
    }
    
    if (!json.has("timestamp") || !json.get("timestamp").isNumber()) {
        return false;
    }
    req.timestamp = json.get("timestamp").asInt();
    
    if (!json.has("metrics") || !json.get("metrics").isObject()) {
        return false;
    }
    
    const auto& metricsObj = json.get("metrics").asObject();
    for (const auto& [key, value] : metricsObj) {
        if (value.isNumber()) {
            req.metrics[key] = value.asDouble();
        }
    }
    
    return !req.metrics.empty();
}

bool ReportHandler::parseQueryRequest(const std::string& queryStr, QueryRequest& req) {
    // 简单解析：device_id=xxx&limit=100
    std::istringstream iss(queryStr);
    std::string token;
    bool hasDeviceId = false;
    req.limit = 100; // 默认值
    
    while (std::getline(iss, token, '&')) {
        size_t pos = token.find('=');
        if (pos == std::string::npos) continue;
        
        std::string key = token.substr(0, pos);
        std::string value = token.substr(pos + 1);
        
        if (key == "device_id") {
            req.deviceId = value;
            hasDeviceId = true;
        } else if (key == "limit") {
            try {
                req.limit = std::stoull(value);
                if (req.limit > 1000) req.limit = 1000; // 上限
            } catch (...) {
                req.limit = 100;
            }
        }
    }
    
    return hasDeviceId && !req.deviceId.empty();
}

JsonValue ReportHandler::handleReport(const ReportRequest& req) {
    // 自动注册设备
    deviceMgr_.ensureRegistered(req.deviceId);
    
    // 构造数据点
    DataPoint point;
    point.timestamp = req.timestamp;
    point.metrics = req.metrics;
    
    // 写入内存存储
    store_.append(req.deviceId, point);
    
    // 返回成功响应
    std::unordered_map<std::string, JsonValue> resp;
    resp["code"] = JsonValue(0LL);
    resp["message"] = JsonValue(std::string("ok"));
    return JsonValue(resp);
}

JsonValue ReportHandler::handleQuery(const QueryRequest& req) {
    auto data = store_.queryLatest(req.deviceId, req.limit);
    
    std::unordered_map<std::string, JsonValue> resp;
    resp["device_id"] = JsonValue(req.deviceId);
    
    std::vector<JsonValue> dataArray;
    for (const auto& point : data) {
        std::unordered_map<std::string, JsonValue> item;
        item["timestamp"] = JsonValue(point.timestamp);
        for (const auto& [key, value] : point.metrics) {
            item[key] = JsonValue(value);
        }
        dataArray.push_back(JsonValue(item));
    }
    resp["data"] = JsonValue(dataArray);
    
    return JsonValue(resp);
}
