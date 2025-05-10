//
// Created by liuqiang on 25-5-10.
//

#ifndef MUJOCO_IMPLOTSINK_H
#define MUJOCO_IMPLOTSINK_H

#include "data_tamer/data_tamer.hpp"
#include "data_tamer_parser/data_tamer_parser.hpp"
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iostream>

namespace DataTamer
{
class ImPlotSink : public DataSinkBase
{
public:
    using TimeSeries = std::vector<double>;
    using ChannelData = std::unordered_map<std::string, TimeSeries>;

    struct PlotData
    {
        std::vector<double> timestamps;
        ChannelData channels;
        std::mutex mutex;
    };

    explicit ImPlotSink(size_t max_points_per_series = 10000) : max_points_per_series_(max_points_per_series) {}

    ~ImPlotSink() override = default;

    void addChannel(const std::string &name, const DataTamer::Schema &schema) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // 转换为 DataTamerParser::Schema
        std::string schema_text = DataTamer::ToStr(schema);
        DataTamerParser::Schema parser_schema = DataTamerParser::BuilSchemaFromText(schema_text);
        schemas_[schema.hash] = parser_schema;
        channel_names_.emplace(name);
    }

    bool storeSnapshot(const DataTamer::Snapshot &snapshot) override
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // 查找对应的 schema
        auto schema_it = schemas_.find(snapshot.schema_hash);
        if (schema_it == schemas_.end())
        {
            return false;
        }

        // 将 DataTamer::Snapshot 转换为 DataTamerParser::SnapshotView
        DataTamerParser::SnapshotView view = {snapshot.schema_hash,
                                              static_cast<uint64_t>(snapshot.timestamp.count()),
                                              {snapshot.active_mask.data(), snapshot.active_mask.size()},
                                              {snapshot.payload.data(), snapshot.payload.size()}};

        std::string channel_name_str(snapshot.channel_name);
        // 确保 channel_name 有效
        if (!isValidChannelName(channel_name_str))
        {
            return false;
        }

        // 获取或创建该通道的 PlotData
        PlotData &plot_data = channel_data_[channel_name_str];
        std::lock_guard<std::mutex> plot_lock(plot_data.mutex);

        // 添加时间戳
        double timestamp = static_cast<double>(view.timestamp) * 1e-9;
        plot_data.timestamps.push_back(timestamp);

        // 定义基本类型回调函数
        auto number_callback = [&](const std::string &field_name, const DataTamerParser::VarNumber &value)
        {
            // 确保该字段有对应的 TimeSeries
            if (plot_data.channels.find(field_name) == plot_data.channels.end())
            {
                plot_data.channels[field_name] = TimeSeries();
            }

            // 将值添加到对应的 TimeSeries
            double numeric_value = std::visit(
                [](auto &&arg)
                {
                    return static_cast<double>(arg);
                },
                value);

            plot_data.channels[field_name].push_back(numeric_value);

            // 保持固定大小，移除旧数据
            if (plot_data.channels[field_name].size() > max_points_per_series_)
            {
                size_t excess = plot_data.channels[field_name].size() - max_points_per_series_;
                plot_data.channels[field_name].erase(plot_data.channels[field_name].begin(),
                                                     plot_data.channels[field_name].begin() + excess);
                plot_data.timestamps.erase(plot_data.timestamps.begin(), plot_data.timestamps.begin() + excess);
            }
        };

        // 定义自定义类型回调函数
        auto custom_callback = [](const std::string &field_name, const DataTamerParser::BufferSpan &buffer)
        {
            // 可以处理自定义类型的特殊逻辑，这里留空
            // 或者可以递归解析自定义类型
        };

        // 使用双回调版本的 ParseSnapshot
        DataTamerParser::ParseSnapshot(schema_it->second, view, number_callback, custom_callback);
        return true;
    }

    // 获取特定通道的数据用于 ImPlot
    bool getData(const std::string &channel_name,
                 std::vector<std::string> &series_names,
                 std::vector<const double *> &series_data,
                 const double *&timestamps,
                 size_t &count)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // 查找通道
        auto it = channel_data_.find(channel_name);
        if (it == channel_data_.end())
        {
            return false;
        }

        PlotData &plot_data = it->second;
        std::lock_guard<std::mutex> plot_lock(plot_data.mutex);

        // 清空输出向量
        series_names.clear();
        series_data.clear();

        // 填充系列数据
        for (auto &pair : plot_data.channels)
        {
            series_names.push_back(pair.first);
            series_data.push_back(pair.second.data());
        }

        // 设置时间戳和数据点数量
        timestamps = plot_data.timestamps.data();
        count = plot_data.timestamps.size();

        return true;
    }

    // 获取所有可用通道的名称
    std::vector<std::string> getChannelNames() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> names;
        names.reserve(channel_data_.size());
        for (const auto &pair : channel_data_)
        {
            names.push_back(pair.first);
        }
        return names;
    }

private:
    // 检查 channel_name 是否有效（即是否存在于 channel_names_ 中）
    bool isValidChannelName(const std::string &name) const
    {
        return channel_names_.find(name) != channel_names_.end();
    }

    size_t max_points_per_series_;
    std::unordered_map<uint64_t, DataTamerParser::Schema> schemas_;
    std::unordered_map<std::string, PlotData> channel_data_;
    std::unordered_set<std::string> channel_names_;
    mutable std::mutex mutex_;
};

} // namespace DataTamer

#endif // MUJOCO_IMPLOTSINK_H
