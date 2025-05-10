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

DataTamerParser::SnapshotView ConvertSnapshot(const DataTamer::Snapshot &snapshot)
{
    return {snapshot.schema_hash,
            uint64_t(snapshot.timestamp.count()),
            {snapshot.active_mask.data(), snapshot.active_mask.size()},
            {snapshot.payload.data(), snapshot.payload.size()}};
}

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

    std::array<Point3D, 3> points;
    points[0] = {1, 2, 3};
    points[1] = {4, 5, 6};
    points[2] = {7, 8, 9};
    std::vector<Quaternion> quats(2);
    quats[0] = {20, 21, 22, 23};
    quats[1] = {30, 31, 32, 33};

    channel->registerValue("valsA", &valsA);
    channel->registerValue("valsB", &valsB);
    channel->registerValue("points", &points);
    channel->registerValue("quats", &quats);

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

    ASSERT_EQ(parsed_values.at("quats[0]/w"), 20);
    ASSERT_EQ(parsed_values.at("quats[0]/x"), 21);
    ASSERT_EQ(parsed_values.at("quats[0]/y"), 22);
    ASSERT_EQ(parsed_values.at("quats[0]/z"), 23);

    ASSERT_EQ(parsed_values.at("quats[1]/w"), 30);
    ASSERT_EQ(parsed_values.at("quats[1]/x"), 31);
    ASSERT_EQ(parsed_values.at("quats[1]/y"), 32);
    ASSERT_EQ(parsed_values.at("quats[1]/z"), 33);
    CLOG_INFO << "Success";
}