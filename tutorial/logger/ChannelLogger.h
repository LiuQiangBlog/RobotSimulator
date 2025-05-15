//
// Created by liuqiang on 25-5-15.
//

#ifndef MUJOCO_CHANNELLOGGER_H
#define MUJOCO_CHANNELLOGGER_H

#include <functional>
#include <memory>
#include <string>
#include <data_tamer/channel.hpp>

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

class ChannelLogger
{
public:
    explicit ChannelLogger(std::string name, LoggerType tp = LoggerType::Mcap);

    bool init();

    template <typename T>
    void registerValue(const std::string& name, const T* ptr);

    template <typename T>
    void registerCustomValue(const std::string& name, const T* ptr, std::shared_ptr<void> serializer);

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

template <typename T>
void ChannelLogger::registerValue(const std::string& name, const T* ptr) {
    _impl->addValueRegistration<T>(name, ptr);
}

template <typename T>
void ChannelLogger::registerCustomValue(const std::string& name, const T* ptr, std::shared_ptr<void> serializer) {
    _impl->addCustomRegistration<T>(name, ptr, std::move(serializer));
}

#endif // MUJOCO_CHANNELLOGGER_H
