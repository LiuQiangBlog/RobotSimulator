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
#include <unordered_set>
#include <shared_mutex>
#include <deque>
#include <implot.h>
#include <zcm/zcm-cpp.hpp>
#include "all_timed_value.hpp"
#include "data_fields.hpp"
#include "data_channel.hpp"
#include "rolling_buffer.h"
#include <thread>
#include <atomic>
#include <set>

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
    int windowWidth, windowHeight;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &windowWidth, &windowHeight);
    ImGui::SetNextWindowSize(ImVec2((float)windowWidth, (float)windowHeight));
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("##Main Window", nullptr, flags);
    {
        ImGui::SetWindowPos(ImVec2(0, 0));
        ImGui::SetWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y));
        ImGuiStyle &style = ImGui::GetStyle();

        style.Colors[ImGuiCol_Tab] = HexToImVec4("#353333");
        style.Colors[ImGuiCol_TabActive] = HexToImVec4("#353333");
        style.Colors[ImGuiCol_Border] = ImVec4(0,0,0,0);
        style.Colors[ImGuiCol_Separator] = ImVec4(0,0,0,0);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0,0,0,0);

        static ImVector<int> active_tabs;
        static int next_tab_id = 0;
        static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable |
                                                ImGuiTabBarFlags_FittingPolicyResizeDown | ImGuiTabBarFlags_DrawSelectedOverline;
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

// 增加窗口状态管理
struct PlotWindowState
{
    bool is_maximized = false;
    ImVec2 normal_size = ImVec2(600, 400);
    ImVec2 normal_pos = ImVec2(0, 0);
    ImVec2 screen_size;            // 新增屏幕尺寸缓存
    bool need_restore_pos = false; // 新增位置恢复标志
};

class Handler
{
public:
    static constexpr size_t MAX_CACHE_SIZE = 1000;
    using DataBUffer = RollingBuffer<double, MAX_CACHE_SIZE>;
    ~Handler() = default;

    void plotChannelData(const std::string &title, const std::string &channel)
    {
        if (plot_bool[title])
        {
            ImGui::Begin(title.c_str(), &plot_bool[title], ImGuiWindowFlags_AlwaysAutoResize);
            ImPlot::SetNextAxisToFit(ImAxis_Y1);
            if (ImPlot::BeginPlot("##Scrolling", ImVec2(600, 400)))
            {
                ImPlot::SetupAxisFormat(ImAxis_X1, "%.3f");
                // ImPlot::SetupAxisFormat(ImAxis_Y1, "%.5f");
                ImPlot::SetupAxes("Time(s)", "");
                if (channel_plot_data.count(channel) > 0)
                {
                    std::lock_guard<std::shared_mutex> lock(mtx);
                    auto &[ts, vals] = channel_plot_data[channel];
                    if (!ts.empty() && !vals.empty() && ts.size() == vals.size())
                    {
                        double min_time = *std::min_element(ts.begin(), ts.end());
                        double max_time = *std::max_element(ts.begin(), ts.end());
                        ImPlot::SetupAxisLimits(ImAxis_X1, min_time, max_time, ImGuiCond_Always);
                        auto [y_min, y_max] = std::minmax_element(vals.begin(), vals.end());
                        ImPlot::SetupAxisLimits(ImAxis_Y1, *y_min, *y_max, ImGuiCond_Always);

                        ImVec4 color = ImPlot::GetColormapColor(0);
                        ImPlot::SetNextLineStyle(color, 1.0f);
                        ImPlot::PlotLine(channel.c_str(), ts.data(), vals.data(), (int)ts.size());
                        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 2.0f, color, 1.5f, color);
                        ImPlot::PlotScatter(channel.c_str(), ts.data(), vals.data(), (int)ts.size());
                    }
                }
                ImPlot::EndPlot();
            }
            ImGui::End();
        }
    }

    // plot all channels
    void plotChannelData(const std::string &title, const std::vector<std::string> &channels)
    {
        CLOG_INFO << channels.size();
        if (plot_bool[title])
        {
            // 确保有对应的窗口状态
            if (plot_window_states.find(title) == plot_window_states.end())
            {
                plot_window_states[title] = PlotWindowState{};
            }
            auto &state = plot_window_states[title];

            // 窗口标志
            ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
            //            ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;
            if (state.is_maximized)
            {
                windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                               ImGuiWindowFlags_NoTitleBar;
            }

            // 开始窗口
            ImVec2 screenSize = ImGui::GetIO().DisplaySize;  // 获取屏幕分辨率
            ImVec2 defaultSize(screenSize.x * 0.8f, screenSize.y * 0.6f);
            ImGui::SetNextWindowSize(defaultSize, ImGuiCond_FirstUseEver);
            // ImVec2 defaultSize(600, 400);
            // ImGui::SetNextWindowSize(defaultSize);
            ImGui::Begin(title.c_str(), &plot_bool[title], windowFlags);

            // 保存正常状态的位置和大小
            if (!state.is_maximized && ImGui::IsWindowFocused())
            {
                state.normal_size = ImGui::GetWindowSize();
                state.normal_pos = ImGui::GetWindowPos();
            }

            // 添加最大化/恢复按钮
            if (ImGui::Button(state.is_maximized ? "[-]" : "[+]"))
            {
                state.is_maximized = !state.is_maximized;
                if (state.is_maximized)
                {
                    // 保存当前状态并最大化
                    ImGuiIO &io = ImGui::GetIO();
                    ImGui::SetWindowPos(title.c_str(), ImVec2(0, 0));
                    ImGui::SetWindowSize(title.c_str(), ImVec2(io.DisplaySize.x, io.DisplaySize.y));
                }
                else
                {
                    // 恢复之前的状态
                    ImGui::SetWindowPos(title.c_str(), state.normal_pos);
                    ImGui::SetWindowSize(title.c_str(), state.normal_size);
                }
            }

            ImPlot::SetNextAxisToFit(ImAxis_Y1);

            // 初始化全局范围
            auto global_x_min = DBL_MAX;
            auto global_x_max = -DBL_MAX;
            auto global_y_min = DBL_MAX;
            auto global_y_max = -DBL_MAX;

            // 计算所有通道的全局范围
            {
                std::shared_lock<std::shared_mutex> lock(mtx);
                for (const auto &channel : channels)
                {
                    if (channel_plot_data.count(channel) > 0)
                    {
                        auto &[ts, vals] = channel_plot_data[channel];
                        if (!ts.empty() && !vals.empty() && ts.size() == vals.size())
                        {
                            double min_time = *std::min_element(ts.begin(), ts.end());
                            double max_time = *std::max_element(ts.begin(), ts.end());
                            auto [y_min, y_max] = std::minmax_element(vals.begin(), vals.end());

                            global_x_min = std::min(global_x_min, min_time);
                            global_x_max = std::max(global_x_max, max_time);
                            global_y_min = std::min(global_y_min, *y_min);
                            global_y_max = std::max(global_y_max, *y_max);
                        }
                    }
                }
            }

            // 绘制图表 - 使用窗口剩余空间
            ImVec2 contentRegion = ImGui::GetContentRegionAvail();
            if (ImPlot::BeginPlot("##ChannelPlot", contentRegion))
            {
                ImPlot::SetupAxisFormat(ImAxis_X1, "%.3f");
                ImPlot::SetupAxes("Time(s)", "Value");
                ImPlot::GetPlotDrawList()->Flags |= ImDrawListFlags_AntiAliasedLines;
                // 设置全局范围
                if (global_x_min <= global_x_max && global_y_min <= global_y_max)
                {
                    ImPlot::SetupAxisLimits(ImAxis_X1, global_x_min, global_x_max, ImGuiCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, global_y_min, global_y_max, ImGuiCond_Always);
                }

                // 绘制所有通道
                {
                    std::shared_lock<std::shared_mutex> lock(mtx);
                    for (size_t i = 0; i < channels.size(); i++)
                    {
                        const auto &channel = channels[i];
                        if (channel_plot_data.count(channel) > 0)
                        {
                            auto &[ts, vals] = channel_plot_data[channel];
                            if (!ts.empty() && !vals.empty() && ts.size() == vals.size())
                            {
                                ImVec4 color = ImPlot::GetColormapColor(int(i));
                                ImPlot::SetNextLineStyle(color, 2.0f);
                                ImPlot::PlotLine(channel.c_str(), ts.data(), vals.data(), (int)ts.size());
                                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 2.0f, color, -1.0f, color);
                                ImPlot::PlotScatter(channel.c_str(), ts.data(), vals.data(), (int)ts.size());
                            }
                        }
                    }
                }
                ImPlot::EndPlot();
            }
            ImGui::End();
        }
    }

    void handle(const zcm::ReceiveBuffer *buffer, const std::string &channel, const all_timed_value *msg)
    {
        static bool initialized{false};
        for (const auto &item : msg->channels)
        {
            channel_data[item.name].first.push_back(item.timestamp);
            channel_data[item.name].second.push_back(item.value);
            if (channel_data[item.name].first.size() > MAX_CACHE_SIZE)
            {
                channel_data[item.name].first.pop_front();
                channel_data[item.name].second.pop_front();
            }
            auto timestamps = std::vector<double>(channel_data[item.name].first.begin(), channel_data[item.name].first.end());
            auto values = std::vector<double>(channel_data[item.name].second.begin(), channel_data[item.name].second.end());
            {
                std::lock_guard<std::shared_mutex> lck(mtx);
                channel_plot_data[item.name].first = timestamps;
                channel_plot_data[item.name].second = values;
            }
            if (!initialized || msg->channel_updated)
            {
                all_channels.insert(item.name);
            }
        }
        if (!initialized)
        {
            initialized = true;
        }
    }

    std::unordered_map<double, std::unordered_map<std::string, double>> frame_buffer;
    std::unordered_map<std::string, std::pair<std::deque<double>, std::deque<double>>> channel_data;
    //    std::unordered_map<std::string, std::pair<DataBUffer, DataBUffer>> channel_data;
    std::unordered_map<std::string, std::pair<std::vector<double>, std::vector<double>>> channel_plot_data;
    std::unordered_set<std::string> all_channels;
    std::unordered_map<int, std::deque<std::string>> tab_selected_channels;
    std::unordered_map<std::string, bool> plot_bool;
    std::unordered_map<std::string, PlotWindowState> plot_window_states;
    std::shared_mutex mtx;
};

void createTabBarWithImPlot(Handler &h)
{
    int windowWidth, windowHeight;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &windowWidth, &windowHeight);
    ImGui::SetNextWindowSize(ImVec2((float)windowWidth, (float)windowHeight));
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    ImGuiStyle &style = ImGui::GetStyle();
    ImVec2 original_window_padding = style.WindowPadding;
    style.WindowPadding.x = 4.0f; // keep TabItem distance to left window edge is 4
    style.WindowPadding.y = 4.0f; // keep TabItem distance to top window edge is 4
    ImGui::Begin("##Main Window", nullptr, flags);
    {
        ImGui::SetWindowPos(ImVec2(0, 0));
        ImGui::SetWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y));
//        style.Colors[ImGuiCol_Tab] = HexToImVec4("#353333");
//        style.Colors[ImGuiCol_TabActive] = HexToImVec4("#353333");
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        static ImVector<int> active_tabs;
        static int next_tab_id = 0;
        static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable |
                                                ImGuiTabBarFlags_FittingPolicyResizeDown;
        static char new_tab_title[64] = "";
        static bool open_new_tab_dialog{false};
        static std::unordered_map<int, std::string> tab_titles;
        static std::string error_message;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 12));
        ImGui::PushStyleVar(ImGuiStyleVar_TabBorderSize, 1.0f);     // 增大边框厚度
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_TabBarBorderSize, 0.0f);  // 隐藏底部边框
        if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
        {

            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
            {
                open_new_tab_dialog = true;
                strcpy(new_tab_title, "Untitled");
                error_message.clear();
            }
            ImGui::PopStyleVar(4);
            for (int n = 0; n < active_tabs.Size;)
            {
                int tab_id = active_tabs[n];
                bool tab_open = true;
                const char* tab_title = tab_titles[tab_id].c_str();

                if (!h.tab_selected_channels.count(tab_id))
                {
                    h.tab_selected_channels[tab_id] = {};
                }
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 12));
                ImGui::PushStyleVar(ImGuiStyleVar_TabBorderSize, 1.0f);     // 增大边框厚度
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
                ImGui::PushStyleVar(ImGuiStyleVar_TabBarBorderSize, 0.0f);  // 隐藏底部边框
                if (ImGui::BeginTabItem(tab_title, &tab_open))
                {
                    ImGui::PopStyleVar(4);
                    if (tab_open)
                    {
                        bool menu_open = false;
                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                        {
                            ImGui::OpenPopup(("ChannelMenu_" + std::to_string(tab_id)).c_str());
                        }
                        if (ImGui::BeginPopup(("ChannelMenu_" + std::to_string(tab_id)).c_str()))
                        {
                            menu_open = true;
                            auto &selected_channels = h.tab_selected_channels[tab_id];
                            static char filter_text[128] = "";
                            ImGui::Text("Filter Channels:");
                            ImGui::PushItemWidth(300);
                            ImGui::InputText("##FilterCondition", filter_text, IM_ARRAYSIZE(filter_text),
                                             ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoHorizontalScroll);
                            trimString(new_tab_title);

                            ImGui::Separator();
                            for (const std::string &channel : h.all_channels)
                            {
                                if (strlen(filter_text) != 0 && channel.find(filter_text) == std::string::npos)
                                {
                                    continue;
                                }
                                bool is_selected = std::find(selected_channels.begin(), selected_channels.end(), channel) != selected_channels.end();
                                if (ImGui::Checkbox(channel.c_str(), &is_selected))
                                {
                                    if (is_selected)
                                    {
                                        selected_channels.push_back(channel);
                                    }
                                    else
                                    {
                                        auto it = std::find(selected_channels.begin(), selected_channels.end(), channel);
                                        if (it != selected_channels.end())
                                        {
                                            selected_channels.erase(it);
                                        }
                                    }
                                }
                            }
                            ImGui::Separator();
                            if (ImGui::Button("Apply Selection"))
                            {
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Clear Selection"))
                            {
                                selected_channels.clear();
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::EndPopup();
                        }
                        if (!menu_open)
                        {
                            auto &channels = h.tab_selected_channels[tab_id];
                            for (auto &item : channels)
                            {
                                CLOG_INFO << item;
                            }
                            h.plotChannelData(tab_title, std::vector<std::string>(channels.begin(), channels.end()));
                        }
                    }
                    ImGui::EndTabItem();
                }
                else
                {
                    ImGui::PopStyleVar(4);
                }
                if (!tab_open)
                {
                    tab_titles.erase(tab_id);
                    active_tabs.erase(active_tabs.begin() + n);
                    h.tab_selected_channels.erase(tab_id);
                }
                else
                {
                    n++;
                }
            }
            ImGui::EndTabBar();
        }

        if (open_new_tab_dialog)
        {
            ImGui::OpenPopup("Untitled");
            open_new_tab_dialog = false;
        }

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                        ImGuiWindowFlags_NoNav;
        static bool first_open = true;
        if (ImGui::BeginPopupModal("Untitled", nullptr, window_flags))
        {
            ImGui::SetWindowFocus();
            ImGui::Text("Enter tab title:");
            ImGui::PushItemWidth(300);
            if (ImGui::InputText("##TitleInput", new_tab_title, IM_ARRAYSIZE(new_tab_title)))
            {
                error_message.clear();
            }
            if (first_open)
            {
                ImGui::SetKeyboardFocusHere(-1);
                first_open = false;
            }
            ImGui::PopItemWidth();

            if (!error_message.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f)); // 红色文本
                ImGui::TextWrapped("%s", error_message.c_str());
                ImGui::PopStyleColor();
            }
            ImGui::Separator();
            bool can_create = false;
            trimString(new_tab_title);
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
            first_open = true;
        }
    }
    style.WindowPadding = original_window_padding;
    ImGui::End();
}

class DataViewer
{
private:
    std::string winTitle;
    GLFWwindow *window{nullptr};
    bool button_left{false};
    bool button_middle{false};
    bool button_right{false};
    double last_x{0};
    double last_y{0};
    std::unique_ptr<zcm::ZCM> zcm;
    Handler h;
    std::thread th_zcm;
    ImVec4 clear{0.45f, 0.55f, 0.60f, 1.00f};
    std::set<std::string> availableChannels;
    std::atomic_bool exit{false};

public:
    explicit DataViewer(const std::string &name)
    {
        winTitle = name;
        zcm::RegisterAllPlugins();
    }
    DataViewer()
    {
        winTitle = "Plotter";
        zcm::RegisterAllPlugins();
    }

    ~DataViewer()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
        zcm->stop();
        exit = true;
        if (th_zcm.joinable())
        {
            th_zcm.join();
        }
    }

    bool init()
    {
        if (!glfwInit())
        {
            CLOG_ERROR << "Init GLFW Failed.";
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // 3.0+ only

        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // hide window title
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);   // hide window first
        window = glfwCreateWindow(1920, 1080, "Viewer", nullptr, nullptr);
        if (!window)
        {
            glfwTerminate();
            CLOG_ERROR << "Failed to create GLFW Window.";
            return false;
        }
        {
            // move glfw window to center position
            GLFWmonitor *monitor = glfwGetPrimaryMonitor();
            if (!monitor)
            {
                return false;
            }
            int window_width, window_height;
            glfwGetWindowSize(window, &window_width, &window_height);
            const GLFWvidmode *mode = glfwGetVideoMode(monitor);
            int screen_width = mode->width;
            int screen_height = mode->height;
            int x_pos = (screen_width - window_width) / 2;
            int y_pos = (screen_height - window_height) / 2;
            glfwSetWindowPos(window, x_pos, y_pos);
        }
        glfwShowWindow(window);
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
        glfwSetWindowTitle(window, winTitle.c_str());
        glfwWindowHint(GLFW_SAMPLES, 4);
        glEnable(GL_MULTISAMPLE);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        io.FontGlobalScale = 1.2f;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark(); // ImGui::StyleColorsLight();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

        glfwSetWindowUserPointer(window, this);
        glfwSetMouseButtonCallback(window, mouseClickCallback);
        glfwSetCursorPosCallback(window, mouseMoveCallback);
        glfwSetScrollCallback(window, mouseScrollCallback);
        glfwSetErrorCallback(errorCallback);

        zcm = std::make_unique<zcm::ZCM>("ipcshm://");
        if (!zcm->good())
        {
            zcm.reset();
            return false;
        }
        zcm->subscribe("all_channel_data", &Handler::handle, &h);
        th_zcm = std::thread(
            [&]()
            {
                zcm->run();
            });

        h.all_channels.insert("a");
        h.all_channels.insert("b");
        h.all_channels.insert("c");
        h.all_channels.insert("d");
        return true;
    }

    void render()
    {
        glfwPollEvents(); // process events
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            return;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
        }

        createTabBarWithImPlot(h);

        // ImGui render
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear.x * clear.w, clear.y * clear.w, clear.z * clear.w, clear.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    bool shouldClose() const
    {
        return glfwWindowShouldClose(window);
    }

protected:
    static void mouseClickCallback(GLFWwindow *win, int button, int action, int mods)
    {
        auto *self = static_cast<DataViewer *>(glfwGetWindowUserPointer(win));
        if (self)
        {
            self->mouseClick(win, button, action, mods);
        }
    }

    static void mouseMoveCallback(GLFWwindow *win, double xPos, double yPos)
    {
        auto *self = static_cast<DataViewer *>(glfwGetWindowUserPointer(win));
        if (self)
        {
            self->mouseMove(win, xPos, yPos);
        }
    }

    static void mouseScrollCallback(GLFWwindow *win, double xOffset, double yOffset)
    {
        auto *self = static_cast<DataViewer *>(glfwGetWindowUserPointer(win));
        if (self)
        {
            DataViewer::mouseScroll(win, xOffset, yOffset);
        }
    }

    static void errorCallback(int error, const char *description)
    {
        fprintf(stderr, "GLFW Error %d: %s\n", error, description);
    }

    void mouseClick(GLFWwindow *win, int button, int action, int mods)
    {
        ImGui_ImplGlfw_MouseButtonCallback(win, button, action, mods);
        if (ImGui::GetIO().WantCaptureMouse)
        {
            return;
        }
        button_left = (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
        button_middle = (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
        button_right = (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
        glfwGetCursorPos(win, &last_x, &last_y);
    }

    void mouseMove(GLFWwindow *win, double xPos, double yPos)
    {
        ImGui_ImplGlfw_CursorPosCallback(win, xPos, yPos);
        if (ImGui::GetIO().WantCaptureMouse)
        {
            return;
        }
        if (!button_left && !button_middle && !button_right)
        {
            return;
        }
        last_x = xPos;
        last_y = yPos;
    }

    static void mouseScroll(GLFWwindow *win, double xOffset, double yOffset)
    {
        ImGui_ImplGlfw_ScrollCallback(win, xOffset, yOffset);
        if (ImGui::GetIO().WantCaptureMouse)
        {
            return;
        }
    }
};

int main()
{
    DataViewer viewer;
    if (!viewer.init())
    {
        return -1;
    }
    while (!viewer.shouldClose())
    {
        viewer.render();
    }
    return 0;
}