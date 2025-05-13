//
// Created by liuqiang on 25-5-13.
//
#include <thread>
#include <chrono>
#include <memory>
#include <future>
#include <functional>
#include <iostream>
#include <queue>

using namespace std;
#define TOTAL 1000000

std::mutex m;
std::condition_variable cv;
queue<int> gQueue;

template <class T, class... Args>
auto measure(T &&func, Args &&...args) -> std::future<typename std::result_of<T(Args...)>::type>
{
    using return_type = typename std::result_of<T(Args...)>::type;
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<T>(func), std::forward<Args>(args)...));
    std::future<return_type> res = task->get_future();
    auto begin = std::chrono::high_resolution_clock::now();
    (*task)();
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
    printf("执行时间: % .3f seconds.\n", elapsed.count() * 1e-9);
    return res;
}

void producer_thread()
{
    for (int i = 0; i < TOTAL; i++)
    {
        std::unique_lock<std::mutex> lk(m);
        gQueue.push(i);
        cv.notify_one();
    }
}

void consumer_thread()
{
    int element = 0;
    while (element != TOTAL - 1)
    {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk,
                []
                {
                    return !gQueue.empty();
                });
        element = gQueue.front();
        gQueue.pop();
        printf("element:%d\n", element);
    }
}

int main()
{
    measure(
        []
        {
            thread a(producer_thread);
            thread b(consumer_thread);
            a.join();
            b.join();
        });

    return 0;
}