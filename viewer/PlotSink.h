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
    Mutex schema_mutex_;

    ~PlotSink() override
    {
        stopThread();
    }

    void addChannel(std::string const &name, Schema const &schema) override
    {
        std::scoped_lock lk(schema_mutex_);
        schemas[schema.hash] = schema;
        schema_names[schema.hash] = name;
        snapshots_count[schema.hash] = 0;
    }

    DataTamerParser::SnapshotView ConvertSnapshot(const DataTamer::Snapshot &snapshot)
    {
        return {snapshot.schema_hash,
                uint64_t(snapshot.timestamp.count()),
                {snapshot.active_mask.data(), snapshot.active_mask.size()},
                {snapshot.payload.data(), snapshot.payload.size()}};
    }

    bool storeSnapshot(const Snapshot &snapshot) override
    {
        std::scoped_lock lk(schema_mutex_);
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
                parsed_values[field_name] = {snapshot_view.timestamp, value};
            };
            DataTamerParser::ParseSnapshot(schema_out, snapshot_view, callback);
        }

        return true;
    }
};

} // namespace DataTamer

#endif // MUJOCO_PLOTSINK_H
