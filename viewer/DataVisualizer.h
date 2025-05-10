//
// Created by liuqiang on 25-5-9.
//

#ifndef MUJOCO_DATAVISUALIZER_H
#define MUJOCO_DATAVISUALIZER_H

#include <data_tamer/data_tamer.hpp>
#include <implot.h>
#include <imgui.h>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <string>
#include "Logging.h"

// 用于存储单个变量的实时数据
struct VariableBuffer
{
    std::vector<double> timestamps;
    std::vector<double> values;
    mutable std::mutex mutex; // 保护数据访问
    size_t max_size = 10000;  // 默认最大数据点数量

    // 添加新数据点，保持队列大小限制
    void addPoint(double timestamp, double value)
    {
        std::lock_guard<std::mutex> lock(mutex);
        timestamps.push_back(timestamp);
        values.push_back(value);

        // 保持固定大小，移除旧数据
        if (timestamps.size() > max_size)
        {
            size_t excess = timestamps.size() - max_size;
            timestamps.erase(timestamps.begin(), timestamps.begin() + excess);
            values.erase(values.begin(), values.begin() + excess);
        }
    }

    // 获取数据副本用于渲染（线程安全）
    void getDataCopy(std::vector<double> &out_times, std::vector<double> &out_values) const
    {
        std::lock_guard<std::mutex> lock(mutex);
        out_times = timestamps;
        out_values = values;
    }
};

// 用于存储单个通道的所有变量
struct ChannelData
{
    std::unordered_map<std::string, std::shared_ptr<VariableBuffer>> variables;
    mutable std::mutex mutex;
};

// 自定义 DataSink 用于实时可视化
class VisualizationSink : public DataTamer::DataSinkBase
{
public:
    VisualizationSink() = default;
    ~VisualizationSink() override
    {
        stopThread();
    }

    // 实现基类接口：添加通道
    void addChannel(const std::string &name, const DataTamer::Schema &schema) override
    {
        std::lock_guard<std::mutex> lock(channels_mutex_);
        if (channels_.find(name) == channels_.end())
        {
            channels_[name] = std::make_shared<ChannelData>();
            for (const auto &field : schema.fields)
            {
                channels_[name]->variables[field.name] = std::make_shared<VariableBuffer>();
            }
        }
    }

    // 实现基类接口：存储快照
    bool storeSnapshot(const DataTamer::Snapshot &snapshot) override
    {
        std::lock_guard<std::mutex> lock(channels_mutex_);
        auto it = channels_.find(std::string(snapshot.channel_name));
        if (it == channels_.end())
            return false;

        const auto &channel_data = it->second;
        const double timestamp = snapshot.timestamp.count() * 1e-9; // 转换为秒

        // 解析快照数据
        SerializeMe::SpanBytes payload(snapshot.payload);
        size_t field_idx = 0;

        for (const auto &field : channels_[it->first]->variables)
        {
            // 检查字段是否活跃
            if (DataTamer::GetBit(snapshot.active_mask, field_idx))
            {
                double value = 0.0;

                // 根据字段类型解析值
                switch (field.second->type)
                {
                case DataTamer::BasicType::DOUBLE:
                    value = *reinterpret_cast<const double *>(payload.data());
                    payload.trimFront(sizeof(double));
                    break;
                case DataTamer::BasicType::FLOAT:
                    value = static_cast<double>(*reinterpret_cast<const float *>(payload.data()));
                    payload.trimFront(sizeof(float));
                    break;
                case DataTamer::BasicType::INT32:
                    value = static_cast<double>(*reinterpret_cast<const int32_t *>(payload.data()));
                    payload.trimFront(sizeof(int32_t));
                    break;
                // 可以添加更多类型...
                default:
                    // 对于不支持的类型，跳过
                    continue;
                }

                // 添加到变量缓冲区
                field.second->addPoint(timestamp, value);
            }
            field_idx++;
        }

        return true;
    }

    // 获取通道变量数据用于可视化
    bool getVariableData(const std::string &channel_name,
                         const std::string &variable_name,
                         std::vector<double> &out_times,
                         std::vector<double> &out_values) const
    {
        std::lock_guard<std::mutex> lock(channels_mutex_);
        auto channel_it = channels_.find(channel_name);
        if (channel_it == channels_.end())
            return false;

        auto var_it = channel_it->second->variables.find(variable_name);
        if (var_it == channel_it->second->variables.end())
            return false;

        var_it->second->getDataCopy(out_times, out_values);
        return true;
    }

    // 获取所有通道名称
    std::vector<std::string> getChannelNames() const
    {
        std::lock_guard<std::mutex> lock(channels_mutex_);
        std::vector<std::string> names;
        names.reserve(channels_.size());
        for (const auto &pair : channels_)
        {
            names.push_back(pair.first);
        }
        return names;
    }

    // 获取通道内的所有变量名称
    std::vector<std::string> getVariableNames(const std::string &channel_name) const
    {
        std::lock_guard<std::mutex> lock(channels_mutex_);
        std::vector<std::string> names;

        auto channel_it = channels_.find(channel_name);
        if (channel_it == channels_.end())
            return names;

        names.reserve(channel_it->second->variables.size());
        for (const auto &pair : channel_it->second->variables)
        {
            names.push_back(pair.first);
        }
        return names;
    }

private:
    mutable std::mutex channels_mutex_;
    std::unordered_map<std::string, std::shared_ptr<ChannelData>> channels_;
};

class DataVisualizer
{
public:
    DataVisualizer()
    {
        sink_ = std::make_shared<VisualizationSink>();
        // 将sink添加到全局注册表
        DataTamer::ChannelsRegistry::Global().addDefaultSink(sink_);

        startUpdateThread();
    }

    ~DataVisualizer()
    {
        running_ = false;
        if (update_thread_.joinable())
        {
            update_thread_.join();
        }
    }

    // 渲染ImGui窗口
    void render()
    {
        ImGui::Begin("Data Visualizer");

        // 选择通道
        std::vector<std::string> channel_names = sink_->getChannelNames();
        static int selected_channel_idx = 0;
        static std::string selected_channel;

        if (!channel_names.empty())
        {
            if (ImGui::BeginCombo("Channel", channel_names[selected_channel_idx].c_str()))
            {
                for (size_t i = 0; i < channel_names.size(); i++)
                {
                    bool is_selected = (selected_channel_idx == i);
                    if (ImGui::Selectable(channel_names[i].c_str(), is_selected))
                    {
                        selected_channel_idx = i;
                        selected_channel = channel_names[i];
                    }
                    if (is_selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            // 显示该通道的变量
            if (!selected_channel.empty())
            {
                std::vector<std::string> variable_names = sink_->getVariableNames(selected_channel);

                for (const auto &var_name : variable_names)
                {
                    renderPlot(selected_channel, var_name);
                }
            }
        }
        else
        {
            ImGui::Text("No channels available");
        }

        ImGui::End();
    }

private:
    std::shared_ptr<VisualizationSink> sink_;
    std::thread update_thread_;
    std::atomic<bool> running_{true};

    void startUpdateThread()
    {
        update_thread_ = std::thread(
            [this]()
            {
                while (running_)
                {
                    // 定期更新数据
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    // 数据更新由DataTamer自动处理
                }
            });
    }

    // 渲染单个变量的图表
    void renderPlot(const std::string &channel_name, const std::string &variable_name)
    {
        std::vector<double> times, values;
        if (!sink_->getVariableData(channel_name, variable_name, times, values))
            return;

        if (times.empty() || values.empty())
            return;

        ImGui::PushID(variable_name.c_str());
        if (ImGui::CollapsingHeader(variable_name.c_str()))
        {
            ImVec2 plot_size = ImVec2(-1, 200);
            if (ImPlot::BeginPlot(variable_name.c_str(), plot_size))
            {
                ImPlot::SetupAxes("Time (s)", variable_name.c_str());
                ImPlot::PlotLine(variable_name.c_str(), times.data(), values.data(), static_cast<int>(times.size()));
                ImPlot::EndPlot();
            }
        }
        ImGui::PopID();
    }
};

#endif // MUJOCO_DATAVISUALIZER_H
