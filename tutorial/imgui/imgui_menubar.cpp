//
// Created by liuqiang on 25-5-13.
//

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <cstdio>
#include "GLFW/glfw3.h" // Will drag system OpenGL headers
#include <string>
#include <vector>
#include <Logging.h>

static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static void centerWindow(GLFWwindow *window)
{
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    if (!monitor)
    {
        return;
    }
    int xpos, ypos, width, height;
    glfwGetMonitorWorkarea(monitor, &xpos, &ypos, &width, &height);
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    int centerX = xpos + (width - windowWidth) / 2;
    int centerY = ypos + (height - windowHeight) / 2;
    glfwSetWindowPos(window, centerX, centerY);
}

// void createMenuBar()
//{
//     if (ImGui::Begin("createMenuBar"))
//     {
//         if (ImGui::BeginMenuBar())
//         {
//             if (ImGui::BeginMenu("File"))
//             {
//                 if (ImGui::MenuItem("New", "Ctrl+N"))
//                 {
//                     // 处理新建操作
//                 }
//                 if (ImGui::MenuItem("Open", "Ctrl+O"))
//                 {
//                     // 处理打开操作
//                 }
//                 if (ImGui::MenuItem("Exit", "Alt+F4"))
//                 {
//                     // 处理退出操作
//                 }
//             }
//             ImGui::EndMenuBar();
//         }
//     }
//     ImGui::End(); // End can not in if(ImGui::Begin()), otherwise program will crash
// }

void createMenuBar()
{
    if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
    }
    if (ImGui::GetIO().KeyMods & ImGuiMod_Ctrl)
    {
        if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_N) == GLFW_PRESS)
        {
            CLOG_INFO << "Create New File";
        }
        if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_O) == GLFW_PRESS)
        {
            CLOG_INFO << "Open File";
        }
    }
    if (ImGui::BeginMainMenuBar()) // 使用 BeginMainMenuBar 创建全局菜单栏
    {
        if (ImGui::MenuItem("New"))
        {
        }
        ImGui::Separator();
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New", "Ctrl+N"))
            {
                CLOG_INFO << "Create New File";
            }
            if (ImGui::MenuItem("Open", "Ctrl+O"))
            {
                CLOG_INFO << "Open File";
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
            }
            if (ImGui::MenuItem("Save As.."))
            {
            }
            if (ImGui::MenuItem("Exit", "Esc", false, false))
            {
                CLOG_INFO << "Exit";
                glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
            }
            ImGui::BeginChild("child", ImVec2(0, 60), ImGuiChildFlags_Borders);
            for (int i = 0; i < 10; i++)
                ImGui::Text("Scrolling Text %d", i);
            ImGui::EndChild();
            static float f = 0.5f;
            static int n = 0;
            ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);
            ImGui::InputFloat("Input", &f, 0.1f);
            ImGui::Combo("Combo", &n, "Yes\0No\0Maybe\0\0");
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void setMenubarColor()
{
    ImGuiStyle &style = ImGui::GetStyle();
    // 设置主菜单栏背景色（深灰色）
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    // 设置菜单项颜色
    style.Colors[ImGuiCol_Header] = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);     // 激活状态
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.5f, 0.7f, 1.0f);  // 悬停状态
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2f, 0.5f, 0.8f, 0.6f); // 文本选中状态

    // 设置下拉菜单颜色
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.2f, 0.2f, 0.2f, 0.95f);      // 菜单面板背景色
    style.Colors[ImGuiCol_Border] = ImVec4(0.4f, 0.4f, 0.4f, 0.5f);        // 菜单边框色
    style.Colors[ImGuiCol_Text] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);          // 菜单项文本色
    style.Colors[ImGuiCol_Button] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);        // 菜单项默认背景
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.5f, 0.7f, 1.0f); // 菜单项悬停背景
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.4f, 0.6f, 0.8f, 1.0f);  // 菜单项激活背景
}

void createMainWindow()
{
    int windowWidth, windowHeight;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &windowWidth, &windowHeight);
    ImGui::SetNextWindowSize(ImVec2((float)windowWidth, (float)windowHeight));
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::Begin("##Main Window", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    {
        // 在这里添加其他 imgui 控件
        ImGui::Text("Hello, ImGui!");
    }
    ImGui::End();
}

// Main code
int main(int, char **)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
    {
        return 1;
    }
    // GL 3.3 + GLSL 130
    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // 3.0+ only

    // Create window with graphics context
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(1280, 720, "Untitled", nullptr, nullptr);
    glfwSetWindowTitle(window, "Hello ImGui");
    if (window == nullptr)
    {
        return 1;
    }
    centerWindow(window);
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
    //    ImGui::StyleColorsDark();
    //     ImGui::StyleColorsLight();
    // ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    ImVec4 clear = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    io.AddKeyEvent(ImGuiKey_Escape, true);

    setMenubarColor();
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

        // todo, here is your imgui code
        createMenuBar(); // 调用创建菜单栏的函数
                         //        createMainWindow();

        // 在渲染循环中添加样式编辑器
        //        ImGui::Begin("Style Editor");
        //        ImGui::ShowStyleEditor();
        //        ImGui::End();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear.x * clear.w, clear.y * clear.w, clear.z * clear.w, clear.w);
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
