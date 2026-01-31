#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class BlockingQueue {
public:
    void push(const T& value) {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            queue_.push(value);
        }
        cv_.notify_one();
    }

    T take() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return !queue_.empty(); });
        T value = queue_.front();
        queue_.pop();
        return value;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.empty();
    }

private:
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<T> queue_;
};

