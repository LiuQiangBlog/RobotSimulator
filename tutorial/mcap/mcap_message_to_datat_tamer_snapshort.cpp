//
// Created by liuqiang on 25-5-25.
//
#include <mcap/mcap.hpp>
#include <data_tamer/data_tamer.hpp>
#include <data_tamer/data_tamer.hpp>
#include <data_tamer/sinks/dummy_sink.hpp>
#include <data_tamer_parser/data_tamer_parser.hpp>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <Logging.h>

void problem(const mcap::Status &status)
{
    if (!status.ok())
    {
        CLOG_ERROR << "mcap error: " << status.message;
    }
}

class McapFileParser
{
public:
    std::vector<DataTamerParser::Schema> schemas;
    std::vector<DataTamerParser::SnapshotView> snapshots;

    explicit McapFileParser() : reader() {}

    bool open(const std::string &filepath)
    {
        // step1: open
        auto status = reader.open(filepath);
        if (!status.ok())
        {
            // throw std::runtime_error("Failed to open MCAP file: " + status.message);
            CLOG_ERROR << "Failed to open MCAP file: " << status.message;
            return false;
        }
        // step2: readSummary
        status = reader.readSummary(mcap::ReadSummaryMethod::NoFallbackScan, problem);
        if (!status.ok())
        {
            CLOG_ERROR << "Can't open summary of the file";
            return -1;
        }
        // step3: schemas
        mcap_schemas = reader.schemas();
        mcap_channels = reader.channels();
        for (const auto &[schema_id, schema] : reader.schemas())
        {
            if (schema->encoding == "data_tamer")
            {
                std::string schema_str(reinterpret_cast<const char *>(schema->data.data()), schema->data.size());
                try
                {
                    CLOG_INFO << schema_str;
                    DataTamerParser::Schema dt_schema = DataTamerParser::BuilSchemaFromText(schema_str, true);
                    schemas.push_back(dt_schema);
                }
                catch (const std::exception &e)
                {
                    CLOG_ERROR << "Failed to parse schema: " << e.what();
                    return false;
                }
            }
        }
        // step4: snapshots
        auto&& messages = reader.readMessages();
        DataTamerParser::SnapshotView snapshot;
        for (auto it = messages.begin(); it != messages.end();)
        {
            const mcap::MessageView &msgView = *it;
            const std::string &channel_name = msgView.channel->topic;
            auto schema_it = std::find_if(schemas.begin(), schemas.end(),
                                          [&](const auto &schema)
                                          {
                                              return schema.channel_name == channel_name;
                                          });

            if (schema_it == schemas.end())
            {
                return false;
            }
            const size_t data_size = msgView.message.dataSize;
            const auto *data_ptr = reinterpret_cast<const uint8_t *>(msgView.message.data);
            try
            {
                SerializeMe::SpanBytesConst buffer(data_ptr, data_size);

                DataTamer::ActiveMask active_mask;
                DataTamer::PayloadVector payload;

                SerializeMe::DeserializeFromBuffer(buffer, active_mask);
                SerializeMe::DeserializeFromBuffer(buffer, payload);

                snapshot.schema_hash = schema_it->hash;
                snapshot.timestamp = msgView.message.logTime;
                snapshot.active_mask = {active_mask.data(), active_mask.size()};
                snapshot.payload = {payload.data(), payload.size()};
            }
            catch (const std::exception &e)
            {
                CLOG_ERROR << "Failed to deserialize snapshot: " << e.what();
                return false;
            }
            ++it;
            snapshots.push_back(snapshot);
        }
        return true;
    }

    static bool parse(const DataTamerParser::Schema &schema, DataTamerParser::SnapshotView snapshot_view)
    {
        std::map<std::string, std::pair<double, double>> parsed_values; // timestamp, value
        auto sec = std::chrono::seconds(snapshot_view.timestamp).count();
        auto callback = [&](const std::string &field_name, const DataTamerParser::VarNumber &number)
        {
            const double value = std::visit(
                [](const auto &var)
                {
                    return double(var);
                },
                number);
            parsed_values[field_name] = {sec, value}; // timestamp unit is s
        };
        DataTamerParser::ParseSnapshot(schema, snapshot_view, callback);
        return true;
    }

    void publish()
    {
        auto&& messages = reader.readMessages();
        DataTamerParser::SnapshotView snapshot;
        for (auto it = messages.begin(); it != messages.end();)
        {
            const mcap::MessageView &msgView = *it;
            if (mcap_schemas.count(msgView.schema->id) <= 0 || mcap_channels.count(msgView.channel->id) <= 0)
            {
                continue;
            }
            auto schema = mcap_schemas[msgView.schema->id];
            if (schema->encoding == "data_tamer")
            {
                std::string schema_str(reinterpret_cast<const char *>(schema->data.data()), schema->data.size());
                DataTamerParser::Schema dt_schema;
                try
                {
                    CLOG_INFO << schema_str;
                    dt_schema = DataTamerParser::BuilSchemaFromText(schema_str, true);
                }
                catch (const std::exception &e)
                {
                    CLOG_ERROR << "Failed to parse schema: " << e.what();
                    return;
                }
                const size_t data_size = msgView.message.dataSize;
                const auto *data_ptr = reinterpret_cast<const uint8_t *>(msgView.message.data);
                try
                {
                    SerializeMe::SpanBytesConst buffer(data_ptr, data_size);

                    DataTamer::ActiveMask active_mask;
                    DataTamer::PayloadVector payload;

                    SerializeMe::DeserializeFromBuffer(buffer, active_mask);
                    SerializeMe::DeserializeFromBuffer(buffer, payload);

                    snapshot.schema_hash = dt_schema.hash;
                    snapshot.timestamp = msgView.message.logTime;
                    snapshot.active_mask = {active_mask.data(), active_mask.size()};
                    snapshot.payload = {payload.data(), payload.size()};

                    std::map<std::string, std::pair<double, double>> parsed_values; // timestamp, value
                    static auto ts{std::chrono::system_clock::time_point(std::chrono::nanoseconds(msgView.message.logTime))};
                    auto te = std::chrono::system_clock::time_point(std::chrono::nanoseconds(msgView.message.logTime));
                    auto sec = std::chrono::duration<double>(te - ts).count();
                    auto callback = [&](const std::string &field_name, const DataTamerParser::VarNumber &number)
                    {
                        const double value = std::visit(
                            [](const auto &var)
                            {
                                return double(var);
                            },
                            number);
                        parsed_values[field_name] = {sec, value}; // timestamp unit is s
                    };
                    DataTamerParser::ParseSnapshot(dt_schema, snapshot, callback);
                }
                catch (const std::exception &e)
                {
                    CLOG_ERROR << "Failed to deserialize snapshot: " << e.what();
                    return;
                }
                ++it;
                snapshots.push_back(snapshot);
            }
        }
    }

    void close()
    {
        reader.close();
    }

private:
    mcap::McapReader reader;
    std::unordered_map<mcap::SchemaId, mcap::SchemaPtr> mcap_schemas;
    std::unordered_map<mcap::ChannelId, mcap::ChannelPtr> mcap_channels;
};

int main()
{
    std::string filepath("/home/liuqiang/ClionProjects/RobotSimulator/cmake-build-debug/bin/test_sample.mcap");
    McapFileParser parser;
    if (parser.open(filepath))
    {
        for (auto &schema : parser.schemas)
        {
            CLOG_INFO << schema.channel_name;
        }
    }
    parser.close();
    return 0;
}
