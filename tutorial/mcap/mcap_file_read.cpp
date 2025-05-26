//
// Created by liuqiang on 25-5-24.
//
#include <mcap/mcap.hpp>
#include <Logging.h>

 int main()
{
     std::string filepath("/home/liuqiang/ClionProjects/RobotSimulator/cmake-build-debug/bin/test_sample.mcap");
     mcap::McapReader reader;
     // step1: open
     auto status = reader.open(filepath);
     if (!status.ok())
     {
         CLOG_ERROR << "Can't open file";
         return -1;
     }
     // step2: readSummary
     status = reader.readSummary(mcap::ReadSummaryMethod::NoFallbackScan);
     if (!status.ok())
     {
         CLOG_ERROR << "Can't open summary of the file";
         return -1;
     }
     // step3: schemas
     std::unordered_map<int, mcap::SchemaPtr> mcap_schemas; // schema_id
     for (const auto &[schema_id, schema_ptr] : reader.schemas())
     {
         mcap_schemas.insert({schema_id, schema_ptr});
     }
     // step4: channels
     std::unordered_map<int, mcap::ChannelPtr> channels;    // channel_id
     std::unordered_set<mcap::ChannelId> channels_containing_datatamer_schema;
     std::unordered_set<mcap::ChannelId> channels_containing_datatamer_data;
     auto statistics = reader.statistics();
     uint64_t total_dt_schemas = 0;
     for (const auto &[channel_id, channel_ptr] : reader.channels())
     {
         channels.insert({channel_id, channel_ptr});
         const auto &schema = mcap_schemas.at(channel_ptr->schemaId);
         const auto &topic_name = channel_ptr->topic;
         std::string definition(reinterpret_cast<const char *>(schema->data.data()), schema->data.size());
         if (schema->name == "data_tamer_msgs/msg/Schemas")
         {
             channels_containing_datatamer_schema.insert(channel_id);
             total_dt_schemas += statistics->channelMessageCounts.at(channel_id);
         }
         if (schema->name == "data_tamer_msgs/msg/Snapshot")
         {
             channels_containing_datatamer_data.insert(channel_id);
         }
         std::string channel_encoding = channel_ptr->messageEncoding;
         std::string schema_encoding = schema->encoding;
     }
     reader.readMessages();
     return 0;
 }

