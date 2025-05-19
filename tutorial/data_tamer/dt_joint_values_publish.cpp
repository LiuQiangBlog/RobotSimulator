//
// Created by liuqiang on 25-5-17.
//
#include <data_tamer/data_tamer.hpp>
#include <data_tamer/sinks/publish_sink.hpp>
#include <data_tamer/sinks/type_definition.hpp>
#include <Eigen/Dense>
#include <thread>
#include <unordered_map>
#include <unordered_set>

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
//    std::unordered_set<std::string> set1, set2;
//    set1.insert("a");
//    set1.insert("b");
//    set2.insert("a");
//    set2.insert("b");
//    if (set1 != set2)
//    {
//        CLOG_INFO << "set1 != set2";
//    }
//    else
//    {
//        CLOG_INFO << "set1 == set2";
//    }
//    std::vector<double> q(7);
    std::array<double, 7> q;
    Eigen::Vector3d pos;
    Eigen::Matrix3d mat;
    Pose pose;

    channel->registerValue("Joint", &q);//标准化命名
    channel->registerValue("Pos", &pos);
    channel->registerValue("Rot", &mat);
    channel->registerValue("Pose", &pose);
    while (true)
    {
        auto res = Eigen::VectorXd::Random(7);
//        q.assign(res.begin(), res.end());
        std::copy(res.begin(), res.end(), q.begin());
        pos.setRandom();
        mat.setRandom();
        pose.pos.setRandom();
        pose.rot.setRandom();
        channel->takeSnapshot();
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
    return 0;
}