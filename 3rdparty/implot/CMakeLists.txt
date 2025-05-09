add_library(implot STATIC implot.cpp implot_items.cpp)
target_link_libraries(implot imgui glfw glew)
