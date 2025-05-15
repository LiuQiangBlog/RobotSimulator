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
#include <unordered_set>
#include <shared_mutex>
#include <Eigen/Dense>
#include <deque>
#include <zcm/zcm-cpp.hpp>
#include "timed_value.hpp"
#include <Logging.h>
#include "data_fields.hpp"
#include <thread>
#include <atomic>

namespace DataTamer
{

using Mutex = std::shared_mutex;
class Handler
{
public:
    ~Handler() = default;
    void handle(const zcm::ReceiveBuffer *buffer, const std::string &channel, const data_fields *msg)
    {
        std::lock_guard<std::shared_mutex> lock(mtx);
        fields = msg->channels;
    }
    std::shared_mutex mtx;
    std::vector<std::string> fields;
};
/**
 * @brief The PublishSink publish message, only counting the number of snapshots received.
 * Used mostly for testing and debugging.
 */
class PublishSink : public DataSinkBase
{
public:
    const size_t MAX_CACHE_SIZE = 10000;
    std::unordered_map<uint64_t, Schema> schemas;
    std::unordered_map<uint64_t, std::string> schema_names;
    std::unordered_map<uint64_t, long> snapshots_count;
    Snapshot latest_snapshot;
    Mutex schema_mutex;
    // std::unordered_map<std::string, std::pair<std::deque<double>, std::deque<double>>> channel_data;
    // std::unordered_map<std::string, std::pair<std::vector<double>, std::vector<double>>> channel_plot_data;
    uint64_t start_time;
    std::unique_ptr<zcm::ZCM> publisher, subscriber;
    timed_value data;
//    std::unordered_set<std::string> channels;
    std::unordered_map<std::string, std::deque<timed_value>> buffer_data;
//    data_fields fields;
    std::thread threadForSub, threadForPub;
    Handler h;
    std::atomic_bool exit{false};
    std::vector<std::string> channels;

    PublishSink()
    {
        auto duration = std::chrono::system_clock::now().time_since_epoch();
        start_time = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    }

    ~PublishSink() override
    {
        stopThread();
        exit = true;
        if (threadForSub.joinable())
        {
            threadForSub.join();
        }
        if (threadForPub.joinable())
        {
            threadForPub.join();
        }
    }

    bool init()
    {
        publisher = std::make_unique<zcm::ZCM>("ipcshm://");
        subscriber = std::make_unique<zcm::ZCM>("ipcshm://");
        if (!(publisher->good() && subscriber->good()))
        {
            CLOG_ERROR << "Failed to create zcm::ZCM()";
            publisher.reset();
            return false;
        }
        subscriber->subscribe("SubChannels", &Handler::handle, &h);
        threadForSub = std::thread(
            [&]()
            {
                subscriber->run();
            });
        threadForPub = std::thread(
            [&]()
            {
                while (!exit)
                {
                    {
                        std::lock_guard<std::shared_mutex> lock(h.mtx);
                        channels = h.fields;
                    }
                    while (channels.empty())
                    {
                        for (auto &channel : channels)
                        {
                            if (buffer_data.count(channel) > 0)
                            {
                                publisher->publish(channel, &buffer_data[channel].front());
                                buffer_data[channel].pop_front();
                            }
                            else
                            {
                                CLOG_ERROR << "No channel: " << channel;
                            }
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            });
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
                auto snapshot_tp =
                    std::chrono::system_clock::time_point(std::chrono::nanoseconds(snapshot_view.timestamp));
                auto sec = std::chrono::duration<double>(snapshot_tp - start_tp).count();
                parsed_values[field_name] = {sec, value}; // timestamp unit is s
            };
            DataTamerParser::ParseSnapshot(schema_out, snapshot_view, callback);
            for (auto &[key, pair] : parsed_values)
            {
                data.set(pair.first, pair.second);
                buffer_data[key].push_back(data);
                if (buffer_data[key].size() > MAX_CACHE_SIZE)
                {
                    buffer_data[key].pop_front();
                }
//                if (publisher)
//                {
//                    data.set(pair.first, pair.second);
//                    if (channels.count(key) <= 0)
//                    {
//                        fields.channels.push_back(key);
//                        fields.cnt = (int)fields.channels.size();
//                        publisher->publish("PubChannels", &fields); // publish all channels
//
//                        channels.insert(key);
//                        buffer_data.insert({key, data});
//                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
//                    }
//                    else
//                    {
//                        for (auto &[bufKey, bufVal] : buffer_data)
//                        {
//                            publisher->publish(bufKey, &bufVal);
//                        }
//                        buffer_data.clear();
//                        publisher->publish(key, &data);
//                    }
//                }
                //                channel_data[key].first.push_back(pair.first);
                //                channel_data[key].second.push_back(pair.second);
                //                if (channel_data[key].first.size() > 10000)
                //                {
                //                    channel_data[key].first.pop_front();
                //                    channel_data[key].second.pop_front();
                //                }
                //                auto timestamps = std::vector<double>(channel_data[key].first.begin(),
                //                channel_data[key].first.end()); auto values =
                //                std::vector<double>(channel_data[key].second.begin(), channel_data[key].second.end());
                //                channel_plot_data[key].first = timestamps;
                //                channel_plot_data[key].second = values;
            }
        }
        return true;
    }
};

} // namespace DataTamer

#endif // MUJOCO_ZCMSINK_H
