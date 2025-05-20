//
// Created by liuqiang on 25-3-21.
//
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "imgui_demo.cpp"

// 错误回调函数
static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char**)
{
    // 设置 GLFW 错误回调
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // 配置 GLFW
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // 创建 GLFW 窗口
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // 开启垂直同步

    // 初始化 ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // 启用键盘控制
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // 启用游戏手柄控制

    // 设置 ImGui 样式
    ImGui::StyleColorsDark();

    // 初始化 ImGui 后端
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // 主循环
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // 开始新的 ImGui 帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 显示演示窗口
        ImGui::ShowDemoWindow();

        // 渲染 ImGui
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // 清理并关闭
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
