//
// Created by liuqiang on 25-5-19.
//
#pragma once
#include <string_view>

namespace TestTypes
{
struct Point3D
{
    double x = 0;
    double y = 0;
    double z = 0;
};

struct Quaternion
{
    double w = 1;
    double x = 0;
    double y = 0;
    double z = 0;
};

struct Pose
{
    Point3D pos;
    Quaternion rot;
};

} // end namespace TestTypes

namespace PseudoEigen
{

class Vector2d
{
    double _x = 0;
    double _y = 0;

public:
    Vector2d() = default;
    Vector2d(double x, double y) : _x(x), _y(y) {}

    const double &x() const
    {
        return _x;
    }
    const double &y() const
    {
        return _y;
    }

    double &x()
    {
        return _x;
    }
    double &y()
    {
        return _y;
    }
};

} // namespace PseudoEigen

namespace TestTypes
{

template <typename AddField>
std::string_view TypeDefinition(Point3D &point, AddField &add)
{
    add("x", &point.x);
    add("y", &point.y);
    add("z", &point.z);
    return "Point3D";
}

//--------------------------------------------------------------
// We must specialize the function TypeDefinition
// This must be done in the same namespace of the original type

template <typename AddField>
std::string_view TypeDefinition(Quaternion &quat, AddField &add)
{
    add("w", &quat.w);
    add("x", &quat.x);
    add("y", &quat.y);
    add("z", &quat.z);
    return "Quaternion";
}

template <typename AddField>
std::string_view TypeDefinition(Pose &pose, AddField &add)
{
    add("position", &pose.pos);
    add("rotation", &pose.rot);
    return "Pose";
}

} // end namespace TestTypes

namespace PseudoEigen
{
template <typename AddField>
std::string_view TypeDefinition(Vector2d &vect, AddField &add)
{
    add("x", &vect.x());
    add("y", &vect.y());
    return "Vector2d";
}

} // end namespace PseudoEigen


#include "data_tamer/data_tamer.hpp"
#include "data_tamer/sinks/mcap_sink.hpp"
#include <data_tamer/sinks/publish_sink.hpp>
#include <iostream>
#include <thread>

using namespace DataTamer;

int main()
{
    auto mcap_sink = std::make_shared<MCAPSink>("test_sample.mcap");
    ChannelsRegistry::Global().addDefaultSink(mcap_sink);

    // Create (or get) a channel using the global registry (singleton)
    auto channelA = ChannelsRegistry::Global().getChannel("channelA");
    auto channelB = ChannelsRegistry::Global().getChannel("channelB");

    // logs in channelA
    std::vector<double> v1(10, 0);
    std::array<float, 4> v2 = {1, 2, 3, 4};
    int32_t v3 = 5;
    uint16_t v4 = 6;
    TestTypes::Pose pose;
    pose.pos = {1, 2, 3};
    pose.rot = {0.4, 0.5, 0.6, 0.7};

    channelA->registerValue("vector_10", &v1);
    channelA->registerValue("array_4", &v2);
    channelA->registerValue("val_int32", &v3);
    channelA->registerValue("val_int16", &v4);
    channelA->registerValue("pose", &pose);

    // logs in channelB
    double v5 = 10;
    uint16_t v6 = 11;
    std::vector<uint8_t> v7(4, 12);
    std::array<uint32_t, 3> v8 = {13, 14, 15};
    std::vector<TestTypes::Point3D> points(2);
    points[0] = {1, 2, 3};
    points[1] = {4, 5, 6};

    channelB->registerValue("real_value", &v5);
    channelB->registerValue("short_int", &v6);
    channelB->registerValue("vector_4", &v7);
    channelB->registerValue("array_3", &v8);
    channelB->registerValue("points", &points);
    std::cout << channelA->getSchema() << std::endl;
    std::cout << channelB->getSchema() << std::endl;

    for (size_t i = 0; i < 1000; i++)
    {
        channelA->takeSnapshot();
        if (i % 2 == 0)
        {
            channelB->takeSnapshot();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::cout << "DONE" << std::endl;
}

