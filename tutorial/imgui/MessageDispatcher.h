//
// Created by liuqiang on 25-5-17.
//

#ifndef MUJOCO_MESSAGEDISPATCHER_H
#define MUJOCO_MESSAGEDISPATCHER_H

#pragma once
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

template<typename T>
class MessageDispatcher {
public:
    using HandlerFn = std::function<void(const T&)>;

    explicit MessageDispatcher(HandlerFn handler)
        : handler_(std::move(handler)), running_(true)
    {
        consumer_thread_ = std::thread(&MessageDispatcher::consumeLoop, this);
    }

    ~MessageDispatcher() {
        stop();
    }

    void push(const T& msg) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            message_queue_.emplace_back(msg);
        }
        queue_cv_.notify_one();
    }

    void stop() {
        running_ = false;
        queue_cv_.notify_one();
        if (consumer_thread_.joinable()) {
            consumer_thread_.join();
        }
    }

private:
    void consumeLoop() {
        while (running_) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [&]() {
                               return !message_queue_.empty() || !running_;
                           });

            while (!message_queue_.empty()) {
                T msg = std::move(message_queue_.front());
                message_queue_.pop_front();
                lock.unlock(); // ✅ unlock while processing to reduce contention
                handler_(msg);
                lock.lock();   // 🔁 lock again for next message
            }
        }
    }

    std::deque<T> message_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    HandlerFn handler_;
    std::atomic<bool> running_;
    std::thread consumer_thread_;
};

#endif // MUJOCO_MESSAGEDISPATCHER_H
