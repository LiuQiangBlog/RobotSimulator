//
// Created by liuqiang on 25-5-13.
//

#include "docking/imgui.h"
#include "docking/backends/imgui_impl_glfw.h"
#include "docking/backends/imgui_impl_opengl3.h"
#include <cstdio>
#include "GLFW/glfw3.h" // Will drag system OpenGL headers
#include "ImCoolBar.h"

static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// 新增：将窗口居中显示的函数
static void centerWindow(GLFWwindow *window)
{
    // 获取主显示器
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    if (!monitor)
        return;

    // 获取显示器工作区域（排除任务栏等）
    int xpos, ypos, width, height;
    glfwGetMonitorWorkarea(monitor, &xpos, &ypos, &width, &height);

    // 获取窗口大小
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    // 计算居中位置
    int centerX = xpos + (width - windowWidth) / 2;
    int centerY = ypos + (height - windowHeight) / 2;

    // 设置窗口位置
    glfwSetWindowPos(window, centerX, centerY);
}

// Main code
int main(int, char **)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

    // Create window with graphics context
    // 创建不可见的窗口
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    // 新增：在窗口创建后但未显示前调用居中函数
    centerWindow(window);
    // 显示窗口
    glfwShowWindow(window);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code
        // to learn more about Dear ImGui!).
        if (show_demo_window)
        {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        auto coolbar_button = [](const char *label) -> bool
        {
            float w = ImGui::GetCoolBarItemWidth();
            auto font_ptr = ImGui::GetIO().Fonts->Fonts[0];
            font_ptr->Scale = ImGui::GetCoolBarItemScale();
            ImGui::PushFont(font_ptr);
            bool res = ImGui::Button(label, ImVec2(w, w));
            ImGui::PopFont();
            return res;
        };

        if (ImGui::BeginCoolBar("##CoolBarMain", ImCoolBarFlags_Horizontal, ImVec2(0.5f, 1.0f)))
        {
            if (ImGui::CoolBarItem())
            {
                if (coolbar_button("A"))
                {
                }
            }
            if (ImGui::CoolBarItem())
            {
                if (coolbar_button("B"))
                {
                }
            }
            if (ImGui::CoolBarItem())
            {
                if (coolbar_button("C"))
                {
                }
            }
            if (ImGui::CoolBarItem())
            {
                if (coolbar_button("D"))
                {
                }
            }
            if (ImGui::CoolBarItem())
            {
                if (coolbar_button("E"))
                {
                }
            }
            if (ImGui::CoolBarItem())
            {
                if (coolbar_button("F"))
                {
                }
            }
            if (ImGui::CoolBarItem())
            {
                if (coolbar_button("G"))
                {
                }
            }
            if (ImGui::CoolBarItem())
            {
                if (coolbar_button("H"))
                {
                }
            }
            if (ImGui::CoolBarItem())
            {
                if (coolbar_button("I"))
                {
                }
            }
            if (ImGui::CoolBarItem())
            {
                if (coolbar_button("J"))
                {
                }
            }
            if (ImGui::CoolBarItem())
            {
                if (coolbar_button("K"))
                {
                }
            }
            if (ImGui::CoolBarItem())
            {
                if (coolbar_button("L"))
                {
                }
            }
            if (ImGui::CoolBarItem())
            {
                if (coolbar_button("M"))
                {
                }
            }
            ImGui::EndCoolBar();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
                     clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
