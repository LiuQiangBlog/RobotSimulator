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

class DataTamerMCAPParser
{
public:
    explicit DataTamerMCAPParser(const std::string &filepath)
        : reader_(), message_view_(std::make_unique<mcap::LinearMessageView>(reader_, [](const mcap::Status &) {})),
          iter_(message_view_->begin()), // 显式初始化迭代器
          end_(message_view_->end())
    {
        auto status = reader_.open(filepath);
        if (!status.ok())
        {
            throw std::runtime_error("Failed to open MCAP file: " + status.message);
        }
        loadSchemas();
    }

    std::optional<DataTamerParser::SnapshotView> next()
    {
        if (iter_ == end_)
        {
            return std::nullopt;
        }

        // 获取当前消息
        const mcap::MessageView &msgView = *iter_;
        ++iter_; // 安全递增迭代器

        // 通过channel_name查找对应的Schema
        const std::string &channel_name = msgView.channel->topic;

        auto schema_it = std::find_if(schemas_.begin(), schemas_.end(),
                                      [&](const auto &schema)
                                      {
                                          return schema.channel_name == channel_name;
                                      });

        if (schema_it == schemas_.end())
        {
            return std::nullopt;
        }

        // 构建SnapshotView
        DataTamerParser::SnapshotView snapshot;
        snapshot.schema_hash = schema_it->hash;
        snapshot.timestamp = msgView.message.logTime;
        snapshot.active_mask = {
            reinterpret_cast<const uint8_t *>(msgView.message.data),
            sizeof(uint64_t) // 假设active_mask占前8字节
        };
        snapshot.payload = {reinterpret_cast<const uint8_t *>(msgView.message.data + sizeof(uint64_t)),
                            msgView.message.dataSize - sizeof(uint64_t)};

        return snapshot;
    }

    const std::vector<DataTamerParser::Schema> &getSchemas() const
    {
        return schemas_;
    }

private:
    mcap::McapReader reader_;
    std::unique_ptr<mcap::LinearMessageView> message_view_;
    mcap::LinearMessageView::Iterator iter_;
    mcap::LinearMessageView::Iterator end_;
    std::vector<DataTamerParser::Schema> schemas_;

    void loadSchemas()
    {
        for (const auto &[mcap_schema_id, mcap_schema] : reader_.schemas())
        {
            if (mcap_schema->encoding == "data_tamer")
            {
                // 将MCAP的Schema.data转换为字符串
                std::string schema_str(reinterpret_cast<const char *>(mcap_schema->data.data()),
                                       mcap_schema->data.size());

                // 使用官方函数解析Schema
                try
                {
                    DataTamerParser::Schema dt_schema = DataTamerParser::BuilSchemaFromText(schema_str, true);
                    schemas_.push_back(dt_schema);
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Failed to parse schema: " << e.what() << std::endl;
                }
            }
        }
    }
};
