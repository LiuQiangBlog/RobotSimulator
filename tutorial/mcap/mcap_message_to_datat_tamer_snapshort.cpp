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
        for (const auto &[schema_id, schema] : reader.schemas())
        {
            if (schema->encoding == "data_tamer")
            {
                std::string schema_str(reinterpret_cast<const char *>(schema->data.data()), schema->data.size());
                try
                {
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
        auto messages = reader.readMessages();
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

private:
    mcap::McapReader reader;
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
}
