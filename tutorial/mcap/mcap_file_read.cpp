//
// Created by liuqiang on 25-5-24.
//
#include <mcap/mcap.hpp>
#include <Logging.h>

// int main()
//{
//     std::string filepath("/home/liuqiang/ClionProjects/RobotSimulator/cmake-build-debug/bin/test_sample.mcap");
//     mcap::McapReader reader;
//     // step1: open
//     auto status = reader.open(filepath);
//     if (!status.ok())
//     {
//         CLOG_ERROR << "Can't open file";
//         return -1;
//     }
//     // step2: readSummary
//     status = reader.readSummary(mcap::ReadSummaryMethod::NoFallbackScan);
//     if (!status.ok())
//     {
//         CLOG_ERROR << "Can't open summary of the file";
//         return -1;
//     }
//     // step3: schemas
//     std::unordered_map<int, mcap::SchemaPtr> mcap_schemas; // schema_id
//     for (const auto &[schema_id, schema_ptr] : reader.schemas())
//     {
//         mcap_schemas.insert({schema_id, schema_ptr});
//     }
//     // step4: channels
//     std::unordered_map<int, mcap::ChannelPtr> channels;    // channel_id
//     std::unordered_set<mcap::ChannelId> channels_containing_datatamer_schema;
//     std::unordered_set<mcap::ChannelId> channels_containing_datatamer_data;
//     auto statistics = reader.statistics();
//     uint64_t total_dt_schemas = 0;
//     for (const auto &[channel_id, channel_ptr] : reader.channels())
//     {
//         channels.insert({channel_id, channel_ptr});
//         const auto &schema = mcap_schemas.at(channel_ptr->schemaId);
//         const auto &topic_name = channel_ptr->topic;
//         std::string definition(reinterpret_cast<const char *>(schema->data.data()), schema->data.size());
//         if (schema->name == "data_tamer_msgs/msg/Schemas")
//         {
//             channels_containing_datatamer_schema.insert(channel_id);
//             total_dt_schemas += statistics->channelMessageCounts.at(channel_id);
//         }
//         if (schema->name == "data_tamer_msgs/msg/Snapshot")
//         {
//             channels_containing_datatamer_data.insert(channel_id);
//         }
//         std::string channel_encoding = channel_ptr->messageEncoding;
//         std::string schema_encoding = schema->encoding;
//     }
//     return 0;
// }

#include <mcap/mcap.hpp>
#include <data_tamer/data_tamer.hpp>
#include <data_tamer/schema.hpp>
#include <data_tamer/snapshot.hpp>
#include <Logging.h>

int main()
{
    std::string filepath("test_sample.mcap");
    mcap::McapReader reader;
    auto status = reader.open(filepath);
    if (!status.ok())
    {
        CLOG_ERROR << "Can't open file";
        return -1;
    }

    status = reader.readSummary(mcap::ReadSummaryMethod::NoFallbackScan);
    if (!status.ok())
    {
        CLOG_ERROR << "Can't read summary of the file";
        return -1;
    }

    // 收集所有Schema和Channel
    std::unordered_map<int, mcap::SchemaPtr> mcap_schemas;
    std::unordered_map<int, mcap::ChannelPtr> channels;
    std::unordered_set<mcap::ChannelId> dt_schema_channels;
    std::unordered_set<mcap::ChannelId> dt_data_channels;

    for (const auto &[schema_id, schema_ptr] : reader.schemas())
    {
        mcap_schemas[schema_id] = schema_ptr;
    }

    for (const auto &[channel_id, channel_ptr] : reader.channels())
    {
        channels[channel_id] = channel_ptr;
        const auto &schema = mcap_schemas.at(channel_ptr->schemaId);
        if (schema->name == "data_tamer_msgs/msg/Schemas")
        {
            dt_schema_channels.insert(channel_id);
        }
        if (schema->name == "data_tamer_msgs/msg/Snapshot")
        {
            dt_data_channels.insert(channel_id);
        }
    }

    // 解析data_tamer的Schema消息
    std::unordered_map<std::string, DataTamer::Schema> known_schemas;
    auto messageStream = reader.readMessages();
    mcap::MessageRecord msg;

    while (messageStream.read(msg))
    {
        if (dt_schema_channels.count(msg.channel->id) > 0)
        {
            // 解析data_tamer的Schema消息
            DataTamer::Schemas dt_schemas;
            SerializeMe::SpanBytesConst buffer(reinterpret_cast<const uint8_t *>(msg.message.data),
                                               msg.message.dataSize);
            SerializeMe::DeserializeFromBuffer(buffer, dt_schemas);

            // 存储所有解析出的Schema
            for (const auto &schema : dt_schemas.schemas)
            {
                known_schemas[schema.channel_name] = schema;
                CLOG_INFO << "Loaded schema for channel: " << schema.channel_name;
            }
        }
        else if (dt_data_channels.count(msg.channel->id) > 0)
        {
            // 解析data_tamer的Snapshot消息
            const std::string &channel_name = msg.channel->topic;
            auto schema_it = known_schemas.find(channel_name);

            if (schema_it != known_schemas.end())
            {
                DataTamer::Snapshot snapshot;
                SerializeMe::SpanBytesConst buffer(reinterpret_cast<const uint8_t *>(msg.message.data),
                                                   msg.message.dataSize);
                SerializeMe::DeserializeFromBuffer(buffer, snapshot);

                // 使用DataTamer的Parser解析实际数据
                DataTamer::Parser data_parser;
                data_parser.addSchema(schema_it->second);
                data_parser.addSnapshot(snapshot);

                // 访问解析后的数据
                auto data = data_parser.getData(channel_name);
                CLOG_INFO << "Received data for channel: " << channel_name;

                // 处理具体字段...
            }
        }
    }

    return 0;
}