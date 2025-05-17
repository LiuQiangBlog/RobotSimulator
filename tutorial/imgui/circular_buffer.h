//
// Created by liuqiang on 25-5-17.
//

#ifndef MUJOCO_CIRCULARBUFFER_H
#define MUJOCO_CIRCULARBUFFER_H

#include <vector>

template <typename T>
class CircularBuffer
{
public:
    std::vector<T> buffer;
    size_t head = 0, tail = 0;

    explicit CircularBuffer(std::size_t N) : buffer(N) {}

    [[nodiscard]] bool is_full() const
    {
        return (tail + 1) % buffer.size() == head;
    }

    [[nodiscard]] bool is_empty() const
    {
        return head == tail;
    }

    void push(const T &item)
    {
        if (!is_full())
        {
            buffer[tail] = item;
            tail = (tail + 1) % buffer.size();
        }
        else
        {
            pop();
            push(item);
        }
    }

    void pop()
    {
        if (!is_empty())
        {
            T item = buffer[head];
            head = (head + 1) % buffer.size();
        }
    }
};

#endif // MUJOCO_CIRCULARBUFFER_H
