//
// Created by liuqiang on 25-5-17.
//

#ifndef MUJOCO_ROLLING_BUFFER_H
#define MUJOCO_ROLLING_BUFFER_H

#include <vector>
#include <algorithm>
#include <mutex>
#include <optional>

template <typename T>
class RollingBuffer
{
public:
    explicit RollingBuffer(size_t max_size) : max_size_(max_size), active_buffer_(0)
    {
        buffers_[0].reserve(max_size_);
        buffers_[1].reserve(max_size_);
    }

    void push_back(T value)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto &current = buffers_[active_buffer_];
        if (current.size() >= max_size_)
        {
            // 切换到备用缓冲区
            active_buffer_ = !active_buffer_;
            auto &next = buffers_[active_buffer_];

            // 移动当前缓冲区的后 max_size-1 个元素到备用缓冲区
            // 并添加新值
            next.assign(current.begin() + 1, current.end());
            next.push_back(value);

            // 清空原缓冲区（保留容量）
            current.clear();
        }
        else
        {
            current.push_back(value);
        }
    }

    std::vector<T> data() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return buffers_[active_buffer_];
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return buffers_[active_buffer_].size();
    }

    size_t capacity() const
    {
        return max_size_;
    }

private:
    const size_t max_size_;
    mutable std::mutex mutex_;
    int active_buffer_;
    std::vector<T> buffers_[2];
};

#endif // MUJOCO_ROLLING_BUFFER_H
