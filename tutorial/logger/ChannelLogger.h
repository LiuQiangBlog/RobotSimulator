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
using Schema = DataTamer::Schema;

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

    Schema getSchema() const
    {
        return channel->getSchema();
    }

    const std::unordered_map<std::string, size_t> &getRegisteredValues() const
    {
        return channel->getRegisteredValues();
    }

    std::vector<std::string> getFlatFieldNames()
    {
        std::vector<std::string> result;
        auto schema = channel->getSchema();
        for (const auto& variable : schema.fields)
        {
            if (schema.custom_types.count(variable.type_name) > 0)
            {
                for (auto &element : schema.custom_types[variable.type_name])
                {
                    result.push_back(variable.field_name + "/" + element.field_name);
                }
            }
            else
            {
                result.push_back(variable.field_name);
            }
        }
        return result;
    }

    void ExtractFlatFieldPathsRecursive(const DataTamer::TypeField &field,
                                        const std::map<std::string, DataTamer::FieldsVector> &types_list,
                                        const std::string &prefix,
                                        std::vector<std::string> &out_paths)
    {
        uint32_t vect_size = field.array_size;
        std::string new_prefix = prefix.empty() ? field.field_name : (prefix + "/" + field.field_name);

        auto doExtract = [&](const std::string &var_name)
        {
            if (field.type != DataTamer::BasicType::OTHER)
            {
                // 基础类型，直接输出路径
                out_paths.push_back(var_name);
            }
            else
            {
                // 自定义结构体类型，递归展开
                const DataTamer::FieldsVector &fields = types_list.at(field.type_name);
                for (const auto &sub_field : fields)
                {
                    ExtractFlatFieldPathsRecursive(sub_field, types_list, var_name, out_paths);
                }
            }
        };

        if (!field.is_vector)
        {
            doExtract(new_prefix);
        }
        else
        {
            for (uint32_t i = 0; i < vect_size; ++i)
            {
                const auto indexed_name = new_prefix + "[" + std::to_string(i) + "]";
                doExtract(indexed_name);
            }
        }
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
