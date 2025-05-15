//
// Created by liuqiang on 25-5-15.
//

#ifndef MUJOCO_CHANNELLOGGER_H
#define MUJOCO_CHANNELLOGGER_H

#include <memory>
#include <vector>
#include <array>
#include <typeindex>
#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include <data_tamer/channel.hpp>
#include <data_tamer/sinks/mcap_sink.hpp>
#include <data_tamer/sinks/plot_sink.hpp>
#include <data_tamer/sinks/publish_sink.hpp>

enum class LoggerType
{
    Mcap = 0,
    Plot,
    Publish,
    McapPlot,
    McapPublish,
    PlotPublish,
    McapPlotPublish
};

class McapLogger
{
public:
    explicit McapLogger(std::string channelName, const std::string &fileName)
        : channel(DataTamer::LogChannel::create(std::move(channelName)))
    {
        mcapSink = std::make_shared<DataTamer::MCAPSink>(fileName);
    }

    bool init()
    {
        if (publishSink && publishSink->init())
        {
            initialized = true;
            return true;
        }
        return false;
    }

    template <typename T>
    void registerValue(const std::string& name, const T* value)
    {
        channel->registerValue(name, value);
    }

    template <typename T>
    void registerCustomValue(const std::string& name, const T* value, DataTamer::CustomSerializer::Ptr serializer)
    {
        channel->registerCustomValue(name, value, serializer);
    }

private:
    std::shared_ptr<DataTamer::LogChannel> channel;

    std::shared_ptr<DataTamer::MCAPSink> mcapSink{nullptr};
    std::shared_ptr<DataTamer::PlotSink> plotSink{nullptr};
    std::shared_ptr<DataTamer::PublishSink> publishSink{nullptr};

    std::unordered_map<std::string, data_fields> mapFields;
    bool initialized{true};
};


#endif // MUJOCO_CHANNELLOGGER_H
