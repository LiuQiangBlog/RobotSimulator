//
// Created by liuqiang on 25-5-17.
//
#include <data_tamer/data_tamer.hpp>
#include <data_tamer/sinks/publish_sink.hpp>
#include <data_tamer/sinks/plot_sink.hpp>
#include <Eigen/Dense>
#include <thread>

int main()
{
    zcm::RegisterAllPlugins();
    auto channel = DataTamer::LogChannel::create("channel1");
    auto publisher = std::make_shared<DataTamer::PublishSink>();
    if (!publisher->init())
    {
        return -1;
    }
    channel->addDataSink(publisher);

    std::vector<double> q(7);
    channel->registerValue("Joint", &q);
    while (true)
    {
        auto res = Eigen::VectorXd::Random(7);
        q.assign(res.begin(), res.end());
        channel->takeSnapshot();
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
    return 0;
}