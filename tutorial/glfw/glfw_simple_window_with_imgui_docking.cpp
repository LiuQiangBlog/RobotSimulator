//
// Created by liuqiang on 25-5-21.
//
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <Logging.h>
#include <unordered_map>
#include <algorithm>

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

void createTree()
{
    if (ImGui::TreeNode("Backend Flags"))
    {
        ImGui::BeginDisabled();
        ImGui::Checkbox("io.ConfigDebugBeginReturnValueOnce", &ImGui::GetIO().ConfigDebugBeginReturnValueOnce);
        ImGui::EndDisabled();
        ImGui::TreePop();
        ImGui::Spacing();
    }
}

void createTabBar()
{
    if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_AutoSelectNewTabs))
    {
        if (ImGui::BeginTabItem("Description"))
        {
            ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor "
                               "incididunt ut labore et dolore magna aliqua. ");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Details"))
        {
            ImGui::Text("ID: 0123456789");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void createTable()
{
    // Demonstrate the various window flags. Typically you would just use the default!
    static bool no_titlebar = false;
    static bool no_scrollbar = false;
    static bool no_menu = false;
    static bool no_move = false;
    static bool no_resize = false;
    static bool no_collapse = false;
    static bool no_close = false;
    static bool no_nav = false;
    static bool no_background = false;
    static bool no_bring_to_front = false;
    static bool no_docking = false;
    static bool unsaved_document = false;
    if (ImGui::CollapsingHeader("Window options", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::BeginTable("split", 3))
        {
            ImGui::TableNextColumn();
            ImGui::Checkbox("No titlebar", &no_titlebar);
            ImGui::TableNextColumn();
            ImGui::Checkbox("No scrollbar", &no_scrollbar);
            ImGui::TableNextColumn();
            ImGui::Checkbox("No menu", &no_menu);
            ImGui::TableNextColumn();
            ImGui::Checkbox("No move", &no_move);
            ImGui::TableNextColumn();
            ImGui::Checkbox("No resize", &no_resize);
            ImGui::TableNextColumn();
            ImGui::Checkbox("No collapse", &no_collapse);
            ImGui::TableNextColumn();
            ImGui::Checkbox("No close", &no_close);
            ImGui::TableNextColumn();
            ImGui::Checkbox("No nav", &no_nav);
            ImGui::TableNextColumn();
            ImGui::Checkbox("No background", &no_background);
            ImGui::TableNextColumn();
            ImGui::Checkbox("No bring to front", &no_bring_to_front);
            ImGui::TableNextColumn();
            ImGui::Checkbox("No docking", &no_docking);
            ImGui::TableNextColumn();
            ImGui::Checkbox("Unsaved document", &unsaved_document);
            ImGui::EndTable();
        }
    }
}

void setMenubarColor()
{
    ImGuiStyle &style = ImGui::GetStyle();
    // 设置主菜单栏背景色（深灰色）
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(32 / 255.f, 30 / 255.f, 30 / 255.f, 1.00f);
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

void setWindowColor()
{
    // 在初始化 ImGui 后设置样式
    ImGuiStyle &style = ImGui::GetStyle();

    // 设置窗口背景色（RGBA 格式，范围 0.0-1.0）
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);   // 深色背景
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f); // 子窗口背景
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.2f, 0.2f, 0.2f, 0.95f);   // 弹出窗口背景
}

// 函数：将十六进制颜色字符串转换为ImVec4（支持#RRGGBB和#AARRGGBB）
ImVec4 HexToImVec4(const std::string &hexColor, float alpha = 1.0f)
{
    std::string colorStr = hexColor;
    // 移除#符号
    if (!colorStr.empty() && colorStr[0] == '#')
    {
        colorStr.erase(0, 1);
    }

    unsigned int hexValue = 0;
    std::istringstream iss(colorStr);
    iss >> std::hex >> hexValue;

    int numChars = (int)colorStr.length();
    if (numChars == 6)
    { // #RRGGBB（无透明度）
        unsigned char r = (hexValue >> 16) & 0xFF;
        unsigned char g = (hexValue >> 8) & 0xFF;
        unsigned char b = hexValue & 0xFF;
        return ImVec4(r / 255.f, g / 255.f, b / 255.f, alpha);
    }
    else if (numChars == 8)
    { // #AARRGGBB（带透明度）
        unsigned char a = (hexValue >> 24) & 0xFF;
        unsigned char r = (hexValue >> 16) & 0xFF;
        unsigned char g = (hexValue >> 8) & 0xFF;
        unsigned char b = hexValue & 0xFF;
        return ImVec4(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
    }
    else
    {
        // 默认返回黑色（处理错误情况）
        return ImVec4(0.0f, 0.0f, 0.0f, alpha);
    }
}

void createFullScreenWindow()
{
    static bool use_work_area = true;
    static bool p_open = true;
    static ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
    ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);
    if (!p_open)
    {
        return;
    }
    if (ImGui::Begin("Example: Fullscreen window", &p_open, flags))
    {
        ImGui::Checkbox("Use work area instead of main area", &use_work_area);
        ImGui::SameLine();
        ImGui::CheckboxFlags("ImGuiWindowFlags_NoBackground", &flags, ImGuiWindowFlags_NoBackground);
        ImGui::CheckboxFlags("ImGuiWindowFlags_NoDecoration", &flags, ImGuiWindowFlags_NoDecoration);
        ImGui::Indent();
        ImGui::CheckboxFlags("ImGuiWindowFlags_NoTitleBar", &flags, ImGuiWindowFlags_NoTitleBar);
        ImGui::CheckboxFlags("ImGuiWindowFlags_NoCollapse", &flags, ImGuiWindowFlags_NoCollapse);
        ImGui::CheckboxFlags("ImGuiWindowFlags_NoScrollbar", &flags, ImGuiWindowFlags_NoScrollbar);
        ImGui::Unindent();
    }
    ImGui::End();
}

void createMenuBar()
{
    float menuBarHeight = 0.0f;
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
            }
            if (ImGui::MenuItem("Open", "Ctrl+O"))
            {
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
            }
            if (ImGui::MenuItem("Save As.."))
            {
            }
            if (ImGui::MenuItem("Exit", "Esc"))
            {
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
        menuBarHeight = ImGui::GetWindowHeight();
        ImGui::EndMainMenuBar();
        CLOG_INFO << "menuBarHeight: " << menuBarHeight;
    }
}

void createMenuBarWithMainWindow()
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
    float menuBarHeight = 0.0f;
    bool hasMenuBar = false;
    ImGuiStyle &style = ImGui::GetStyle();
    style.Colors[ImGuiCol_MenuBarBg] = HexToImVec4("#353333");
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
        menuBarHeight = ImGui::GetWindowHeight();
        ImGui::EndMainMenuBar();
        hasMenuBar = true;
    }

    int windowWidth, windowHeight;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &windowWidth, &windowHeight);
    ImGui::SetNextWindowSize(ImVec2((float)windowWidth, (float)windowHeight - (hasMenuBar ? menuBarHeight : 0)));
    ImGui::SetNextWindowPos(ImVec2(0, hasMenuBar ? menuBarHeight : 0));
    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;
    ImGui::Begin("##Main Window", nullptr, flags);
    {
        ImGui::SetWindowPos(ImVec2(0, hasMenuBar ? menuBarHeight : 0));
        ImGui::SetWindowSize(
            ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - (hasMenuBar ? menuBarHeight : 0)));
        // 在这里添加其他 imgui 控件
        ImGui::Text("Hello, ImGui!");
    }
    ImGui::End();
}

// 检查标题是否已存在
bool isTitleExists(const char *title, const std::unordered_map<int, std::string> &tab_titles)
{
    return std::any_of(tab_titles.begin(), tab_titles.end(),
                       [title](const auto &pair)
                       {
                           return pair.second == title;
                       });
}

// 清理字符串（去除首尾空格）
void trimString(char *str)
{
    if (!str)
    {
        return;
    }

    // 去除尾部空格
    size_t len = strlen(str);
    while (len > 0 && isspace(str[len - 1]))
    {
        str[len - 1] = '\0';
        len--;
    }

    // 去除头部空格
    char *ptr = str;
    while (*ptr && isspace(*ptr))
    {
        ptr++;
    }

    if (ptr != str)
    {
        memmove(str, ptr, strlen(ptr) + 1);
    }
}

void createTabBarWithMainWindow()
{
    float tabBarHeight = 0.0f;
    bool hasTabBar = false;
    int windowWidth, windowHeight;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &windowWidth, &windowHeight);
    ImGui::SetNextWindowSize(ImVec2((float)windowWidth, (float)windowHeight - (hasTabBar ? tabBarHeight : 0)));
    ImGui::SetNextWindowPos(ImVec2(0, hasTabBar ? tabBarHeight : 0));
    static ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("##Main Window", nullptr, flags);
    {
        ImGui::SetWindowPos(ImVec2(0, hasTabBar ? tabBarHeight : 0));
        ImGui::SetWindowSize(
            ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - (hasTabBar ? tabBarHeight : 0)));
        ImGuiStyle &style = ImGui::GetStyle();
        style.Colors[ImGuiCol_Tab] = HexToImVec4("#353333");
        style.Colors[ImGuiCol_TabActive] = HexToImVec4("#353333");
        //        if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_DrawSelectedOverline))
        //        {
        //            if (ImGui::BeginTabItem("Description"))
        //            {
        //                ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod
        //                tempor incididunt ut labore et dolore magna aliqua. "); ImGui::EndTabItem();
        //            }
        //            if (ImGui::BeginTabItem("Details"))
        //            {
        //                ImGui::Text("ID: 0123456789");
        //                ImGui::EndTabItem();
        //            }
        //
        //            tabBarHeight = ImGui::GetWindowHeight();
        //            ImGui::EndTabBar();
        //            hasTabBar = true;
        //        }

        //        static ImVector<int> active_tabs;
        //        static int next_tab_id = 0;
        //        static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_AutoSelectNewTabs |
        //        ImGuiTabBarFlags_Reorderable |
        //                                                ImGuiTabBarFlags_FittingPolicyResizeDown;
        //        if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
        //        {
        //            // Demo Trailing Tabs: click the "+" button to add a new tab.
        //            // (In your app you may want to use a font icon instead of the "+")
        //            // We submit it before the regular tabs, but thanks to the ImGuiTabItemFlags_Trailing flag it will
        //            always appear at the end. if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing |
        //            ImGuiTabItemFlags_NoTooltip))
        //            {
        //                active_tabs.push_back(next_tab_id++); // Add new tab
        //            }
        //            // Submit our regular tabs
        //            for (int n = 0; n < active_tabs.Size; )
        //            {
        //                bool open = true;
        //                char name[16];
        //                snprintf(name, IM_ARRAYSIZE(name), "%04d", active_tabs[n]);
        //                if (ImGui::BeginTabItem(name, &open, ImGuiTabItemFlags_None))
        //                {
        //                    ImGui::Text("This is the %s tab!", name);
        //                    ImGui::EndTabItem();
        //                }
        //                if (!open)
        //                {
        //                    active_tabs.erase(active_tabs.Data + n);
        //                }
        //                else
        //                {
        //                    n++;
        //                }
        //            }
        //            ImGui::EndTabBar();
        //        }

        static ImVector<int> active_tabs;
        static int next_tab_id = 0;
        static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable |
                                                ImGuiTabBarFlags_FittingPolicyResizeDown;
        static char new_tab_title[64] = "New Tab";              // 存储新标签页标题
        static bool open_new_tab_dialog = false;                // 控制对话框显示
        static std::unordered_map<int, std::string> tab_titles; // 存储标签页ID到标题的映射
        static std::string error_message; // 存储错误信息
        if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
        {
            // 点击加号按钮
            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
            {
                open_new_tab_dialog = true; // 打开对话框
                strcpy(new_tab_title, "New Tab"); // 重置默认标题
                error_message.clear(); // 清空错误信息
            }

            // 渲染所有标签页
            for (int n = 0; n < active_tabs.Size;)
            {
                int tab_id = active_tabs[n];
                bool tab_open = true;

                // 获取标签页标题（如果不存在则使用默认ID）
                const char* tab_title = tab_titles[tab_id].c_str();
                // 渲染标签页内容
                if (ImGui::BeginTabItem(tab_title, &tab_open)) {
                    ImGui::Text("Content of '%s'", tab_title);
                    ImGui::EndTabItem();
                }

                // 处理关闭标签页
                if (!tab_open) {
                    tab_titles.erase(tab_id);
                    active_tabs.erase(active_tabs.begin() + n);
                } else {
                    n++;
                }
            }
            ImGui::EndTabBar();
        }

        // 处理新建标签页对话框
        if (open_new_tab_dialog)
        {
            ImGui::OpenPopup("New Tab");
            open_new_tab_dialog = false;
        }

        // 显示对话框
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                        ImGuiWindowFlags_NoNav;
        static bool first_open = true;
        if (ImGui::BeginPopupModal("New Tab", nullptr, window_flags))
        {
            ImGui::SetWindowFocus(); // 确保对话框获得焦点
            // 输入框
            ImGui::Text("Enter tab title:");
            ImGui::PushItemWidth(300);
            if (ImGui::InputText("##TitleInput", new_tab_title, IM_ARRAYSIZE(new_tab_title)))
            {
                error_message.clear(); // 输入变化时清除错误信息
            }
            // 仅在首次打开时设置焦点
            if (first_open)
            {
                ImGui::SetKeyboardFocusHere(-1);
                first_open = false;
            }
            ImGui::PopItemWidth();

            // 显示错误信息（如果有）
            if (!error_message.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f)); // 红色文本
                ImGui::TextWrapped("%s", error_message.c_str());
                ImGui::PopStyleColor();
            }

            // 按钮
            ImGui::Separator();

            // 禁用创建按钮的条件：标题为空或已存在
            bool can_create = false;
            trimString(new_tab_title); // 先清理字符串

            if (strlen(new_tab_title) == 0)
            {
                error_message = "Title cannot be empty!";
            }
            else if (isTitleExists(new_tab_title, tab_titles))
            {
                error_message = "Title already exists!";
            }
            else
            {
                can_create = true;
                error_message.clear();
            }

            // 根据条件启用/禁用按钮
            ImGui::BeginDisabled(!can_create);
            if (ImGui::Button("Create", ImVec2(120, 0)))
            {
                int new_id = next_tab_id++;
                active_tabs.push_back(new_id);
                tab_titles[new_id] = std::string(new_tab_title);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        else
        {
            // 对话框关闭时重置标志
            first_open = true;
        }
        ImGui::Text("Hello, ImGui!");
    }
    ImGui::End();
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
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // hide window first
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

        //        createFullScreenWindow();
        //        createMenuBar();
        //        createMenuBarWithMainWindow();
        createTabBarWithMainWindow();

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