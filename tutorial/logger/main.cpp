//
// Created by liuqiang on 25-5-15.
//
#include "ChannelLogger.h"
#include <Eigen/Dense>

int main()
{
    Eigen::Vector3d tip;
    ChannelLogger logger("channel");
    logger.registerValue("pos", &tip);
}