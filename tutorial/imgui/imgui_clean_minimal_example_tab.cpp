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

// 标签页结构体
struct Tab
{
    char name[64];
    const char* content;
    int id; // 标识符，用于区分不同的tab页
};

std::vector<Tab> dynamic_tabs;
int selected_tab_id = -1; // 记录当前激活的tab的id

void ShowMyTabsWindow()
{
    ImGui::Begin("Tabs");

    // 标签栏
    if (ImGui::BeginTabBar("TabBar"))
    {
        // 遍历并渲染每个tab
        for (size_t i = 0; i < dynamic_tabs.size(); i++)
        {
            Tab& tab = dynamic_tabs[i];
            if (ImGui::BeginTabItem(tab.name))
            {
                // 记录当前激活的tab
                selected_tab_id = tab.id;
                // 显示标签内容
                ImGui::TextUnformatted(tab.content);
                ImGui::EndTabItem();
            }
            else
            {
                // 检查是否右键点击了标签
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                {
                    selected_tab_id = tab.id; // 记录当前右键点击的标签id
                    ImGui::OpenPopup("Tab Context Menu");
                }
            }
        }

        // 添加右侧的+按钮来创建新tab
        if (ImGui::Button("+"))
        {
            // 添加新标签
            Tab new_tab;
            strcpy(new_tab.name, "untitled");
            new_tab.content = "New tab content";
            new_tab.id = dynamic_tabs.size(); // 为新标签分配一个唯一的id
            dynamic_tabs.push_back(new_tab);
        }

        ImGui::EndTabBar();

        // 标签右键菜单
        if (ImGui::BeginPopup("Tab Context Menu"))
        {
            if (ImGui::MenuItem("Rename"))
            {
                // 打开重命名弹窗
                ImGui::OpenPopup("Rename Tab Dialog");
            }
            if (ImGui::MenuItem("Delete"))
            {
                // 删除当前选中的tab
                if (selected_tab_id != -1 && selected_tab_id < (int)dynamic_tabs.size())
                {
                    dynamic_tabs.erase(dynamic_tabs.begin() + selected_tab_id);
                    selected_tab_id = -1; // 重置选中的tab
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // 重命名标签对话框
        if (ImGui::BeginPopupModal("Rename Tab Dialog", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            char current_name[64];
            if (selected_tab_id != -1 && selected_tab_id < (int)dynamic_tabs.size())
            {
                strcpy(current_name, dynamic_tabs[selected_tab_id].name);
            }
            else
            {
                ImGui::CloseCurrentPopup();
                return;
            }

            char new_name[64] = "";
            ImGui::InputText("New Tab Name", new_name, IM_ARRAYSIZE(new_name));
            if (ImGui::Button("Rename", ImVec2(120, 0)))
            {
                if (new_name[0] && selected_tab_id != -1 && selected_tab_id < (int)dynamic_tabs.size())
                {
                    strcpy(dynamic_tabs[selected_tab_id].name, new_name);
                }
                ImGui::CloseCurrentPopup();
                selected_tab_id = -1;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
                selected_tab_id = -1;
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();
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

        if (show_demo_window)
        {
            ImGui::ShowDemoWindow(&show_demo_window);
            ImGui::DemoWindowWidgetsTabs();
        }
        ShowMyTabsWindow();
//        showTab(new_tab_name, dynamic_tabs, selected_tab_id);

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
