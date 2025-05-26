//
// Created by liuqiang on 25-5-24.
//
#include <mcap/mcap.hpp>
#include <Logging.h>
#include <data_tamer/data_tamer.hpp>
#include <data_tamer/sinks/dummy_sink.hpp>
#include <data_tamer_parser/data_tamer_parser.hpp>

int main()
{
    std::string filepath("test.mcap");
    mcap::McapWriter writer;
    mcap::McapWriterOptions options("data_tamer");
    options.compression = mcap::Compression::Zstd;
    auto status = writer.open(filepath, options);
    if (!status.ok())
    {
        throw std::runtime_error("Failed to open MCAP file for writing");
    }
    std::unordered_map<uint64_t, uint16_t> hash_to_channel_id;
    std::unordered_map<std::string, DataTamer::Schema> schemas;

    std::string channel_name;
    DataTamer::Schema schema;
    schemas[channel_name] = schema;
    auto it = hash_to_channel_id.find(schema.hash);
    if (it != hash_to_channel_id.end())
    {
        return -1;
    }

    std::stringstream ss;
    ss << schema;
    std::string schema_str = ss.str();
    auto const schema_name = channel_name + "::" + std::to_string(schema.hash);
    // step1: Register a Schema
    mcap::Schema mcap_schema(schema_name, "data_tamer", schema_str);
    writer.addSchema(mcap_schema);
    // step2: Register a Channel
    mcap::Channel publisher(channel_name, "data_tamer", mcap_schema.id);
    writer.addChannel(publisher);
    hash_to_channel_id[schema.hash] = publisher.id;

    DataTamer::Snapshot snapshot;
    // the payload must contain both the ActiveMask and the other data
    std::vector<uint8_t> merged_payload;
    const auto size_mask = snapshot.active_mask.size();
    const auto size_data = snapshot.payload.size();

    merged_payload.resize(size_mask + size_data + sizeof(uint32_t) * 2);
    SerializeMe::SpanBytes buffer(merged_payload);
    SerializeMe::SerializeIntoBuffer(buffer, snapshot.active_mask);
    SerializeMe::SerializeIntoBuffer(buffer, snapshot.payload);

    // Write our message
    mcap::Message msg;
    msg.channelId = hash_to_channel_id.at(snapshot.schema_hash);
    msg.sequence = 1; // Optional
    // Timestamp requires nanosecond
    msg.logTime = mcap::Timestamp(snapshot.timestamp.count());
    msg.publishTime = msg.logTime;
    msg.data = reinterpret_cast<const std::byte *>(merged_payload.data()); // NOLINT
    msg.dataSize = merged_payload.size();
    // step3: write message into mcap file
    status = writer.write(msg);
    if (!status.ok())
    {
        CLOG_ERROR << "Write failed";
        return -1;
    }
    return 0;
}