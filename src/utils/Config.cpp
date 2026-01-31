#include "Config.hpp"
#include "Logger.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <tuple>

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

bool Config::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open config file: " + filename);
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    std::string currentSection;
    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        if (line[0] == '[') {
            std::size_t endPos = line.find(']');
            if (endPos != std::string::npos) {
                currentSection = trim(line.substr(1, endPos - 1));
            }
            continue;
        }
        std::size_t equalPos = line.find('=');
        if (equalPos != std::string::npos) {
            std::string key = trim(line.substr(0, equalPos));
            std::string value = trim(line.substr(equalPos + 1));
            if (value.length() >= 2 && ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\''))) {
                value = value.substr(1, value.length() - 2);
            }
            if (!currentSection.empty() && !key.empty()) {
                data_[currentSection][key] = value;
            }
        }
    }
    file.close();
    LOG_INFO("Config loaded from: " + filename);
    return true;
}

void Config::loadFromEnv() {
    std::lock_guard<std::mutex> lock(mutex_);
    const std::vector<std::tuple<std::string, std::string, std::string>> envMappings = {
        {"mysql", "host", "DEVICE_SERVER_MYSQL_HOST"},
        {"mysql", "port", "DEVICE_SERVER_MYSQL_PORT"},
        {"mysql", "user", "DEVICE_SERVER_MYSQL_USER"},
        {"mysql", "password", "DEVICE_SERVER_MYSQL_PASSWORD"},
        {"mysql", "database", "DEVICE_SERVER_MYSQL_DATABASE"},
        {"mysql", "pool_size_min", "DEVICE_SERVER_MYSQL_POOL_MIN"},
        {"mysql", "pool_size_max", "DEVICE_SERVER_MYSQL_POOL_MAX"},
        {"mysql", "connect_timeout", "DEVICE_SERVER_MYSQL_TIMEOUT"},
        {"server", "port", "DEVICE_SERVER_PORT"},
        {"server", "thread_pool_size", "DEVICE_SERVER_THREADS"},
        {"storage", "mode", "DEVICE_SERVER_STORAGE_MODE"},
        {"storage", "batch_size", "DEVICE_SERVER_BATCH_SIZE"},
    };
    for (const auto& [section, key, envName] : envMappings) {
        const char* envValue = std::getenv(envName.c_str());
        if (envValue && envValue[0] != '\0') {
            data_[section][key] = envValue;
        }
    }
}

std::string Config::getString(const std::string& section, const std::string& key, const std::string& defaultValue) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto sectionIt = data_.find(section);
    if (sectionIt == data_.end()) return defaultValue;
    auto keyIt = sectionIt->second.find(key);
    if (keyIt == sectionIt->second.end()) return defaultValue;
    return keyIt->second;
}

int Config::getInt(const std::string& section, const std::string& key, int defaultValue) const {
    std::string value = getString(section, key, "");
    if (value.empty()) return defaultValue;
    try { return std::stoi(value); } catch (...) { return defaultValue; }
}

bool Config::getBool(const std::string& section, const std::string& key, bool defaultValue) const {
    std::string value = getString(section, key, "");
    if (value.empty()) return defaultValue;
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "true" || lower == "yes" || lower == "1" || lower == "on") return true;
    if (lower == "false" || lower == "no" || lower == "0" || lower == "off") return false;
    return defaultValue;
}

double Config::getDouble(const std::string& section, const std::string& key, double defaultValue) const {
    std::string value = getString(section, key, "");
    if (value.empty()) return defaultValue;
    try { return std::stod(value); } catch (...) { return defaultValue; }
}

void Config::set(const std::string& section, const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    data_[section][key] = value;
}

bool Config::has(const std::string& section, const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto sectionIt = data_.find(section);
    if (sectionIt == data_.end()) return false;
    return sectionIt->second.find(key) != sectionIt->second.end();
}

StorageMode Config::getStorageMode() const {
    std::string mode = getString("storage", "mode", "memory");
    std::string lower = mode;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "mysql" || lower == "db" || lower == "database") return StorageMode::MYSQL;
    if (lower == "hybrid" || lower == "mixed" || lower == "both") return StorageMode::HYBRID;
    return StorageMode::MEMORY;
}

std::string Config::trim(const std::string& str) {
    std::size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    std::size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string Config::toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}