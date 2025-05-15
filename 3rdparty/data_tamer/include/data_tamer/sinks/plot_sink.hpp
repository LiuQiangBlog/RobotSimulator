//
// Created by liuqiang on 25-5-10.
//

#ifndef MUJOCO_PLOTSINK_H
#define MUJOCO_PLOTSINK_H

#include "data_tamer/data_sink.hpp"
#include <data_tamer_parser/data_tamer_parser.hpp>
#include <mutex>
#include <unordered_map>
#include <shared_mutex>
#include <Eigen/Dense>
#include <deque>

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

namespace DataTamer
{

using Mutex = std::shared_mutex;

/**
 * @brief The DummySink does nothing, only counting the number of snapshots received.
 * Used mostly for testing and debugging.
 */
class PlotSink : public DataSinkBase
{
public:
    std::unordered_map<uint64_t, Schema> schemas;
    std::unordered_map<uint64_t, std::string> schema_names;
    std::unordered_map<uint64_t, long> snapshots_count;
    Snapshot latest_snapshot;
    Mutex schema_mutex;
    std::unordered_map<std::string, std::pair<std::deque<double>, std::deque<double>>> channel_data;
    std::unordered_map<std::string, std::pair<std::vector<double>, std::vector<double>>> channel_plot_data;
    uint64_t start_time;

    PlotSink()
    {
        auto duration = std::chrono::system_clock::now().time_since_epoch();
        start_time = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    }

    ~PlotSink() override
    {
        stopThread();
    }

    void addChannel(std::string const &name, Schema const &schema) override
    {
        std::scoped_lock lk(schema_mutex);
        schemas[schema.hash] = schema;
        schema_names[schema.hash] = name;
        snapshots_count[schema.hash] = 0;
    }

    static DataTamerParser::SnapshotView ConvertSnapshot(const DataTamer::Snapshot &snapshot)
    {
        return {snapshot.schema_hash,
                uint64_t(snapshot.timestamp.count()),
                {snapshot.active_mask.data(), snapshot.active_mask.size()},
                {snapshot.payload.data(), snapshot.payload.size()}};
    }

    bool storeSnapshot(const Snapshot &snapshot) override
    {
        std::scoped_lock lk(schema_mutex);
        latest_snapshot = snapshot;

        auto it = snapshots_count.find(snapshot.schema_hash);
        if (it != snapshots_count.end())
        {
            it->second++;
        }

        if (schemas.count(latest_snapshot.schema_hash) > 0)
        {
            const auto &schema_in = schemas[latest_snapshot.schema_hash];
            const auto &schema_out = DataTamerParser::BuilSchemaFromText(ToStr(schema_in));
            const auto snapshot_view = ConvertSnapshot(latest_snapshot);
            std::map<std::string, std::pair<double, double>> parsed_values; // timestamp, value
            auto callback = [&](const std::string &field_name, const DataTamerParser::VarNumber &number)
            {
                const double value = std::visit(
                    [](const auto &var)
                    {
                        return double(var);
                    },
                    number);
                auto start_tp = std::chrono::system_clock::time_point(std::chrono::nanoseconds(start_time));
                auto snapshot_tp = std::chrono::system_clock::time_point(std::chrono::nanoseconds(snapshot_view.timestamp));
                auto sec = std::chrono::duration<double>(snapshot_tp - start_tp).count();
                parsed_values[field_name] = {sec, value}; // timestamp unit is s
            };
            DataTamerParser::ParseSnapshot(schema_out, snapshot_view, callback);
            for (auto &[key, pair] : parsed_values)
            {
                channel_data[key].first.push_back(pair.first);
                channel_data[key].second.push_back(pair.second);
                if (channel_data[key].first.size() > 10000)
                {
                    channel_data[key].first.pop_front();
                    channel_data[key].second.pop_front();
                }
                auto timestamps = std::vector<double>(channel_data[key].first.begin(), channel_data[key].first.end());
                auto values = std::vector<double>(channel_data[key].second.begin(), channel_data[key].second.end());
                channel_plot_data[key].first = timestamps;
                channel_plot_data[key].second = values;
            }
        }
        return true;
    }
};

} // namespace DataTamer

#endif // MUJOCO_PLOTSINK_H
