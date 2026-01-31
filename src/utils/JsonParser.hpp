#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <variant>

// 简易 JSON 解析器（仅支持基本类型和对象）
class JsonValue {
public:
    using Value = std::variant<std::nullptr_t, bool, long long, double, std::string, 
                               std::unordered_map<std::string, JsonValue>, 
                               std::vector<JsonValue>>;
    
    JsonValue() : value_(nullptr) {}
    JsonValue(Value v) : value_(std::move(v)) {}
    
    bool isNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
    bool isObject() const { return std::holds_alternative<std::unordered_map<std::string, JsonValue>>(value_); }
    bool isArray() const { return std::holds_alternative<std::vector<JsonValue>>(value_); }
    bool isString() const { return std::holds_alternative<std::string>(value_); }
    bool isNumber() const { return std::holds_alternative<long long>(value_) || std::holds_alternative<double>(value_); }
    
    const std::unordered_map<std::string, JsonValue>& asObject() const {
        return std::get<std::unordered_map<std::string, JsonValue>>(value_);
    }
    
    const std::vector<JsonValue>& asArray() const {
        return std::get<std::vector<JsonValue>>(value_);
    }
    
    const std::string& asString() const {
        return std::get<std::string>(value_);
    }
    
    long long asInt() const {
        if (std::holds_alternative<long long>(value_)) {
            return std::get<long long>(value_);
        }
        return static_cast<long long>(std::get<double>(value_));
    }
    
    double asDouble() const {
        if (std::holds_alternative<double>(value_)) {
            return std::get<double>(value_);
        }
        return static_cast<double>(std::get<long long>(value_));
    }
    
    bool has(const std::string& key) const {
        if (!isObject()) return false;
        return asObject().find(key) != asObject().end();
    }
    
    const JsonValue& get(const std::string& key) const {
        static JsonValue nullValue;
        if (!isObject()) return nullValue;
        auto it = asObject().find(key);
        if (it == asObject().end()) return nullValue;
        return it->second;
    }
    
    const Value& getValue() const { return value_; }
    
private:
    Value value_;
};

class JsonParser {
public:
    static JsonValue parse(const std::string& json);
    static std::string stringify(const JsonValue& value);
    
private:
    static JsonValue parseValue(const char*& p, const char* end);
    static JsonValue parseObject(const char*& p, const char* end);
    static JsonValue parseArray(const char*& p, const char* end);
    static JsonValue parseString(const char*& p, const char* end);
    static JsonValue parseNumber(const char*& p, const char* end);
    static void skipWhitespace(const char*& p, const char* end);
    static std::string escapeString(const std::string& s);
};
