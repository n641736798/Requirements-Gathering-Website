#include "DeviceManager.hpp"
#include "storage/MySQLStore.hpp"
#include "utils/Logger.hpp"

DeviceManager::DeviceManager(DeviceManagerMode mode)
    : mode_(mode)
    , mysqlStore_(nullptr) {
}

void DeviceManager::setMySQLStore(MySQLStore* store) {
    mysqlStore_ = store;
}

bool DeviceManager::exists(const std::string& deviceId) {
    switch (mode_) {
        case DeviceManagerMode::MEMORY: {
            std::lock_guard<std::mutex> lock(mtx_);
            return devices_.find(deviceId) != devices_.end();
        }
        
        case DeviceManagerMode::MYSQL: {
            if (!mysqlStore_) {
                LOG_ERROR("MySQL store not set");
                return false;
            }
            return mysqlStore_->deviceExists(deviceId);
        }
        
        case DeviceManagerMode::HYBRID: {
            // 先检查内存缓存
            {
                std::lock_guard<std::mutex> lock(mtx_);
                if (devices_.find(deviceId) != devices_.end()) {
                    return true;
                }
            }
            // 再检查数据库
            if (mysqlStore_ && mysqlStore_->deviceExists(deviceId)) {
                // 加入内存缓存
                std::lock_guard<std::mutex> lock(mtx_);
                devices_.insert(deviceId);
                return true;
            }
            return false;
        }
        
        default:
            return false;
    }
}

void DeviceManager::ensureRegistered(const std::string& deviceId) {
    switch (mode_) {
        case DeviceManagerMode::MEMORY: {
            std::lock_guard<std::mutex> lock(mtx_);
            devices_.insert(deviceId);
            break;
        }
        
        case DeviceManagerMode::MYSQL: {
            if (!mysqlStore_) {
                LOG_ERROR("MySQL store not set");
                return;
            }
            mysqlStore_->ensureDeviceRegistered(deviceId);
            break;
        }
        
        case DeviceManagerMode::HYBRID: {
            // 先注册到内存
            {
                std::lock_guard<std::mutex> lock(mtx_);
                devices_.insert(deviceId);
            }
            // 再注册到数据库
            if (mysqlStore_) {
                mysqlStore_->ensureDeviceRegistered(deviceId);
            }
            break;
        }
    }
}

std::size_t DeviceManager::getDeviceCount() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return devices_.size();
}

void DeviceManager::clearMemoryCache() {
    std::lock_guard<std::mutex> lock(mtx_);
    devices_.clear();
}
