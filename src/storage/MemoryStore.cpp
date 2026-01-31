#include "MemoryStore.hpp"
#include <mutex>

void MemoryStore::append(const std::string& deviceId, const DataPoint& point) {
    std::unique_lock<std::shared_mutex> lock(mtx_);
    auto& series = data_[deviceId];
    series.push_back(point);
}

std::vector<DataPoint> MemoryStore::queryLatest(const std::string& deviceId, std::size_t limit) const {
    std::shared_lock<std::shared_mutex> lock(mtx_);
    std::vector<DataPoint> result;
    auto it = data_.find(deviceId);
    if (it == data_.end()) {
        return result;
    }
    const auto& series = it->second;
    if (series.empty()) {
        return result;
    }

    std::size_t n = series.size();
    std::size_t start = (n > limit) ? (n - limit) : 0;
    result.reserve(n - start);
    for (std::size_t i = start; i < n; ++i) {
        result.push_back(series[i]);
    }
    return result;
}

