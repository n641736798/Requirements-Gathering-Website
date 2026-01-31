#include "ThreadPool.hpp"
#include "utils/Logger.hpp"

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::start(std::size_t threadCount) {
    if (running_) return;
    running_ = true;
    activeTasks_ = 0;
    for (std::size_t i = 0; i < threadCount; ++i) {
        workers_.emplace_back(&ThreadPool::workerLoop, this, i);
    }
}

void ThreadPool::stop() {
    if (!running_) return;
    running_ = false;

    // 向队列中塞入空任务以唤醒线程并退出
    for (std::size_t i = 0; i < workers_.size(); ++i) {
        taskQueue_.push(nullptr);
    }

    for (auto& t : workers_) {
        if (t.joinable()) {
            t.join();
        }
    }
    workers_.clear();
}

void ThreadPool::waitForTasks() {
    std::unique_lock<std::mutex> lock(waitMtx_);
    // 等待队列为空且没有正在执行的任务
    waitCv_.wait(lock, [this] {
        return taskQueue_.empty() && activeTasks_ == 0;
    });
}

void ThreadPool::submit(Task task) {
    if (!running_) return;
    taskQueue_.push(std::move(task));
}

void ThreadPool::workerLoop(std::size_t threadIndex) {
    while (running_) {
        Task task = taskQueue_.take();
        if (!task) {
            // 退出信号
            continue;
        }
        
        // 增加正在执行的任务计数
        ++activeTasks_;
        LOG_DEBUG("ThreadPool worker #" + std::to_string(threadIndex) + " executing task");
        
        try {
            task();
        } catch (...) {
            // 捕获异常，确保计数正确减少
        }
        
        // 减少正在执行的任务计数
        --activeTasks_;
        
        // 通知等待的线程
        {
            std::lock_guard<std::mutex> lock(waitMtx_);
            waitCv_.notify_all();
        }
    }
}

