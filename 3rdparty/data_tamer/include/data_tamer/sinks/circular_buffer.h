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
    bool full = false;

    explicit CircularBuffer(size_t size) : buffer(size) {}

    void push(const T &item)
    {
        buffer[tail] = item;
        tail = (tail + 1) % buffer.size();
        if (tail == head)
        {
            full = true;
        }
    }

    T pop()
    {
        T item = buffer[head];
        head = (head + 1) % buffer.size();
        full = false;
        return item;
    }
};

#endif // MUJOCO_CIRCULARBUFFER_H
