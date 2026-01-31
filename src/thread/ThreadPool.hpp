#pragma once

#include <vector>
#include <thread>
#include <functional>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include "BlockingQueue.hpp"

class ThreadPool {
public:
    using Task = std::function<void()>;

    ThreadPool() = default;
    ~ThreadPool();

    void start(std::size_t threadCount);
    void stop();
    
    // 等待所有任务完成（包括队列中的和正在执行的）
    void waitForTasks();

    void submit(Task task);

private:
    void workerLoop(std::size_t threadIndex);

private:
    std::vector<std::thread> workers_;
    BlockingQueue<Task> taskQueue_;
    std::atomic<bool> running_{false};
    std::atomic<std::size_t> activeTasks_{0};  // 正在执行的任务数
    std::mutex waitMtx_;  // 用于等待任务完成的互斥锁
    std::condition_variable waitCv_;  // 用于通知任务完成的条件变量
};

