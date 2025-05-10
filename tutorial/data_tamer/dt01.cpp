//
// Created by liuqiang on 25-5-10.
//
#include <data_tamer/data_tamer.hpp>
#include <data_tamer/sinks/dummy_sink.hpp>
#include <data_tamer_parser/data_tamer_parser.hpp>
#include <gtest/gtest.h>

TEST(DataTamerParser, ReadSchema)
{
    const char *text = R"(
int8 v1
float64 v2
float32[5] array
int32[] vect
bool is_true
char[256] blob
uint16  my/short
  )";
    const auto schema = DataTamerParser::BuilSchemaFromText(text);
    ASSERT_EQ(schema.fields.size(), 7);

    auto channel = DataTamer::LogChannel::create("channel");
    auto dummy_sink = std::make_shared<DataTamer::DummySink>();
    channel->addDataSink(dummy_sink);

}