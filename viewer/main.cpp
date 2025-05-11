//
// Created by liuqiang on 25-5-9.
//
#include "Viewer.h"
#include <mutex>
#include <condition_variable>
#include <thread>

std::mutex mtx;
std::condition_variable cv;

int main()
{
    std::string file("/home/liuqiang/CLionProjects/RobotControlAlgorithms/mujoco/tora_one/scene.xml");
    //    std::string file("/home/liuqiang/PycharmProjects/mink/examples/kuka_iiwa_14/scene.xml");
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

    std::atomic<bool> exit{false};
    auto control = [&]()
    {
        while (!exit.load(std::memory_order_relaxed))
        {
            // todo, implement your control strategy
        }
    };
    std::thread th(control);

    while (!viewer.shouldClose())
    {
        mj_step(model, data);
        viewer.render();
    }
    exit = true;
    th.join();

    mj_deleteData(data);
    mj_deleteModel(model);
    glfwTerminate();

    return 0;
}