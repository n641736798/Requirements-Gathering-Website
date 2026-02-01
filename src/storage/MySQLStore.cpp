#include "MySQLStore.hpp"
#include "utils/Logger.hpp"
#include <sstream>
#include <cstring>

MySQLStore::MySQLStore() : initialized_(false) {
}

MySQLStore::~MySQLStore() { shutdown(); }

bool MySQLStore::init(const MySQLConfig& config, const PoolConfig& poolConfig) {
    if (initialized_) { LOG_WARNING("MySQLStore already initialized"); return true; }
    if (!ConnectionPool::getInstance().init(config, poolConfig)) { LOG_ERROR("Failed to initialize connection pool"); return false; }
    initialized_ = true;
    LOG_INFO("MySQLStore initialized");
    return true;
}

void MySQLStore::shutdown() {
    if (!initialized_) return;
    ConnectionPool::getInstance().shutdown();
    initialized_ = false;
    LOG_INFO("MySQLStore shutdown");
}

void MySQLStore::appendRequirement(const Requirement& req) {
    if (!initialized_) { LOG_ERROR("MySQLStore not initialized"); return; }
    ConnectionGuard guard(ConnectionPool::getInstance().getConnection());
    if (!guard) { LOG_ERROR("Failed to get connection"); return; }

    std::string escapedTitle = guard->escapeString(req.title);
    std::string escapedContent = guard->escapeString(req.content);
    std::string escapedContact = guard->escapeString(req.contact);
    std::string escapedNotes = guard->escapeString(req.notes);

    std::ostringstream sql;
    sql << "INSERT INTO requirements (title, content, willing_to_pay, contact, notes) VALUES (";
    sql << "'" << escapedTitle << "', '" << escapedContent << "', ";
    if (req.willing_to_pay < 0) {
        sql << "NULL";
    } else {
        sql << req.willing_to_pay;
    }
    sql << ", ";
    if (req.contact.empty()) {
        sql << "NULL";
    } else {
        sql << "'" << escapedContact << "'";
    }
    sql << ", ";
    if (req.notes.empty()) {
        sql << "NULL";
    } else {
        sql << "'" << escapedNotes << "'";
    }
    sql << ")";

    if (!guard->execute(sql.str())) LOG_ERROR("Failed to insert requirement");
}

RequirementQueryResult MySQLStore::queryRequirements(int page, int limit,
    int willingToPay, const std::string& keyword) const {
    RequirementQueryResult result;
    result.page = page;
    result.limit = limit;

    if (!initialized_) { LOG_ERROR("MySQLStore not initialized"); return result; }
    ConnectionGuard guard(ConnectionPool::getInstance().getConnection());
    if (!guard) return result;

    std::ostringstream whereClause;
    bool hasWhere = false;
    if (willingToPay >= 0) {
        if (hasWhere) whereClause << " AND ";
        if (willingToPay == 2) {
            whereClause << "willing_to_pay IS NULL";
        } else {
            whereClause << "willing_to_pay = " << willingToPay;
        }
        hasWhere = true;
    }
    if (!keyword.empty()) {
        if (hasWhere) whereClause << " AND ";
        std::string escapedKeyword = guard->escapeString(keyword);
        whereClause << "(title LIKE '%" << escapedKeyword << "%' OR content LIKE '%" << escapedKeyword << "%'";
        hasWhere = true;
    }
    std::string whereStr = hasWhere ? ("WHERE " + whereClause.str()) : "";

    std::ostringstream countSql;
    countSql << "SELECT COUNT(*) FROM requirements " << whereStr;
    MYSQL_RES* countRes = guard->query(countSql.str());
    if (countRes) {
        MYSQL_ROW row = mysql_fetch_row(countRes);
        if (row && row[0]) result.total = std::stoll(row[0]);
        mysql_free_result(countRes);
    }

    int offset = (page - 1) * limit;
    if (offset < 0) offset = 0;

    std::ostringstream dataSql;
    dataSql << "SELECT id, title, content, willing_to_pay, contact, notes, created_at, updated_at ";
    dataSql << "FROM requirements " << whereStr << " ";
    dataSql << "ORDER BY created_at DESC LIMIT " << limit << " OFFSET " << offset;

    MYSQL_RES* res = guard->query(dataSql.str());
    if (!res) return result;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(res);
        Requirement r;
        r.id = row[0] ? std::stoll(row[0]) : 0;
        r.title = row[1] && lengths[1] > 0 ? std::string(row[1], lengths[1]) : "";
        r.content = row[2] && lengths[2] > 0 ? std::string(row[2], lengths[2]) : "";
        r.willing_to_pay = row[3] ? std::stoi(row[3]) : -1;
        r.contact = row[4] && lengths[4] > 0 ? std::string(row[4], lengths[4]) : "";
        r.notes = row[5] && lengths[5] > 0 ? std::string(row[5], lengths[5]) : "";
        r.created_at = row[6] && lengths[6] > 0 ? std::string(row[6], lengths[6]) : "";
        r.updated_at = row[7] && lengths[7] > 0 ? std::string(row[7], lengths[7]) : "";
        result.data.push_back(r);
    }
    mysql_free_result(res);
    return result;
}

bool MySQLStore::deviceExists(const std::string& deviceId) const {
    if (!initialized_) { LOG_ERROR("MySQLStore not initialized"); return false; }
    ConnectionGuard guard(ConnectionPool::getInstance().getConnection());
    if (!guard) return false;

    std::string escapedId = guard->escapeString(deviceId);
    std::ostringstream sql;
    sql << "SELECT 1 FROM device_data.devices WHERE device_id = '" << escapedId << "' LIMIT 1";
    MYSQL_RES* res = guard->query(sql.str());
    if (!res) return false;
    bool exists = (mysql_num_rows(res) > 0);
    mysql_free_result(res);
    return exists;
}

void MySQLStore::ensureDeviceRegistered(const std::string& deviceId) {
    if (!initialized_) { LOG_ERROR("MySQLStore not initialized"); return; }
    ConnectionGuard guard(ConnectionPool::getInstance().getConnection());
    if (!guard) { LOG_ERROR("Failed to get connection"); return; }

    std::string escapedId = guard->escapeString(deviceId);
    std::ostringstream sql;
    sql << "INSERT IGNORE INTO device_data.devices (device_id) VALUES ('" << escapedId << "')";
    if (!guard->execute(sql.str())) LOG_ERROR("Failed to ensure device registered");
}
