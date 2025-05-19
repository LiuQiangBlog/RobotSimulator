//
// Created by liuqiang on 25-5-19.
//

#include "DataTamerViewer.hpp"

int main(int, char **)
{
    Plotter pt;
    if (!pt.init())
    {
        return -1;
    }
    pt.plot("Pose*");
    pt.plot("Rot*");
    pt.plot("Joint/0-3");
    pt.plot("Pos/*");
    pt.plot("pos/*");
    while (!pt.shouldClose())
    {
        pt.render();
    }
    return 0;
}