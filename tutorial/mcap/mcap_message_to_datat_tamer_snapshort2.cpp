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
#include <deque>

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
    explicit McapFileParser() : reader() {}

    bool open(const std::string &filepath)
    {
        // step1: open
        auto status = reader.open(filepath);
        if (!status.ok())
        {
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
        // step3: schemas and channels
        mcap_schemas = reader.schemas();
        mcap_channels = reader.channels();
        return true;
    }

    void publish()
    {
        auto&& messages = reader.readMessages();
        DataTamerParser::SnapshotView snapshot;
        for (auto it = messages.begin(); it != messages.end();)
        {
            const mcap::MessageView &msgView = *it;
            if (!ts.has_value())
            {
                ts = std::chrono::system_clock::time_point(std::chrono::nanoseconds(msgView.message.logTime));
            }
            if (mcap_schemas.count(msgView.schema->id) <= 0)
            {
                ++it;
                continue;
            }
            auto schema = mcap_schemas[msgView.schema->id];
            if (schema->encoding == "data_tamer")
            {
                std::string schema_str(reinterpret_cast<const char *>(schema->data.data()), schema->data.size());
                CLOG_INFO << schema_str;
                try
                {
                    DataTamerParser::Schema dt_schema = DataTamerParser::BuilSchemaFromText(schema_str, true);
                    const size_t data_size = msgView.message.dataSize;
                    const auto *data_ptr = reinterpret_cast<const uint8_t *>(msgView.message.data);
                    try
                    {
                        SerializeMe::SpanBytesConst buffer(data_ptr, data_size);

                        DataTamer::ActiveMask active_mask;
                        DataTamer::PayloadVector payload;

                        SerializeMe::DeserializeFromBuffer(buffer, active_mask);
                        SerializeMe::DeserializeFromBuffer(buffer, payload);

                        auto &logTime = msgView.message.logTime;
                        snapshot.schema_hash = dt_schema.hash;
                        snapshot.timestamp = logTime;
                        snapshot.active_mask = {active_mask.data(), active_mask.size()};
                        snapshot.payload = {payload.data(), payload.size()};

                        std::map<std::string, std::pair<double, double>> parsed_values; // timestamp, value
                        auto te = std::chrono::system_clock::time_point(std::chrono::nanoseconds(logTime));
                        auto sec = std::chrono::duration<double>(te - ts.value()).count();
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
                        std::scoped_lock lck(schema_mutex);
                        for (auto &[key, pair] : parsed_values)
                        {
                            channel_data[key].first.push_back(pair.first);
                            channel_data[key].second.push_back(pair.second);
                            if (channel_data[key].first.size() > 10000)
                            {
                                channel_data[key].first.pop_front();
                                channel_data[key].second.pop_front();
                            }
                            auto timestamps = std::vector<double>(channel_data[key].first.begin(), channel_data[key].first.end());
                            auto values = std::vector<double>(channel_data[key].second.begin(), channel_data[key].second.end());
                            channel_plot_data[key].first = timestamps;
                            channel_plot_data[key].second = values;
                        }
                    }
                    catch (const std::exception &e)
                    {
                        CLOG_ERROR << "Failed to deserialize snapshot: " << e.what();
                        return;
                    }
                }
                catch (const std::exception &e)
                {
                    CLOG_ERROR << "Failed to parse schema: " << e.what();
                    return;
                }
                ++it;
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
    std::optional<std::chrono::system_clock::time_point> ts;
    std::unordered_map<std::string, std::pair<std::deque<double>, std::deque<double>>> channel_data;
    std::unordered_map<std::string, std::pair<std::vector<double>, std::vector<double>>> channel_plot_data;
    Mutex schema_mutex;
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
