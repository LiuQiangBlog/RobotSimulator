include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(imgui_docking STATIC
        docking/imgui.cpp
        docking/imgui_draw.cpp
        docking/imgui_tables.cpp
        docking/imgui_widgets.cpp
        docking/backends/imgui_impl_glfw.cpp
        docking/backends/imgui_impl_opengl3.cpp
        docking/imgui_demo.cpp
)

target_link_libraries(imgui_docking glew glfw)

add_executable(imgui_docking_demo main.cpp)
target_link_libraries(imgui_docking_demo imgui_docking glfw dl)
