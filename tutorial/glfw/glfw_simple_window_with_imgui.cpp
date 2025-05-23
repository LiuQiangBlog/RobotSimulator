//
// Created by liuqiang on 25-5-21.
//
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

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

int main()
{
    if (!glfwInit())
    {
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // 3.0+ only

//    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // hide window title
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);   // hide window first
    GLFWwindow *window = glfwCreateWindow(1280, 720, "Hello ImGui", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    centerWindow(window);   // move window to center
    glfwShowWindow(window); // show window
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    float clear[] = {0.45f, 0.55f, 0.60f, 1.00f};

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    ImVec2 view_port_pos, view_port_size;
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

        if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
        }

        // 窗口的 ID 和 标题
        ImGuiID dockspaceID = ImGui::GetID("##ui.dock_space");
        const char *UI_DOCK_WINDOW = "##ui.dock_window";
        const char *UI_PROJECT_BOX = u8"工程##ui.project";
        const char *UI_PROPERTY_BOX = u8"属性##ui.property";
        const char *UI_TOOL_BOX = u8"工具##ui.tools";
        const char *UI_MESSAGE_BOX = u8"消息##ui.message";
        const char *UI_LOG_BOX = u8"日志##ui.log";
        const char *UI_VIEW_BOX = u8"##ui.view";

        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        int windowFlags = ImGuiWindowFlags_NoDecoration // 无标题栏、不可改变大小、无滚动条、不可折叠
                          | ImGuiWindowFlags_NoMove     // 不可移动
                          | ImGuiWindowFlags_NoBackground          // 无背景（背景透明）
                          | ImGuiWindowFlags_MenuBar               // 菜单栏
                          | ImGuiWindowFlags_NoDocking             // 不可停靠
                          | ImGuiWindowFlags_NoBringToFrontOnFocus // 无法设置前台和聚焦
                          | ImGuiWindowFlags_NoNavFocus            // 无法通过键盘和手柄聚焦
            ;

        // 压入样式设置
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);            // 无边框
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f)); // 无边界
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);              // 无圆角
        // ImGui::SetNextWindowBgAlpha(0.0f); // 窗口 alpha 为 0，同样可以不显示背景

        ImGui::Begin(UI_DOCK_WINDOW, nullptr, windowFlags); // 开始停靠窗口
        ImGui::PopStyleVar(3);                        // 弹出样式设置

        // 创建停靠空间
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
        { // 判断是否开启停靠
            // 判断是否有根节点，防止一直重建
            if (!ImGui::DockBuilderGetNode(dockspaceID))
            {
                // 移除根节点
                ImGui::DockBuilderRemoveNode(dockspaceID);

                // 创建根节点
                ImGuiID root = ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_None);

                // 设置根节点位置大小
                ImGui::DockBuilderSetNodePos(root, {0.0f, 0.0f});
                ImGui::DockBuilderSetNodeSize(root, ImGui::GetWindowSize());

                // 分割停靠空间

                // 根节点分割左节点
                ImGuiID leftNode = ImGui::DockBuilderSplitNode(root, ImGuiDir_Left, 0.25f, nullptr, &root);

                // 根节点分割右节点
                ImGuiID rightNode = ImGui::DockBuilderSplitNode(root, ImGuiDir_Right, 0.25f / 0.75f, nullptr, &root);

                // 根节点分割下节点
                ImGuiID bottomNode = ImGui::DockBuilderSplitNode(root, ImGuiDir_Down, 0.25f, nullptr, &root);

                // 左节点分割上下节点
                ImGuiID leftTopNode, leftBottomNode;
                ImGui::DockBuilderSplitNode(leftNode, ImGuiDir_Up, 0.5f, &leftTopNode, &leftBottomNode);

                // 设置节点属性

                // 禁止其他窗口/节点分割根节点
                // ImGui::DockBuilderGetNode(dockspaceID)->LocalFlags |= ImGuiDockNodeFlags_NoDockingSplit;

                // 设置分割到最后的根节点隐藏标签栏
                ImGui::DockBuilderGetNode(root)->LocalFlags |= ImGuiDockNodeFlags_HiddenTabBar;

                // 设置节点停靠窗口
                ImGui::DockBuilderDockWindow(UI_PROJECT_BOX, leftTopNode);     // 左上节点
                ImGui::DockBuilderDockWindow(UI_PROPERTY_BOX, leftBottomNode); // 左下节点
                ImGui::DockBuilderDockWindow(UI_TOOL_BOX, rightNode);          // 右边节点

                ImGui::DockBuilderDockWindow(UI_MESSAGE_BOX, bottomNode); // 下面节点同时停靠两个窗口
                ImGui::DockBuilderDockWindow(UI_LOG_BOX, bottomNode);

                ImGui::DockBuilderDockWindow(UI_VIEW_BOX, root); // 观察窗口平铺“客户区”，停靠的节点是 CentralNode

                // 结束停靠设置
                ImGui::DockBuilderFinish(dockspaceID);

                // 设置焦点窗口
                ImGui::SetWindowFocus(UI_VIEW_BOX);
            }

            // 创建停靠空间
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }
        ImGui::End(); // 结束停靠窗口

        // 工程框
        if (ImGui::Begin(UI_PROJECT_BOX))
        {
            ImGui::LabelText("label", "text");
            ImGui::Button("button");
        }
        ImGui::End();

        // 属性框
        if (ImGui::Begin(UI_PROPERTY_BOX))
        {
            ImGui::LabelText("label", "text");
            ImGui::Button("button");
        }
        ImGui::End();

        // 工具框
        if (ImGui::Begin(UI_TOOL_BOX))
        {
            ImGui::LabelText("label", "text");
            ImGui::Button("button");

            if (ImGui::Button(u8"重置布局"))
            {
                // 移除根节点，布局会自动重建
                ImGui::DockBuilderRemoveNode(dockspaceID);
            }
        }
        ImGui::End();

        // 消息框
        if (ImGui::Begin(UI_MESSAGE_BOX))
        {
            ImGui::LabelText("label", "text");
            ImGui::Button("button");
        }
        ImGui::End();

        // 日志框
        if (ImGui::Begin(UI_LOG_BOX))
        {
            ImGui::LabelText("label", "text");
            ImGui::Button("button");
        }
        ImGui::End();

        // 观察窗口，背景设置透明，窗口后面就能进行本地 API 的绘制
        // 压入样式设置
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);            // 无边框
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f)); // 无边界
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);              // 无圆角
        // ImGui::SetNextWindowBgAlpha(0.0f); // 窗口 alpha 为 0，同样可以不显示背景

        if (ImGui::Begin(UI_VIEW_BOX, nullptr, ImGuiWindowFlags_NoBackground))
        { // 无背景窗口
            // 获取窗口坐标
            ImVec2 pos = ImGui::GetWindowPos();
            ImVec2 size = ImGui::GetWindowSize();

            ImGui::Text("position: %0.2f, %0.2f", pos.x, pos.y);
            ImGui::Text("size: %0.2f, %0.2f", size.x, size.y);

            // 记录下视口位置给本地 API 使用
            view_port_pos = pos;
            view_port_size = size;
        }
        ImGui::End();
        ImGui::PopStyleVar(3); // 弹出样式设置

        ImGui::Render();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear[0] * clear[3], clear[1] * clear[3], clear[2] * clear[3], clear[3]);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}