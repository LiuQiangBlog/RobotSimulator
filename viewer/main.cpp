//
// Created by liuqiang on 25-5-9.
//
#include "Viewer.h"
#include <mutex>
#include <condition_variable>

std::mutex mtx;
std::condition_variable cv;

int main()
{
    std::string file("/home/liuqiang/CLionProjects/RobotControlAlgorithms/mujoco/tora_one/scene.xml");
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
    model->opt.timestep = 0.001; // 1ms
    mj_forward(model, data);
    viewer.setBodyVisible("table", true);
    while (!viewer.shouldClose())
    {
        mj_step(model, data);
        viewer.render();
    }

    mj_deleteData(data);
    mj_deleteModel(model);
    glfwTerminate();

    return 0;
}