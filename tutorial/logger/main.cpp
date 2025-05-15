//
// Created by liuqiang on 25-5-15.
//

#include "ChannelLogger.h"
#include <Eigen/Dense>
#include "Logging.h"

int main()
{
    Eigen::Vector3d tip;
    ChannelLogger logger("channel");
    logger.registerValue("pos", &tip);
    tip << 1, 2, 3;
    logger.takeSnapshot();
//    CLOG_INFO << logger.getSchema();
    auto schema = logger.getSchema();
    for (const auto& field : schema.fields)
    {
        CLOG_INFO << field.type_name;
        CLOG_INFO << field.field_name;
    }
    for (auto &[name, vec] : schema.custom_types)
    {
        CLOG_INFO << name; // variable tip's type_name
        for (auto &filed : vec)
        {
            CLOG_INFO << filed.field_name;
        }
    }
//    CLOG_INFO << logger.getSchema().custom_types["pos"][0];
//    for (const auto &field : logger.getSchema().fields)
//    {
//        CLOG_INFO << field.field_name;
//    }
//    for (auto &item : logger.getFlatFieldNames())
//    {
//        CLOG_INFO << item;
//    }
    return 0;
}