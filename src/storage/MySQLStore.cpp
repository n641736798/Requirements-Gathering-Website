#include "MySQLStore.hpp"
#include "utils/Logger.hpp"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <chrono>

MySQLStore::MySQLStore(std::size_t batchSize, int batchIntervalMs)
    : initialized_(false)
    , batchSize_(batchSize)
    , batchIntervalMs_(batchIntervalMs <= 0 ? 0 : batchIntervalMs) {
}

MySQLStore::~MySQLStore() { shutdown(); }

bool MySQLStore::init(const MySQLConfig& config, const PoolConfig& poolConfig) {
    if (initialized_) { LOG_WARNING("MySQLStore already initialized"); return true; }
    if (!ConnectionPool::getInstance().init(config, poolConfig)) { LOG_ERROR("Failed to initialize connection pool"); return false; }
    initialized_ = true;
    if (batchSize_ > 0 && batchIntervalMs_ > 0) {
        flushShutdown_ = false;
        flushThread_ = std::thread(&MySQLStore::flushThreadFunc, this);
        LOG_INFO("MySQLStore batch flush thread started");
    }
    LOG_INFO("MySQLStore initialized");
    return true;
}

void MySQLStore::shutdown() {
    if (!initialized_) return;
    if (flushThread_.joinable()) { flushShutdown_ = true; flushThread_.join(); }
    if (batchSize_ > 0) flushBatch();
    ConnectionPool::getInstance().shutdown();
    initialized_ = false;
    LOG_INFO("MySQLStore shutdown");
}

void MySQLStore::append(const std::string& deviceId, const DataPoint& point) {
    if (!initialized_) { LOG_ERROR("MySQLStore not initialized"); return; }
    if (batchSize_ > 0) {
        bool needFlush = false;
        { std::lock_guard<std::mutex> lock(batchMutex_); batchBuffer_.emplace_back(deviceId, point); needFlush = (batchBuffer_.size() >= batchSize_); }
        if (needFlush) flushBatch();
        return;
    }
    ConnectionGuard guard(ConnectionPool::getInstance().getConnection());
    if (!guard) { LOG_ERROR("Failed to get connection"); return; }
    std::string metricsJson = metricsToJson(point.metrics);
    std::string escapedDeviceId = guard->escapeString(deviceId);
    std::string escapedMetrics = guard->escapeString(metricsJson);
    std::ostringstream sql;
    sql << "INSERT INTO data_points (device_id, timestamp, metrics) VALUES ('" << escapedDeviceId << "', " << point.timestamp << ", '" << escapedMetrics << "')";
    if (!guard->execute(sql.str())) LOG_ERROR("Failed to insert data point");
}

std::vector<DataPoint> MySQLStore::queryLatest(const std::string& deviceId, std::size_t limit) const {
    std::vector<DataPoint> result;
    if (!initialized_) { LOG_ERROR("MySQLStore not initialized"); return result; }
    std::vector<DataPoint> bufferData;
    if (batchSize_ > 0) {
        std::lock_guard<std::mutex> lock(batchMutex_);
        for (const auto& [did, point] : batchBuffer_) { if (did == deviceId) bufferData.push_back(point); }
        std::sort(bufferData.begin(), bufferData.end(), [](const DataPoint& a, const DataPoint& b) { return a.timestamp < b.timestamp; });
    }
    ConnectionGuard guard(ConnectionPool::getInstance().getConnection());
    if (!guard) return bufferData;
    std::string escapedDeviceId = guard->escapeString(deviceId);
    std::ostringstream sql;
    sql << "SELECT timestamp, metrics FROM data_points WHERE device_id = '" << escapedDeviceId << "' ORDER BY timestamp DESC LIMIT " << limit;
    MYSQL_RES* res = guard->query(sql.str());
    if (!res) return bufferData;
    std::vector<DataPoint> dbData;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(res);
        DataPoint point;
        point.timestamp = std::stoll(row[0]);
        if (row[1] && lengths[1] > 0) point.metrics = jsonToMetrics(std::string(row[1], lengths[1]));
        dbData.push_back(point);
    }
    mysql_free_result(res);
    std::vector<DataPoint> merged;
    merged.reserve(bufferData.size() + dbData.size());
    for (const auto& p : bufferData) merged.push_back(p);
    for (auto it = dbData.rbegin(); it != dbData.rend(); ++it) merged.push_back(*it);
    std::sort(merged.begin(), merged.end(), [](const DataPoint& a, const DataPoint& b) { return a.timestamp < b.timestamp; });
    if (merged.size() > limit) result.assign(merged.end() - limit, merged.end());
    else result = std::move(merged);
    return result;
}

void MySQLStore::appendBatch(const std::string& deviceId, const std::vector<DataPoint>& points) {
    if (!initialized_ || points.empty()) return;
    ConnectionGuard guard(ConnectionPool::getInstance().getConnection());
    if (!guard) return;
    std::string escapedDeviceId = guard->escapeString(deviceId);
    std::ostringstream sql;
    sql << "INSERT INTO data_points (device_id, timestamp, metrics) VALUES ";
    bool first = true;
    for (const auto& point : points) {
        if (!first) sql << ", ";
        first = false;
        std::string metricsJson = metricsToJson(point.metrics);
        std::string escapedMetrics = guard->escapeString(metricsJson);
        sql << "('" << escapedDeviceId << "', " << point.timestamp << ", '" << escapedMetrics << "')";
    }
    if (!guard->execute(sql.str())) LOG_ERROR("Failed to batch insert");
}

bool MySQLStore::deviceExists(const std::string& deviceId) const {
    if (!initialized_) return false;
    ConnectionGuard guard(ConnectionPool::getInstance().getConnection());
    if (!guard) return false;
    std::string escapedDeviceId = guard->escapeString(deviceId);
    std::ostringstream sql;
    sql << "SELECT 1 FROM devices WHERE device_id = '" << escapedDeviceId << "' LIMIT 1";
    MYSQL_RES* res = guard->query(sql.str());
    if (!res) return false;
    bool exists = mysql_num_rows(res) > 0;
    mysql_free_result(res);
    return exists;
}

bool MySQLStore::ensureDeviceRegistered(const std::string& deviceId) {
    if (!initialized_) return false;
    ConnectionGuard guard(ConnectionPool::getInstance().getConnection());
    if (!guard) return false;
    std::string escapedDeviceId = guard->escapeString(deviceId);
    std::ostringstream sql;
    sql << "INSERT IGNORE INTO devices (device_id) VALUES ('" << escapedDeviceId << "')";
    return guard->execute(sql.str());
}

std::vector<DataPoint> MySQLStore::queryByTimeRange(const std::string& deviceId, long long startTime, long long endTime, std::size_t limit) const {
    std::vector<DataPoint> result;
    if (!initialized_) return result;
    ConnectionGuard guard(ConnectionPool::getInstance().getConnection());
    if (!guard) return result;
    std::string escapedDeviceId = guard->escapeString(deviceId);
    std::ostringstream sql;
    sql << "SELECT timestamp, metrics FROM data_points WHERE device_id = '" << escapedDeviceId << "' AND timestamp >= " << startTime << " AND timestamp <= " << endTime << " ORDER BY timestamp ASC LIMIT " << limit;
    MYSQL_RES* res = guard->query(sql.str());
    if (!res) return result;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(res);
        DataPoint point;
        point.timestamp = std::stoll(row[0]);
        if (row[1] && lengths[1] > 0) point.metrics = jsonToMetrics(std::string(row[1], lengths[1]));
        result.push_back(point);
    }
    mysql_free_result(res);
    return result;
}

std::size_t MySQLStore::deleteOldData(long long beforeTimestamp) {
    if (!initialized_) return 0;
    ConnectionGuard guard(ConnectionPool::getInstance().getConnection());
    if (!guard) return 0;
    std::ostringstream sql;
    sql << "DELETE FROM data_points WHERE timestamp < " << beforeTimestamp;
    if (!guard->execute(sql.str())) return 0;
    return static_cast<std::size_t>(guard->getAffectedRows());
}

std::size_t MySQLStore::getDataCount(const std::string& deviceId) const {
    if (!initialized_) return 0;
    ConnectionGuard guard(ConnectionPool::getInstance().getConnection());
    if (!guard) return 0;
    std::string escapedDeviceId = guard->escapeString(deviceId);
    std::ostringstream sql;
    sql << "SELECT COUNT(*) FROM data_points WHERE device_id = '" << escapedDeviceId << "'";
    MYSQL_RES* res = guard->query(sql.str());
    if (!res) return 0;
    std::size_t count = 0;
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row && row[0]) count = std::stoull(row[0]);
    mysql_free_result(res);
    return count;
}

std::string MySQLStore::metricsToJson(const std::unordered_map<std::string, double>& metrics) {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [key, value] : metrics) {
        if (!first) oss << ",";
        first = false;
        oss << "\"";
        for (char c : key) { switch (c) { case '\"': oss << "\\\""; break; case '\\': oss << "\\\\"; break; case '\n': oss << "\\n"; break; case '\r': oss << "\\r"; break; case '\t': oss << "\\t"; break; default: oss << c; } }
        oss << "\":" << std::setprecision(15) << value;
    }
    oss << "}";
    return oss.str();
}

std::unordered_map<std::string, double> MySQLStore::jsonToMetrics(const std::string& json) {
    std::unordered_map<std::string, double> metrics;
    if (json.empty() || json.length() < 2) return metrics;
    std::size_t pos = 1;
    while (pos < json.length()) {
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) ++pos;
        if (pos >= json.length() || json[pos] == '}') break;
        if (json[pos] != '\"') break;
        ++pos;
        std::string key;
        while (pos < json.length() && json[pos] != '\"') { if (json[pos] == '\\' && pos + 1 < json.length()) { ++pos; switch (json[pos]) { case 'n': key += '\n'; break; case 'r': key += '\r'; break; case 't': key += '\t'; break; default: key += json[pos]; } } else key += json[pos]; ++pos; }
        if (pos >= json.length()) break;
        ++pos;
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == ':')) ++pos;
        std::string valueStr;
        while (pos < json.length() && json[pos] != ',' && json[pos] != '}') { if (json[pos] != ' ' && json[pos] != '\t' && json[pos] != '\n' && json[pos] != '\r') valueStr += json[pos]; ++pos; }
        if (!key.empty() && !valueStr.empty()) { try { metrics[key] = std::stod(valueStr); } catch (...) {} }
        if (pos < json.length() && json[pos] == ',') ++pos;
    }
    return metrics;
}

void MySQLStore::flushThreadFunc() {
    while (!flushShutdown_) { std::this_thread::sleep_for(std::chrono::milliseconds(batchIntervalMs_)); if (flushShutdown_) break; flushBatch(); }
}

void MySQLStore::flushBatch() {
    std::vector<std::pair<std::string, DataPoint>> toFlush;
    { std::lock_guard<std::mutex> lock(batchMutex_); if (batchBuffer_.empty()) return; toFlush.swap(batchBuffer_); }
    ConnectionGuard guard(ConnectionPool::getInstance().getConnection());
    if (!guard) { std::lock_guard<std::mutex> lock(batchMutex_); for (auto& item : toFlush) batchBuffer_.push_back(std::move(item)); return; }
    std::ostringstream sql;
    sql << "INSERT INTO data_points (device_id, timestamp, metrics) VALUES ";
    bool first = true;
    for (const auto& [deviceId, point] : toFlush) {
        if (!first) sql << ", ";
        first = false;
        std::string escapedDeviceId = guard->escapeString(deviceId);
        std::string metricsJson = metricsToJson(point.metrics);
        std::string escapedMetrics = guard->escapeString(metricsJson);
        sql << "('" << escapedDeviceId << "', " << point.timestamp << ", '" << escapedMetrics << "')";
    }
    if (!guard->execute(sql.str())) { std::lock_guard<std::mutex> lock(batchMutex_); for (auto& item : toFlush) batchBuffer_.push_back(std::move(item)); }
}
