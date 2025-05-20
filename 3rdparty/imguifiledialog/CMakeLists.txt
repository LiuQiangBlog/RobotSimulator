cmake_minimum_required(VERSION 3.5)

project(imfiledialog)

add_library(imfiledialog STATIC
    ImGuiFileDialog.cpp
    ImGuiFileDialog.h
    ImGuiFileDialogConfig.h
)

target_include_directories(imfiledialog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(UNIX)
    target_compile_options(imfiledialog PUBLIC -Wno-unknown-pragmas)
endif()
