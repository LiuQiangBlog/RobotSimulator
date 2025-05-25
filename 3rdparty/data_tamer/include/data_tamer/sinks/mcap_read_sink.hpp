#pragma once

#include "data_tamer/data_sink.hpp"

#include <mutex>
#include <unordered_map>
#include <mcap/writer.hpp>
#include <mcap/reader.hpp>
#include <Logging.h>
#include <set>

// Forward declaration
namespace mcap
{
class McapWriter;
}

namespace DataTamer
{

/**
 * @brief The MCAPSink is an implementation of DataSinkBase that
 * will save the data as MCAP file (https://mcap.dev/)
 */
class MCAPReadSink : public DataSinkBase
{
public:
    /**
     * @brief MCAPSink.
     * IMPORTANT: if you want the recorder to be more robust to crash/segfault,
     * set `do_compression` to false.
     * Compression is safe if your application is closing cleanly.
     *
     * @param filepath   path of the file to be saved. Should have extension ".mcap"
     * @param do_compression if true, compress the data on the fly.
     */
    explicit MCAPReadSink(std::string const &filepath, bool do_compression = false);

    ~MCAPReadSink() override;

    void addChannel(std::string const &channel_name, Schema const &schema) override;

    bool storeSnapshot(const Snapshot &snapshot) override;

    /// After a certain amount of time, the MCAP file will be reset
    /// and overwritten. Default value is 600 seconds (10 minutes)
    /// To disable this feature, use a time of 0 seconds.
    /// WARNING: this can consume a large amount of disk space very quickly.
    void setMaxTimeBeforeReset(std::chrono::seconds reset_time);

    /// When resetting the MCAP recording (see `setMaxTimeBeforeReset`),
    /// if `create_new_file` is true then the filename will be incremented
    /// and then saved instead of overwriting the previous file.
    void setCreateNewFileOnReset(bool create_new_file);

    /// Stop recording and save the file
    void stopRecording();

    /**
     * @brief restartRecording saves the current file (unless we did it already,
     * calling stopRecording) and start recording into a new one.
     * Note that all the registered channels and their schemas will be copied into the new file.
     *
     * @param filepath   file path of the new file (should be ".mcap" extension)
     * @param do_compression if true, compress the data on the fly.
     * WARNING: if this is called with the same filename as previously, the file counter will be reset, too.
     */
    void restartRecording(std::string const &filepath, bool do_compression = false);

    const std::vector<const char *> &compatibleFileExtensions() const
    {
        static std::vector<const char *> ext = {"mcap", "MCAP"};
        return ext;
    }

    bool readDataFromFile(const std::string &filepath)
    {
        // open file
        mcap::McapReader reader;
        // step1: open
        auto status = reader.open(filepath);
        if (!status.ok())
        {
            CLOG_ERROR << "Can't open file";
            return false;
        }
        // step2: readSummary
        status = reader.readSummary(mcap::ReadSummaryMethod::NoFallbackScan);
        if (!status.ok())
        {
            CLOG_ERROR << "Can't open summary of the file";
            return false;
        }
        // step3: statistics
        auto statistics = reader.statistics();
        std::unordered_map<int, mcap::SchemaPtr> mcap_schemas; // schema_id
        std::unordered_map<int, mcap::ChannelPtr> channels;    // channel_id
        uint64_t total_dt_schemas = 0;

        std::unordered_set<mcap::ChannelId> channels_containing_datatamer_schema;
        std::unordered_set<mcap::ChannelId> channels_containing_datatamer_data;
        // step4: schemas
        for (const auto &[schema_id, schema_ptr] : reader.schemas())
        {
            mcap_schemas.insert({schema_id, schema_ptr});
        }
        // step5: channels
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
        return true;
    }

private:
    std::string filepath_;
    bool compression_ = false;
    std::unique_ptr<mcap::McapWriter> writer_;

    std::unordered_map<uint64_t, uint16_t> hash_to_channel_id_;
    std::unordered_map<std::string, Schema> schemas_;

    // file reset variables
    bool create_file_on_reset_ = false;
    std::string original_filepath_;
    size_t file_reset_counter_ = 1;

    std::chrono::seconds reset_time_ = std::chrono::seconds(60 * 10);
    std::chrono::system_clock::time_point start_time_;

    bool forced_stop_recording_ = false;
    std::recursive_mutex mutex_;
    std::set<std::string> notified_encoding_problem;

    void openFile(std::string const &filepath);
    void restartRecordingImpl(std::string const &filepath, bool do_compression, bool new_file);
};

} // namespace DataTamer
