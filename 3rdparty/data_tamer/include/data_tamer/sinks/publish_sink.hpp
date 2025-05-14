//
// Created by liuqiang on 25-5-10.
//

#ifndef MUJOCO_ZCMSINK_H
#define MUJOCO_ZCMSINK_H

#include "data_tamer/data_sink.hpp"
#include <data_tamer_parser/data_tamer_parser.hpp>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <shared_mutex>
#include <Eigen/Dense>
#include <deque>
#include <zcm/zcm-cpp.hpp>
#include "timed_value.hpp"
#include <Logging.h>
#include "data_fields.hpp"

namespace DataTamer
{

using Mutex = std::shared_mutex;

/**
 * @brief The PublishSink publish message, only counting the number of snapshots received.
 * Used mostly for testing and debugging.
 */
class PublishSink : public DataSinkBase
{
public:
    std::unordered_map<uint64_t, Schema> schemas;
    std::unordered_map<uint64_t, std::string> schema_names;
    std::unordered_map<uint64_t, long> snapshots_count;
    Snapshot latest_snapshot;
    Mutex schema_mutex;
//    std::unordered_map<std::string, std::pair<std::deque<double>, std::deque<double>>> channel_data;
//    std::unordered_map<std::string, std::pair<std::vector<double>, std::vector<double>>> channel_plot_data;
    uint64_t start_time;
    std::unique_ptr<zcm::ZCM> publisher;
    timed_value data;

    PublishSink()
    {
        auto duration = std::chrono::system_clock::now().time_since_epoch();
        start_time = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    }

    ~PublishSink() override
    {
        stopThread();
    }

    bool init()
    {
        publisher = std::make_unique<zcm::ZCM>("ipcshm://");
        if (!publisher->good())
        {
            CLOG_ERROR << "Failed to create zcm::ZCM()";
            publisher.reset();
            return false;
        }
        return true;
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
                if (publisher)
                {
                    data.set(pair.first, pair.second);
                    publisher->publish(key, &data);
                }
//                channel_data[key].first.push_back(pair.first);
//                channel_data[key].second.push_back(pair.second);
//                if (channel_data[key].first.size() > 10000)
//                {
//                    channel_data[key].first.pop_front();
//                    channel_data[key].second.pop_front();
//                }
//                auto timestamps = std::vector<double>(channel_data[key].first.begin(), channel_data[key].first.end());
//                auto values = std::vector<double>(channel_data[key].second.begin(), channel_data[key].second.end());
//                channel_plot_data[key].first = timestamps;
//                channel_plot_data[key].second = values;

            }
        }
        return true;
    }
};

} // namespace DataTamer

#endif // MUJOCO_ZCMSINK_H
