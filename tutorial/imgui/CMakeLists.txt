function(AddTutorials Path)
    file(GLOB examples ${Path}/*.cpp)
    foreach (example ${examples})
        message(STATUS "File Path: ${example}")
        string(REGEX MATCHALL "[0-9A-Za-z_]*.cpp" file_name ${example})
        string(REGEX REPLACE ".cpp" "" file_base ${file_name})
        message(${file_base})
        #        ${ARGC}
        #        ${ARGV} ${ARGV0} ${ARGV1} ${ARGV2}
        #        get_filename_component(file_base ${example} NAME_WE)
        add_executable(${file_base} ${example})
        target_link_libraries(${file_base} ${ARGN})
    endforeach ()
endfunction()

find_library(NVML_LIBRARY nvidia-ml)
find_path(NVML_INCLUDE_DIR nvml.h
        PATHS /usr/include /usr/include/nvidia/gdk
        NO_DEFAULT_PATH)
# 检查是否找到NVML
if(NOT NVML_LIBRARY OR NOT NVML_INCLUDE_DIR)
    message(WARNING "NVML library not found. GPU monitoring will be disabled.")
    set(DISABLE_GPU_MONITORING ON)
else()
    message(STATUS "Found NVML library: ${NVML_LIBRARY}")
    message(STATUS "Found NVML headers: ${NVML_INCLUDE_DIR}")
endif()
include_directories(${NVML_INCLUDE_DIR})
AddTutorials(${CMAKE_CURRENT_SOURCE_DIR} pthread glew imgui glfw implot imcoolbar zcm dl ${NVML_LIBRARY})

