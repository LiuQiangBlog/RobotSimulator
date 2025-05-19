//
// Created by liuqiang on 25-5-18.
//

#ifndef MUJOCO_TYPE_DEFINITION_HPP
#define MUJOCO_TYPE_DEFINITION_HPP

#include <Eigen/Dense>

namespace Eigen
{
template <typename AddField>
std::string_view TypeDefinition(Eigen::Vector3d &pos, AddField &add)
{
    add("x", &pos[0]); // define first field nane
    add("y", &pos[1]); // define second field nane
    add("z", &pos[2]); // define third field nane
    return "Eigen::Vector3d"; // define type name
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

template <typename AddField>
std::string_view TypeDefinition(Eigen::VectorXd &vec, AddField &add) {
    add("data", vec.data(), vec.size());
    return "Eigen::VectorXd";
}

template <typename T, typename AddField>
std::string_view TypeDefinition(T& value, AddField& add) {
    if constexpr (std::is_same_v<T, Eigen::VectorXd>) {
        add("data", value.data(), value.size());
        return "Eigen::VectorXd";
    } else {
        static_assert(sizeof(T) == 0, "Unsupported type in TypeDefinition");
    }
}

} // namespace Eigen

struct Pose
{
    Eigen::Vector3d pos;
    Eigen::Matrix3d rot;
    double timestamp;
};

template <typename AddField>
std::string_view TypeDefinition(Pose &pose, AddField &add)
{
    add("pos", &pose.pos);
    add("rot", &pose.rot);
    return "Pose";
}

#endif // MUJOCO_TYPE_DEFINITION_HPP
