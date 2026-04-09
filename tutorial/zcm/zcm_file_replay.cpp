//
// Created by liuqiang on 25-5-13.
//
#include <zcm/zcm-cpp.hpp>
#include "types/example_t.hpp"
#include <iostream>

class Handler
{
public:
    //最常用 / 最佳实践: 类成员函数 + 自动消息解码
    //面向对象：绑定类成员函数，可在类里保存状态、变量
    //类型安全：编译期检查消息类型，不会接错消息
    //工程最常用：机器人 / 自动驾驶 / ZCM 官方示例首选
    //适用场景: 90% 的业务开发、多消息处理、需要保存状态的订阅逻辑
    //想自己重新解码/想记录原始数据大小/想做消息转发/想做日志、性能统计: const ReceiveBuffer* rbuf 就有用了
    //cout << "消息大小：" << rbuf->data_size << " 字节" << endl;
    //一个回调处理多个通道 → chan 就有用了: 一个回调监听多个通道，必须用 chan 判断
    void handle(const zcm::ReceiveBuffer *rbuf, const std::string &chan, const example_t *msg)
    {
        std::cout << "[回放] 接收到消息: " << msg->name << " 时间戳: " << msg->timestamp << " 位置: ("
                  << msg->position[0] << ", " << msg->position[1] << ", " << msg->position[2] << ")" << std::endl;
    }
};

int main()
{
    zcm::RegisterAllPlugins();
    zcm::ZCM zcm("udpm://239.255.76.67:7667?ttl=1,file:///home/liuqiang/log.zcm?mode=r");

    if (!zcm.good())
    {
        std::cerr << "ZCM 初始化失败（file://）" << std::endl;
        return 1;
    }

    Handler h;
    //类成员 + 自动解码
    zcm.subscribe("EXAMPLE", &Handler::handle, &h);
    //Lambda + 自动解码
    //极简代码：不用写类、不用写 Handler
    //就地处理：逻辑写在订阅处，可读性极高
    //自动解码：同样直接拿到 Msg*
    //现代 C++ 风格
    zcm.subscribe<example_t>("EXAMPLE",
                             [](auto &&, auto &&, const example_t *msg)
                             {
                                 std::cout << msg->name << std::endl;
                             });
    zcm.subscribe("EXAMPLE",
                  [](auto &&, auto &&)
                  {
                      // 自己解析二进制
                  });
    //zcm.subscribe<example_t>("EXAMPLE", c_func, nullptr);
    //zcm.subscribe("EXAMPLE", c_func_raw, nullptr);

    std::cout << "开始回放..." << std::endl;
    zcm.run(); // 自动播放文件中的所有消息

    return 0;
}