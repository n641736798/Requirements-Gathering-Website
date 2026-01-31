#include "MemoryStore.hpp"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

static std::string getCurrentDateTime() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

static bool matchesKeyword(const Requirement& req, const std::string& keyword) {
    if (keyword.empty()) return true;
    std::string kw = keyword;
    std::string t = req.title;
    std::string c = req.content;
    std::transform(kw.begin(), kw.end(), kw.begin(), ::tolower);
    std::transform(t.begin(), t.end(), t.begin(), ::tolower);
    std::transform(c.begin(), c.end(), c.begin(), ::tolower);
    return t.find(kw) != std::string::npos || c.find(kw) != std::string::npos;
}

static bool matchesWillingToPay(const Requirement& req, int filter) {
    if (filter < 0) return true;
    if (filter == 2) return req.willing_to_pay < 0;  // 2=空/未填
    return req.willing_to_pay == filter;
}

void MemoryStore::appendRequirement(const Requirement& req) {
    std::unique_lock<std::shared_mutex> lock(mtx_);
    Requirement r = req;
    r.id = static_cast<int64_t>(data_.size()) + 1;
    r.created_at = getCurrentDateTime();
    r.updated_at = r.created_at;
    data_.push_back(r);
}

RequirementQueryResult MemoryStore::queryRequirements(int page, int limit,
    int willingToPay, const std::string& keyword) const {
    std::shared_lock<std::shared_mutex> lock(mtx_);

    std::vector<Requirement> filtered;
    for (const auto& r : data_) {
        if (matchesWillingToPay(r, willingToPay) && matchesKeyword(r, keyword)) {
            filtered.push_back(r);
        }
    }

    std::sort(filtered.begin(), filtered.end(),
        [](const Requirement& a, const Requirement& b) { return a.id > b.id; });

    RequirementQueryResult result;
    result.total = static_cast<int64_t>(filtered.size());
    result.page = page;
    result.limit = limit;

    int offset = (page - 1) * limit;
    if (offset < 0) offset = 0;
    if (static_cast<size_t>(offset) >= filtered.size()) {
        return result;
    }

    int end = offset + limit;
    if (end > static_cast<int>(filtered.size())) end = static_cast<int>(filtered.size());

    for (int i = offset; i < end; ++i) {
        result.data.push_back(filtered[i]);
    }
    return result;
}
