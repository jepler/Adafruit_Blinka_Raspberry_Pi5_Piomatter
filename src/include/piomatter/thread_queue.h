#pragma once
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

namespace piomatter {

template <class T> struct thread_queue {
    thread_queue() : queue{}, mutex{}, cv{} {}

    void push(T t) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(t);
        cv.notify_one();
    }

    std::optional<T> pop_nonblocking() {
        std::unique_lock<std::mutex> lock(mutex);
        if (queue.empty()) {
            return {};
        }
        T val = queue.front();
        queue.pop();
        return val;
    }

    T pop_blocking() {
        std::unique_lock<std::mutex> lock(mutex);
        while (queue.empty()) {
            cv.wait(lock);
        }
        T val = queue.front();
        queue.pop();
        return val;
    }

  private:
    std::queue<T> queue;
    std::mutex mutex;
    std::condition_variable cv;
};

} // namespace piomatter
