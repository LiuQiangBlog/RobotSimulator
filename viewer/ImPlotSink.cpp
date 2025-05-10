//
// Created by liuqiang on 25-5-10.
//

#include "ImPlotSink.h"

#include "data_tamer_parser/data_tamer_parser.hpp"
#include "implot.h"

struct ParsedData {
    double timestamp;
    double value;
};
std::vector<ParsedData> parsedDataList;

class MsgParserImpl
{
public:
    MsgParserImpl(const std::string& topic_name, const std::string& type_name,
                  const std::string& schema)
        : topic_name_(topic_name)
    {
        schema_ = DataTamerParser::BuilSchemaFromText(schema);
    }

    bool parseMessage(const MessageRef serialized_msg, double& timestamp) override
    {
        auto callback = [this, &parsedDataList, timestamp](const std::string& series_name,
                                                           const DataTamerParser::VarNumber& var) {
            double value = std::visit([](auto&& v) { return static_cast<double>(v); }, var);
            ParsedData dataPoint = {timestamp, value};
            parsedDataList.push_back(dataPoint);
        };

        DataTamerParser::SnapshotView snapshot;
        snapshot.schema_hash = schema_.hash;

        DataTamerParser::BufferSpan msg_buffer = {serialized_msg.data(),
                                                  serialized_msg.size()};

        const uint32_t mask_size = DataTamerParser::Deserialize<uint32_t>(msg_buffer);
        snapshot.active_mask.data = msg_buffer.data;
        snapshot.active_mask.size = mask_size;
        msg_buffer.trimFront(mask_size);

        const uint32_t payload_size = DataTamerParser::Deserialize<uint32_t>(msg_buffer);
        snapshot.payload.data = msg_buffer.data;
        snapshot.payload.size = payload_size;

        DataTamerParser::ParseSnapshot(schema_, snapshot, callback);
        return true;
    }

private:
    DataTamerParser::Schema schema_;
    std::string topic_name_;
};

//... 可能存在的 createParser 函数调整

// 假设在主函数中
int main() {
    // 初始化 ImPlot
    ImPlot::CreateContext();
    // 创建解析器并解析数据
    MsgParserImpl parser("topic", "type", "schema");
    double timestamp = 0.0;
    MessageRef serialized_msg; // 假设已经有序列化的消息
    parser.parseMessage(serialized_msg, timestamp);

    // 绘制图形
    drawPlot(parsedDataList);

    // 清理 ImPlot
    ImPlot::DestroyContext();
    return 0;
}

void drawPlot(const std::vector<ParsedData>& data) {
    ImPlot::SetNextPlotLimitsY(0, 10);
    ImPlot::BeginPlot("Data Plot");
    ImPlot::PlotLine("Data Series",
                     &data[0].timestamp,
                     &data[0].value,
                     data.size());
    ImPlot::EndPlot();
}