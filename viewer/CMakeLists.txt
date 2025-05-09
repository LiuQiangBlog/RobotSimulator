add_library(viewer STATIC Viewer.h Viewer.cpp)
target_link_libraries(viewer mujoco)

add_executable(RobotSimulator main.cpp)
target_link_libraries(RobotSimulator viewer mujoco pthread glew glfw gizmo imgui implot)

