cmake_minimum_required(VERSION 3.1)

set(PROJECT ImTools)

enable_language(C CXX)

project(${PROJECT} CXX)

## for group make targets in the dir cmakeTargets
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "cmakeTargets")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(UNIX)
	if(APPLE)
		add_definitions(-DAPPLE)
	else()
		add_definitions(-DLINUX)
	endif()
	add_definitions(-DUNIX)
elseif(WIN32)
	add_definitions(-DWIN32)
	if(MINGW)
		add_definitions(-DMINGW32)
		add_definitions(-DHAVE_STRTOK_R)
		add_definitions(-DHAVE___BUILTIN_FFS)
	endif()
endif()

cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLEW)

## ------------ Layout Manager ------------ ##
file(GLOB APP_LAYOUT_MANAGER_SOURCES
		apps/App_LayoutManager/*.cpp
		apps/App_LayoutManager/*.h
		LayoutManager/*.cpp
		LayoutManager/*.h
#		3rdparty/imgui/backends/imgui_impl_opengl3.cpp
#		3rdparty/imgui/backends/imgui_impl_opengl3.h
#		3rdparty/imgui/backends/imgui_impl_glfw.cpp
#		3rdparty/imgui/backends/imgui_impl_glfw.h
)

add_executable(app_LayoutManager ${APP_LAYOUT_MANAGER_SOURCES})
target_link_libraries(app_LayoutManager PRIVATE glew glfw imgui ctools tinyxml2 dl)



