//
// Created by liuqiang on 25-5-10.
//
#include <data_tamer/data_tamer.hpp>
#include <data_tamer/sinks/plot_sink.hpp>
#include <data_tamer_parser/data_tamer_parser.hpp>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <Logging.h>
#include <Eigen/Dense>

TEST(DataTamerParser, ReadSchema)
{
    auto ns = std::chrono::nanoseconds(1500000); // 1,500,000 纳秒
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(ns);
    CLOG_INFO << ms.count() << " ms"; // 输出：1 ms
    CLOG_INFO << std::chrono::duration<double, std::milli>(ns).count() << " ms"; // 输出：1.5 ms

    auto channel = DataTamer::LogChannel::create("channel");
    auto sink = std::make_shared<DataTamer::PlotSink>();
    channel->addDataSink(sink);

    std::vector<double> valsA = {10, 11, 12};
    std::array<int, 2> valsB = {13, 14};

    std::array<Eigen::Vector3d, 3> points;
    points[0] = {1, 2, 3};
    points[1] = {4, 5, 6};
    points[2] = {7, 8, 9};

    std::vector<Eigen::Matrix3d> rots(2);
    rots[0] = Eigen::Matrix3d::Identity();
    rots[1] = Eigen::Matrix3d::Zero();

    std::vector<Eigen::Quaterniond> quats(2);
    quats[0] = {20, 21, 22, 23};
    quats[1] = {30, 31, 32, 33};

    channel->registerValue("valsA", &valsA);
    channel->registerValue("valsB", &valsB);
    channel->registerValue("points", &points);
    channel->registerValue("rots", &rots);
    channel->registerValue("quats", &quats);

    CLOG_INFO << channel->getSchema();

    channel->takeSnapshot();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    for (const auto &[name, value] : sink->channel_data)
    {
        std::cout << "timestamp: " << std::endl;
        for (auto &item : value.first)
        {
            std::cout << item << std::endl;
        }
        for (auto &item : value.second)
        {
            std::cout << "name: " << name << ", value: " << item << std::endl;
        }
    }

}

