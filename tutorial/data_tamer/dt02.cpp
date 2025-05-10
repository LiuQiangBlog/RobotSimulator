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

DataTamerParser::SnapshotView ConvertSnapshot(const DataTamer::Snapshot &snapshot)
{
    return {snapshot.schema_hash,
            uint64_t(snapshot.timestamp.count()),
            {snapshot.active_mask.data(), snapshot.active_mask.size()},
            {snapshot.payload.data(), snapshot.payload.size()}};
}

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
    const char *text = R"(
int8 v1
float64 v2
float32[5] array
int32[] vect
bool is_true
char[256] blob
uint16  my/short
  )";
    const auto schema = DataTamerParser::BuilSchemaFromText(text);
    ASSERT_EQ(schema.fields.size(), 7);

    auto channel = DataTamer::LogChannel::create("channel");
    auto dummy_sink = std::make_shared<DataTamer::DummySink>();
    channel->addDataSink(dummy_sink);

    std::vector<double> valsA = {10, 11, 12};
    std::array<int, 2> valsB = {13, 14};

    std::array<Eigen::Vector3d, 3> points;
    points[0] = {1, 2, 3};
    points[1] = {4, 5, 6};
    points[2] = {7, 8, 9};

    std::vector<Eigen::Matrix3d> rots(2);
    rots[0] = Eigen::Matrix3d::Identity();
    rots[1] = Eigen::Matrix3d::Zero();

    std::vector<Eigen::Quaterniond> quats(2);
    quats[0] = {20, 21, 22, 23};
    quats[1] = {30, 31, 32, 33};

    channel->registerValue("valsA", &valsA);
    channel->registerValue("valsB", &valsB);
    channel->registerValue("points", &points);
    channel->registerValue("rots", &rots);
    channel->registerValue("quats", &quats);

    CLOG_INFO << channel->getSchema();

    channel->takeSnapshot();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    const auto &schema_in = channel->getSchema();
    const auto &schema_out = DataTamerParser::BuilSchemaFromText(ToStr(schema_in));
    const auto snapshot_view = ConvertSnapshot(dummy_sink->latest_snapshot);

    std::map<std::string, double> parsed_values;
    auto callback = [&](const std::string &field_name, const DataTamerParser::VarNumber &number)
    {
        const double value = std::visit(
            [](const auto &var)
            {
                return double(var);
            },
            number);
        parsed_values[field_name] = value;
    };

    DataTamerParser::ParseSnapshot(schema_out, snapshot_view, callback);

    for (const auto &[name, value] : parsed_values)
    {
        std::cout << name << ": " << value << std::endl;
    }

    ASSERT_EQ(parsed_values.at("valsA[0]"), 10);
    ASSERT_EQ(parsed_values.at("valsA[1]"), 11);
    ASSERT_EQ(parsed_values.at("valsA[2]"), 12);

    ASSERT_EQ(parsed_values.at("valsB[0]"), 13);
    ASSERT_EQ(parsed_values.at("valsB[1]"), 14);

    ASSERT_EQ(parsed_values.at("points[0]/x"), 1);
    ASSERT_EQ(parsed_values.at("points[0]/y"), 2);
    ASSERT_EQ(parsed_values.at("points[0]/z"), 3);

    ASSERT_EQ(parsed_values.at("points[1]/x"), 4);
    ASSERT_EQ(parsed_values.at("points[1]/y"), 5);
    ASSERT_EQ(parsed_values.at("points[1]/z"), 6);

    ASSERT_EQ(parsed_values.at("points[2]/x"), 7);
    ASSERT_EQ(parsed_values.at("points[2]/y"), 8);
    ASSERT_EQ(parsed_values.at("points[2]/z"), 9);

    ASSERT_EQ(parsed_values.at("rots[0]/r00"), 1);
    ASSERT_EQ(parsed_values.at("rots[0]/r01"), 0);
    ASSERT_EQ(parsed_values.at("rots[0]/r02"), 0);


    ASSERT_EQ(parsed_values.at("rots[1]/r00"), 0);
    ASSERT_EQ(parsed_values.at("rots[1]/r01"), 0);
    ASSERT_EQ(parsed_values.at("rots[1]/r02"), 0);

    CLOG_INFO << "Success";
}