//
// Created by liuqiang on 25-5-10.
//
#include <data_tamer/data_tamer.hpp>
#include <data_tamer/sinks/dummy_sink.hpp>
#include <data_tamer_parser/data_tamer_parser.hpp>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <Logging.h>
#include <Eigen/Dense>

struct Pose
{
    Eigen::Vector3d pos;
    Eigen::Matrix3d rot;
    double timestamp;
};

//template <typename AddField>
//std::string_view TypeDefinition(Eigen::Vector3d &pos, AddField &add)
//{
//    add("x", &pos[0]);
//    add("y", &pos[1]);
//    add("z", &pos[2]);
//    return "Eigen::Vector3d";
//}
//
////--------------------------------------------------------------
//// We must specialize the function TypeDefinition
//// This must be done in the same namespace of the original type
//
//template <typename AddField>
//std::string_view TypeDefinition(Eigen::Matrix3d &rot, AddField &add)
//{
//    add("r00", &rot(0,0));
//    add("r01", &rot(0,1));
//    add("r02", &rot(0,2));
//    add("r10", &rot(1,0));
//    add("r11", &rot(1,1));
//    add("r12", &rot(1,2));
//    add("r20", &rot(2,0));
//    add("r21", &rot(2,1));
//    add("r22", &rot(2,2));
//    return "Eigen::Matrix3d";
//}

namespace Eigen
{
template <typename AddField>
std::string_view TypeDefinition(Eigen::Vector3d &pos, AddField &add)
{
    add("x", &pos[0]);
    add("y", &pos[1]);
    add("z", &pos[2]);
    return "Eigen::Vector3d";
}

template <typename AddField>
std::string_view TypeDefinition(Eigen::Matrix3d &rot, AddField &add)
{
    add("r00", &rot(0,0));
    add("r01", &rot(0,1));
    add("r02", &rot(0,2));
    add("r10", &rot(1,0));
    add("r11", &rot(1,1));
    add("r12", &rot(1,2));
    add("r20", &rot(2,0));
    add("r21", &rot(2,1));
    add("r22", &rot(2,2));
    return "Eigen::Matrix3d";
}

template <typename AddField>
std::string_view TypeDefinition(Eigen::Quaterniond &qua, AddField &add)
{
    add("w", &qua.w());
    add("x", &qua.x());
    add("y", &qua.y());
    add("z", &qua.z());
    return "Eigen::Quaterniond";
}
} // namespace Eigen

template <typename AddField>
std::string_view TypeDefinition(Pose &pose, AddField &add)
{
    add("pos", &pose.pos);
    add("rot", &pose.rot);
    return "Pose";
}

TEST(DataTamerParser, ReadSchema)
{
    auto ns = std::chrono::nanoseconds(1500000); // 1,500,000 纳秒
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(ns);
    CLOG_INFO << ms.count() << " ms"; // 输出：1 ms
    CLOG_INFO << std::chrono::duration<double, std::milli>(ns).count() << " ms"; // 输出：1.5 ms
}

