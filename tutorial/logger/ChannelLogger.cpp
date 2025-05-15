//
// Created by liuqiang on 25-5-15.
//

#include "ChannelLogger.h"
#include <data_tamer/data_tamer.hpp>
#include <data_tamer/sinks/mcap_sink.hpp>
#include <data_tamer/sinks/publish_sink.hpp>

#include <memory>
#include <vector>
#include <array>
#include <typeindex>
#include <unordered_map>

struct IRegistrar
{
    virtual void registerTo(DataTamer::LogChannel &channel) const = 0;
    virtual ~IRegistrar() = default;
};

template <typename T>
struct ValueRegistrar : IRegistrar
{
    std::string name;
    const T *ptr;
    void registerTo(DataTamer::LogChannel &ch) const override
    {
        ch.registerValue<T>(name, ptr);
    }
};

template <typename T>
struct CustomRegistrar : IRegistrar
{
    std::string name;
    const T *ptr;
    std::shared_ptr<DataTamer::CustomSerializer> serializer;
    void registerTo(DataTamer::LogChannel &ch) const override
    {
        ch.registerCustomValue(name, ptr, serializer);
    }
};

struct ChannelLogger::Impl
{
    std::shared_ptr<DataTamer::PublishSink> pubSink;
    std::shared_ptr<DataTamer::LogChannel> channel;
    std::vector<std::function<void()>> registrations;
    std::unordered_map<std::string, data_fields> mapFields;
    LoggerType type;

    explicit Impl(std::string name, LoggerType tp) : type(tp), channel(DataTamer::LogChannel::create(std::move(name)))
    {
        if (type == LoggerType::Mcap)
        {

        }
        pubSink = std::make_shared<DataTamer::PublishSink>();

    }

    void applyRegistrations()
    {
        for (auto &r : registrations)
        {
            r(); // 调用注册行为
        }
        registrations.clear();
    }

    template <typename T>
    void addValueRegistration(const std::string &name, const T *ptr)
    {
        registrations.emplace_back(
            [=, ch = channel]()
            {
                ch->registerValue<T>(name, ptr);
            });
    }

    template <typename T>
    void addCustomRegistration(const std::string &name, const T *ptr, std::shared_ptr<void> serializer)
    {
        auto casted = std::static_pointer_cast<DataTamer::CustomSerializer>(serializer);
        registrations.emplace_back(
            [=, ch = channel]()
            {
                ch->registerCustomValue(name, ptr, casted);
            });
    }

    void publishFields(const std::string &prefix)
    {
        auto schema = channel->getSchema();
        data_fields fields;
        for (auto &field : schema.fields)
        {
            fields.channels.push_back(prefix + "/" + field.field_name);
        }
        fields.cnt = (int)fields.channels.size();
        mapFields.insert({prefix, fields});
        if (pubSink)
        {
            pubSink->publisher->publish(prefix, &mapFields[prefix]);
            CLOG_INFO << "publish: " << prefix;
        }
    }

};

ChannelLogger::ChannelLogger(std::string name, LoggerType tp)
{
    _impl = std::make_unique<ChannelLogger::Impl>(std::move(name), tp);
}

bool ChannelLogger::init()
{
    static bool initialized{false};
    if (!initialized)
    {
        if (_impl->pubSink->init())
        {
            initialized = true;
            return true;
        }
        return false;
    }
}

void ChannelLogger::registerValue(std::function<void(void* log_channel)> registrar)
{
    _impl->registerValue(registrar);
}
