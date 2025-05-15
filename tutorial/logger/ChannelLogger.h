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

using LogChannelPtr = std::shared_ptr<DataTamer::LogChannel>;
using MCAPSinkPtr = std::shared_ptr<DataTamer::MCAPSink>;
using PlotSinkPtr = std::shared_ptr<DataTamer::PlotSink>;
using PublishSinkPtr = std::shared_ptr<DataTamer::PublishSink>;
using RegistrationID = DataTamer::RegistrationID;

class ChannelLogger
{
public:
    explicit ChannelLogger(std::string channelName)
    {
        channel = DataTamer::LogChannel::create(std::move(channelName));
    }

    void addMcapSink(const std::string &fileName, bool do_compression = false)
    {
        mcapSink = std::make_shared<DataTamer::MCAPSink>(fileName, do_compression);
    }

    void addPlotSink()
    {
        plotSink = std::make_shared<DataTamer::PlotSink>();
    }

    void addPublishSink()
    {
        publishSink = std::make_shared<DataTamer::PublishSink>();
    }

    template <typename T>
    RegistrationID registerValue(const std::string& name, const T* value)
    {
        return channel->registerValue(name, value);
    }

    template <template <class, class> class Container, class T, class... TArgs>
    RegistrationID registerValue(const std::string &name, const Container<T, TArgs...> *value)
    {
        return channel->registerValue(name, value);
    }

    template <typename T, size_t N>
    RegistrationID registerValue(const std::string &name, const std::array<T, N> *value)
    {
        return channel->registerValue(name, value);
    }

    template <typename T>
    RegistrationID registerCustomValue(const std::string& name, const T* value, DataTamer::CustomSerializer::Ptr serializer)
    {
        return channel->registerCustomValue(name, value, serializer);
    }

    bool takeSnapshot()
    {
        channel->takeSnapshot();
    }

    const std::string &channelName() const
    {
        return channel->channelName();
    }

    void setEnabled(const RegistrationID &id, bool enable)
    {
        channel->setEnabled(id, enable);
    }

    void unregister(const RegistrationID &id)
    {
        channel->unregister(id);
    }


private:
    LogChannelPtr channel;

    MCAPSinkPtr mcapSink{nullptr};
    PlotSinkPtr plotSink{nullptr};
    PublishSinkPtr publishSink{nullptr};

    std::unordered_map<std::string, data_fields> mapFields;
    bool initialized{true};
};

#endif // MUJOCO_CHANNELLOGGER_H
