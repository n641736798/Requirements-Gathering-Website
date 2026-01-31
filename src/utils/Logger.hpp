#pragma once

#include <string>
#include <mutex>
#include <fstream>

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger {
public:
    static void init(const std::string& filename);
    static void log(LogLevel level, const std::string& msg);

private:
    static std::mutex mtx_;
    static std::ofstream ofs_;
};

#define LOG_DEBUG(msg) Logger::log(LogLevel::DEBUG, msg)
#define LOG_INFO(msg)  Logger::log(LogLevel::INFO, msg)
#define LOG_WARN(msg)  Logger::log(LogLevel::WARN, msg)
#define LOG_WARNING(msg) Logger::log(LogLevel::WARN, msg)
#define LOG_ERROR(msg) Logger::log(LogLevel::ERROR, msg)
