add_library(logger ChannelLogger.cpp)
target_link_libraries(logger pthread data_tamer zcm dl)

add_executable(channel_logger_demo main.cpp)

