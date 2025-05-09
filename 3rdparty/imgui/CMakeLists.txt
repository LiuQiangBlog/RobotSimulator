include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(imgui STATIC
        imgui.cpp
        imgui_draw.cpp
        imgui_tables.cpp
        imgui_widgets.cpp
        backends/imgui_impl_glfw.cpp
        backends/imgui_impl_opengl3.cpp
        imgui_demo.cpp
)

target_link_libraries(imgui glew glfw)

add_executable(imgui_demo main.cpp)
target_link_libraries(imgui_demo imgui glfw dl)
