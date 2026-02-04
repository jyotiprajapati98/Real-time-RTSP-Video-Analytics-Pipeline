#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

template <typename T>
class SafeQueue {
public:
    SafeQueue() : stop_flag(false) {}

    void push(T value) {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(std::move(value));
        cond.notify_one();
    }

    // Returns true if value was retrieved, false if queue is stopped and empty
    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(mtx);
        cond.wait(lock, [this] { return !queue.empty() || stop_flag; });
        
        if (queue.empty() && stop_flag) {
            return false;
        }
        
        value = std::move(queue.front());
        queue.pop();
        return true;
    }

    // Non-blocking peek/pop logic if needed, or simple size check
    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.size();
    }

    void stop() {
        std::lock_guard<std::mutex> lock(mtx);
        stop_flag = true;
        cond.notify_all();
    }

    // Clear queue, useful for dropping frames if lagging
    void clear() {
         std::lock_guard<std::mutex> lock(mtx);
         std::queue<T> empty;
         std::swap(queue, empty);
    }

private:
    std::queue<T> queue;
    mutable std::mutex mtx;
    std::condition_variable cond;
    bool stop_flag;
};
