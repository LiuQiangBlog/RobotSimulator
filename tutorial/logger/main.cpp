//
// Created by liuqiang on 25-5-15.
//

#include "ChannelLogger.h"
#include <Eigen/Dense>
#include "Logging.h"
#include "data_tamer/sinks/publish_sink.hpp"

int main()
{
    Eigen::Vector3d tip;
    tip << 1, 2, 3;
    ChannelLogger logger("channel");
    logger.registerValue("pos", &tip);
    double a, b, c;
    logger.registerValue("a", &a);
    logger.registerValue("b", &b);
    std::vector<double> vec(7);
    logger.registerValue("q", &vec);
//    logger.takeSnapshot();
//    CLOG_INFO << logger.getSchema();
    auto schema = logger.getSchema();
    for (const auto& variable : schema.fields)
    {
        if (schema.custom_types.count(variable.type_name) > 0)
        {
            for (auto &element : schema.custom_types[variable.type_name])
            {
                CLOG_INFO << variable.field_name + "/" + element.field_name;
            }
        }
        else
        {
           CLOG_INFO << variable.field_name;
        }
    }
//    for (auto &item : logger.getFlatFieldNames())
//    {
//        CLOG_INFO << item;
//    }
    return 0;
}