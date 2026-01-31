#include "ReportHandler.hpp"
#include "utils/Logger.hpp"
#include <sstream>

ReportHandler::ReportHandler(StoreInterface& store) : store_(store) {
}

bool ReportHandler::parseRequirementReportRequest(const JsonValue& json, RequirementReportRequest& req) {
    if (!json.isObject()) return false;

    if (!json.has("title") || !json.get("title").isString()) return false;
    req.title = json.get("title").asString();
    if (req.title.empty()) return false;

    if (!json.has("content") || !json.get("content").isString()) return false;
    req.content = json.get("content").asString();
    if (req.content.empty()) return false;

    if (json.has("willing_to_pay")) {
        const auto& v = json.get("willing_to_pay");
        if (v.isNull()) {
            req.willing_to_pay = -1;
        } else if (v.isNumber()) {
            int val = static_cast<int>(v.asInt());
            if (val == 0) req.willing_to_pay = 0;
            else if (val == 1) req.willing_to_pay = 1;
            else req.willing_to_pay = -1;
        } else {
            req.willing_to_pay = -1;
        }
    } else {
        req.willing_to_pay = -1;
    }

    if (json.has("contact") && json.get("contact").isString()) {
        req.contact = json.get("contact").asString();
    }
    if (json.has("notes") && json.get("notes").isString()) {
        req.notes = json.get("notes").asString();
    }
    return true;
}

bool ReportHandler::parseRequirementQueryRequest(const std::string& queryStr, RequirementQueryRequest& req) {
    std::istringstream iss(queryStr);
    std::string token;
    req.page = 1;
    req.limit = 100;
    req.willing_to_pay = -1;

    while (std::getline(iss, token, '&')) {
        size_t pos = token.find('=');
        if (pos == std::string::npos) continue;

        std::string key = token.substr(0, pos);
        std::string value = token.substr(pos + 1);

        if (key == "page") {
            try { req.page = std::max(1, std::stoi(value)); } catch (...) {}
        } else if (key == "limit") {
            try { req.limit = std::max(1, std::min(100, std::stoi(value))); } catch (...) {}
        } else if (key == "willing_to_pay") {
            try {
                int v = std::stoi(value);
                if (v == 0 || v == 1) req.willing_to_pay = v;
                else if (v == 2) req.willing_to_pay = 2;  // 2 表示筛选"空/未填"(NULL)
            } catch (...) {}
        } else if (key == "keyword") {
            req.keyword = value;
        }
    }
    return true;
}

JsonValue ReportHandler::handleRequirementReport(const RequirementReportRequest& req) {
    Requirement r;
    r.title = req.title;
    r.content = req.content;
    r.willing_to_pay = req.willing_to_pay;
    r.contact = req.contact;
    r.notes = req.notes;

    store_.appendRequirement(r);

    std::unordered_map<std::string, JsonValue> resp;
    resp["code"] = JsonValue(0LL);
    resp["message"] = JsonValue(std::string("ok"));
    return JsonValue(resp);
}

JsonValue ReportHandler::handleRequirementQuery(const RequirementQueryRequest& req) {
    auto result = store_.queryRequirements(req.page, req.limit, req.willing_to_pay, req.keyword);

    std::unordered_map<std::string, JsonValue> resp;
    resp["code"] = JsonValue(0LL);

    std::vector<JsonValue> dataArray;
    for (const auto& r : result.data) {
        std::unordered_map<std::string, JsonValue> item;
        item["id"] = JsonValue(r.id);
        item["title"] = JsonValue(r.title);
        item["content"] = JsonValue(r.content);
        item["willing_to_pay"] = r.willing_to_pay < 0 ? JsonValue() : JsonValue(static_cast<long long>(r.willing_to_pay));
        item["contact"] = JsonValue(r.contact);
        item["notes"] = JsonValue(r.notes);
        item["created_at"] = JsonValue(r.created_at);
        item["updated_at"] = JsonValue(r.updated_at);
        dataArray.push_back(JsonValue(item));
    }
    resp["data"] = JsonValue(dataArray);
    resp["total"] = JsonValue(result.total);
    resp["page"] = JsonValue(static_cast<long long>(result.page));
    resp["limit"] = JsonValue(static_cast<long long>(result.limit));

    return JsonValue(resp);
}
