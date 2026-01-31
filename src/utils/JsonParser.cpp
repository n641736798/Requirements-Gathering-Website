#include "JsonParser.hpp"
#include <sstream>
#include <cctype>
#include <cmath>

void JsonParser::skipWhitespace(const char*& p, const char* end) {
    while (p < end && std::isspace(*p)) ++p;
}

JsonValue JsonParser::parseString(const char*& p, const char* end) {
    if (p >= end || *p != '"') return JsonValue(nullptr);
    ++p;
    std::string s;
    while (p < end && *p != '"') {
        if (*p == '\\' && p + 1 < end) {
            ++p;
            switch (*p) {
                case 'n': s += '\n'; break;
                case 't': s += '\t'; break;
                case 'r': s += '\r'; break;
                case '\\': s += '\\'; break;
                case '"': s += '"'; break;
                default: s += *p; break;
            }
        } else {
            s += *p;
        }
        ++p;
    }
    if (p < end) ++p;
    return JsonValue(s);
}

JsonValue JsonParser::parseNumber(const char*& p, const char* end) {
    const char* start = p;
    bool isFloat = false;
    while (p < end && (std::isdigit(*p) || *p == '.' || *p == '-' || *p == '+' || *p == 'e' || *p == 'E')) {
        if (*p == '.' || *p == 'e' || *p == 'E') isFloat = true;
        ++p;
    }
    std::string numStr(start, p);
    if (isFloat) {
        return JsonValue(std::stod(numStr));
    } else {
        return JsonValue(std::stoll(numStr));
    }
}

JsonValue JsonParser::parseArray(const char*& p, const char* end) {
    if (p >= end || *p != '[') return JsonValue(nullptr);
    ++p;
    skipWhitespace(p, end);
    std::vector<JsonValue> arr;
    if (p < end && *p == ']') {
        ++p;
        return JsonValue(arr);
    }
    while (p < end) {
        skipWhitespace(p, end);
        arr.push_back(parseValue(p, end));
        skipWhitespace(p, end);
        if (p >= end || *p == ']') {
            if (p < end) ++p;
            break;
        }
        if (*p != ',') break;
        ++p;
    }
    return JsonValue(arr);
}

JsonValue JsonParser::parseObject(const char*& p, const char* end) {
    if (p >= end || *p != '{') return JsonValue(nullptr);
    ++p;
    skipWhitespace(p, end);
    std::unordered_map<std::string, JsonValue> obj;
    if (p < end && *p == '}') {
        ++p;
        return JsonValue(obj);
    }
    while (p < end) {
        skipWhitespace(p, end);
        if (*p != '"') break;
        JsonValue keyVal = parseString(p, end);
        if (!keyVal.isString()) break;
        std::string key = keyVal.asString();
        skipWhitespace(p, end);
        if (p >= end || *p != ':') break;
        ++p;
        skipWhitespace(p, end);
        obj[key] = parseValue(p, end);
        skipWhitespace(p, end);
        if (p >= end || *p == '}') {
            if (p < end) ++p;
            break;
        }
        if (*p != ',') break;
        ++p;
    }
    return JsonValue(obj);
}

JsonValue JsonParser::parseValue(const char*& p, const char* end) {
    skipWhitespace(p, end);
    if (p >= end) return JsonValue(nullptr);
    
    if (*p == '"') {
        return parseString(p, end);
    } else if (*p == '{') {
        return parseObject(p, end);
    } else if (*p == '[') {
        return parseArray(p, end);
    } else if (*p == '-' || std::isdigit(*p)) {
        return parseNumber(p, end);
    } else if (p + 4 <= end && std::string(p, p + 4) == "null") {
        p += 4;
        return JsonValue(nullptr);
    } else if (p + 4 <= end && std::string(p, p + 4) == "true") {
        p += 4;
        return JsonValue(true);
    } else if (p + 5 <= end && std::string(p, p + 5) == "false") {
        p += 5;
        return JsonValue(false);
    }
    return JsonValue(nullptr);
}

JsonValue JsonParser::parse(const std::string& json) {
    const char* p = json.c_str();
    const char* end = p + json.size();
    return parseValue(p, end);
}

std::string JsonParser::stringify(const JsonValue& value) {
    if (value.isNull()) {
        return "null";
    } else if (value.isString()) {
        return "\"" + escapeString(value.asString()) + "\"";
    } else {
        const auto& val = value.getValue();
        if (std::holds_alternative<bool>(val)) {
            return std::get<bool>(val) ? "true" : "false";
        }
    }
    if (value.isNumber()) {
        std::ostringstream oss;
        const auto& val = value.getValue();
        if (std::holds_alternative<long long>(val)) {
            oss << std::get<long long>(val);
        } else if (std::holds_alternative<double>(val)) {
            oss << std::get<double>(val);
        }
        return oss.str();
    } else if (value.isObject()) {
        std::ostringstream oss;
        oss << "{";
        bool first = true;
        for (const auto& [k, v] : value.asObject()) {
            if (!first) oss << ",";
            first = false;
            oss << "\"" << escapeString(k) << "\":" << stringify(v);
        }
        oss << "}";
        return oss.str();
    } else if (value.isArray()) {
        std::ostringstream oss;
        oss << "[";
        bool first = true;
        for (const auto& v : value.asArray()) {
            if (!first) oss << ",";
            first = false;
            oss << stringify(v);
        }
        oss << "]";
        return oss.str();
    }
    return "null";
}

std::string JsonParser::escapeString(const std::string& s) {
    std::string result;
    for (char c : s) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}
