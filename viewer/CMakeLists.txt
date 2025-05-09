add_library(mujoco_extend
        src/mujoco_chain.cpp
        src/mujoco_workcell.cpp
        src/mujoco_viewer.cpp
        src/mujoco_math.cpp
        src/mujoco_imgui_utils.cpp
        src/mujoco_imguizmo_utils.cpp
        src/mujoco_implot_utils.cpp
)
target_link_libraries(mujoco_extend mujoco)