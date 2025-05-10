#add_library(viewer Viewer.h Viewer.cpp)
#target_link_libraries(viewer imgui mujoco pthread glew glfw gizmo implot)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../cmake-build-debug/lib)
add_executable(RobotSimulator main.cpp Viewer.h Viewer.cpp DataVisualizer.cpp DataVisualizer.h)
target_link_libraries(RobotSimulator mujoco glew glfw imgui implot gizmo data_tamer dl)

