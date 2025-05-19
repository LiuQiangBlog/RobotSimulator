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
#include "data_channel.hpp"
#include <thread>
#include <atomic>

namespace DataTamer
{

using Mutex = std::shared_mutex;
class Handler
{
public:
    Handler()
    {
        fields.cnt = 0;
    }
    ~Handler() = default;
    void channels_req(const zcm::ReceiveBuffer *buffer, const std::string &channel, const data_fields *msg)
    {
        std::lock_guard<std::shared_mutex> lock(mtx);
        zcm->publish("channels_rep", &fields);
    }

    std::shared_mutex mtx;
    data_fields fields;
    zcm::ZCM *zcm{nullptr};
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
//    Mutex schema_mutex;
    // std::unordered_map<std::string, std::pair<std::deque<double>, std::deque<double>>> channel_data;
    // std::unordered_map<std::string, std::pair<std::vector<double>, std::vector<double>>> channel_plot_data;
    uint64_t start_time;
    std::unique_ptr<zcm::ZCM> zcm, zcm_channels;
    timed_value data;
    std::unordered_set<std::string> pub_channels;
    std::unordered_map<std::string, std::deque<timed_value>> buffer_data;
//    data_channel new_channel;
    std::thread th_zcm_channels, th_zcm;
    Handler h;
    std::atomic_bool exit{false};
    std::condition_variable_any cv;

    PublishSink()
    {
        auto duration = std::chrono::system_clock::now().time_since_epoch();
        start_time = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    }

    ~PublishSink() override
    {
        stopThread();
        zcm->stop();
        zcm_channels->stop();
        exit = true;
        if (th_zcm_channels.joinable())
        {
            th_zcm_channels.join();
        }
//        if (th_zcm.joinable())
//        {
//            th_zcm.join();
//        }
    }

    bool init()
    {
        zcm = std::make_unique<zcm::ZCM>("ipcshm://");
        zcm_channels = std::make_unique<zcm::ZCM>("ipcshm://");
        if (!(zcm->good() && zcm_channels->good()))
        {
            CLOG_ERROR << "Failed to create zcm::ZCM()";
            zcm.reset();
            zcm_channels.reset();
            return false;
        }
        h.zcm = zcm_channels.get();
        zcm_channels->subscribe("channels_req", &Handler::channels_req, &h);
        th_zcm_channels = std::thread(
            [&]()
            {
                zcm_channels->run();
            });
//        th_zcm = std::thread(
//            [&]()
//            {
//                while (!exit)
//                {
//                    std::vector<std::string> channels_to_publish;
//                    {
//                        std::shared_lock<std::shared_mutex> lock(h.mtx);
//                        channels_to_publish = h.fields.channels;
//                    }
//                    for (auto &channel : channels_to_publish)
//                    {
//                        if (buffer_data.count(channel) > 0)
//                        {
//                            timed_value temp;
//                            {
//                                std::lock_guard<std::shared_mutex> lock(h.mtx);
//                                if (buffer_data[channel].empty())
//                                {
//                                    continue;
//                                }
//                                temp = buffer_data[channel].front();
//                                buffer_data[channel].pop_front();
//                            }
//                            zcm->publish(channel, &temp);
//                        }
//                        else
//                        {
//                            CLOG_ERROR << "No channel: " << channel;
//                        }
//                    }
//                    if (exit)
//                    {
//                        CLOG_INFO << "aaaaaaa";
//                        return;
//                    }
//                    CLOG_INFO << "ccccccc";
//                }
//            });
        return true;
    }

    void addChannel(std::string const &name, Schema const &schema) override
    {
//        std::scoped_lock lk(schema_mutex);
        std::scoped_lock lk(h.mtx);
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
//        std::scoped_lock lk(schema_mutex);
        std::unique_lock lk(h.mtx);
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

            auto start_tp = std::chrono::system_clock::time_point(std::chrono::nanoseconds(start_time));
            auto snapshot_tp = std::chrono::system_clock::time_point(std::chrono::nanoseconds(snapshot_view.timestamp));
            auto sec = std::chrono::duration<double>(snapshot_tp - start_tp).count();

            auto callback = [&](const std::string &field_name, const DataTamerParser::VarNumber &number)
            {
                const double value = std::visit(
                    [](const auto &var)
                    {
                        return double(var);
                    },
                    number);
                parsed_values[field_name] = {sec, value}; // timestamp unit is s
            };
            DataTamerParser::ParseSnapshot(schema_out, snapshot_view, callback);

            for (auto &[key, pair] : parsed_values)
            {
                if (pub_channels.count(key) <= 0)
                {
                    pub_channels.insert(key);
//                    new_channel.channel = key;
//                    new_channel.cnt = (int)parsed_values.size();
                    // std::scoped_lock lck(h.mtx);
                    h.fields.channels.push_back(key);
                    h.fields.cnt = (int)h.fields.channels.size();
                }
            }
            // zcm_channels->publish("new_channel", &new_channel);
            static std::unordered_set<std::string> pre_pub_channels;
            if (pre_pub_channels != pub_channels)
            {
                zcm_channels->publish("channels_rep", &h.fields);
                pre_pub_channels = pub_channels;
            }
            for (auto &[key, pair] : parsed_values)
            {
                data.set(pair.first, pair.second, (int)parsed_values.size());
                buffer_data[key].push_back(data);
                if (buffer_data[key].size() > MAX_CACHE_SIZE)
                {
                    buffer_data[key].pop_front();
                }
                zcm->publish(key, &data);
//                std::cerr << "  data: " << data.timestamp << ", " << data.value << std::endl;
//                CLOG_INFO << "bbbbbb";
            }
//            std::cerr << "------------------------------" << std::endl;
//            CLOG_INFO << "parsed_values size: " << parsed_values.size();
//            for (const auto &[field, value] : parsed_values) {
//                CLOG_INFO << "  Field: " << field << ", value: " << value.first << ", " << value.second;
//            }
        }

        return true;
    }
};

} // namespace DataTamer

#endif // MUJOCO_ZCMSINK_H
