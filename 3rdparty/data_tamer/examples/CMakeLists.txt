
function(CompileExample TARGET)
    add_executable(${TARGET} ${TARGET}.cpp)
    target_include_directories(${TARGET}
            PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
    target_link_libraries(${TARGET} data_tamer pthread)
endfunction()

CompileExample(T01_basic_example)
CompileExample(T02_custom_types)
CompileExample(T03_mcap_writer)
CompileExample(mcap_1m_per_sec)

add_executable(mcap_reader mcap_reader.cpp)
target_include_directories(mcap_reader
        PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

target_link_libraries(mcap_reader data_tamer ${mcap_LIBRARY})
target_include_directories(mcap_reader
        PRIVATE
        $<BUILD_INTERFACE:${PROJECT_DIR}/3rdparty>
)
