//
// Created by liuqiang on 25-5-9.
//
#include "Viewer.h"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <data_tamer/sinks/publish_sink.hpp>

std::mutex mtx;
std::condition_variable cv;

int main()
{
    zcm::RegisterAllPlugins();
    std::string file("/home/liuqiang/CLionProjects/RobotControlAlgorithms/mujoco/tora_one/scene.xml");
//        std::string file("/home/liuqiang/PycharmProjects/mink/examples/kuka_iiwa_14/scene.xml");
    char err[1000];
    mjModel *model = mj_loadXML(file.c_str(), nullptr, err, 1000);
    if (!model)
    {
        CLOG_ERROR << err;
        return -1;
    }
    mjData *data = mj_makeData(model);
    if (!data)
    {
        mj_deleteModel(model);
        return -1;
    }

    Viewer viewer(model, data);
    if (!viewer.init())
    {
        return -1;
    }
    viewer.addFunction(
        [&]()
        {
            viewer.showJointSliders(mtx);
        });

    model->opt.timestep = 0.001; // 1ms
    mj_forward(model, data);
    viewer.setBodyVisible("table", false);
    viewer.showMocapGizmo("target");
    viewer.drawBodyFrame("table");
    auto channel = DataTamer::LogChannel::create("channel");
    auto sink = std::make_shared<DataTamer::PlotSink>();
    channel->addDataSink(sink);

    auto publisher = std::make_shared<DataTamer::PublishSink>();
    if (!publisher->init())
    {
        return -1;
    }
    channel->addDataSink(publisher);

    viewer.addFunction(
        [&]()
        {
            viewer.plotChannelData("pos/x", sink);
            viewer.plotChannelData("pos/y", sink);
            viewer.plotChannelData("pos/z", sink);
        });

    std::atomic<bool> exit{false};
    auto control = [&]()
    {
        Eigen::Vector3d tip;
        channel->registerValue("pos", &tip);
        CLOG_INFO << channel->getSchema();
        while (!exit.load(std::memory_order_relaxed))
        {
            auto loop_start_time = std::chrono::high_resolution_clock::now();
            // todo, here implement your control strategy

            mj_step(model, data);

            auto bodyId = mj_name2id(model, mjOBJ_BODY, "target");
            if (bodyId != -1)
            {
                std::lock_guard<std::mutex> lck(mtx);
                Eigen::Map<Eigen::Vector3d> pos(data->xpos + bodyId * 3, 3);
                tip = pos;
            }
            channel->takeSnapshot();

            auto loop_end_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> loop_duration = loop_end_time - loop_start_time;
            if (loop_duration.count() < model->opt.timestep)
            {
                std::this_thread::sleep_for(std::chrono::duration<double>(model->opt.timestep - loop_duration.count()));
            }
        }
    };
    std::thread th(control);

    while (!viewer.shouldClose())
    {
        viewer.render();
    }
    exit = true;
    th.join();

    mj_deleteData(data);
    mj_deleteModel(model);
    glfwTerminate();

    return 0;
}