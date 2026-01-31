#include "Logger.hpp"

#include <iostream>
#include <chrono>
#include <iomanip>

std::mutex Logger::mtx_;
std::ofstream Logger::ofs_;

static const char* levelToString(LogLevel level) {
    switch (level) {
    case LogLevel::DEBUG: return "DEBUG";
    case LogLevel::INFO:  return "INFO";
    case LogLevel::WARN:  return "WARN";
    case LogLevel::ERROR: return "ERROR";
    default:              return "UNKNOWN";
    }
}

void Logger::init(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mtx_);
    ofs_.open(filename, std::ios::app);
    if (!ofs_) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
    }
}

void Logger::log(LogLevel level, const std::string& msg) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_time{};
#if defined(_WIN32)
    localtime_s(&tm_time, &t);
#else
    localtime_r(&t, &tm_time);
#endif

    std::ostream& out = ofs_.is_open() ? ofs_ : std::cerr;
    out << std::put_time(&tm_time, "%Y-%m-%d %H:%M:%S")
        << " [" << levelToString(level) << "] "
        << msg << std::endl;
}

