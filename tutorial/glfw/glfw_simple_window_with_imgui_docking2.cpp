//
// Created by liuqiang on 25-5-21.
//
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <Logging.h>
#include <unordered_map>
#include <algorithm>
#include <unordered_set>
#include <shared_mutex>
#include <deque>
#include <implot.h>
#include <implot_internal.h>
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

ImVec4 HexToImVec4(const std::string &hexColor, float alpha = 1.0f)
{
    std::string colorStr = hexColor;
    if (!colorStr.empty() && colorStr[0] == '#')
    {
        colorStr.erase(0, 1);
    }

    unsigned int hexValue = 0;
    std::istringstream iss(colorStr);
    iss >> std::hex >> hexValue;

    int numChars = (int)colorStr.length();
    if (numChars == 6)
    {
        unsigned char r = (hexValue >> 16) & 0xFF;
        unsigned char g = (hexValue >> 8) & 0xFF;
        unsigned char b = hexValue & 0xFF;
        return {float(r) / 255.f, float(g) / 255.f, float(b) / 255.f, alpha};
    }
    else if (numChars == 8)
    {
        unsigned char a = (hexValue >> 24) & 0xFF;
        unsigned char r = (hexValue >> 16) & 0xFF;
        unsigned char g = (hexValue >> 8) & 0xFF;
        unsigned char b = hexValue & 0xFF;
        return {float(r) / 255.f, float(g) / 255.f, float(b) / 255.f, float(a) / 255.f};
    }
    else
    {
        return {0.0f, 0.0f, 0.0f, alpha};
    }
}

bool isTitleExists(const char *title, const std::unordered_map<int, std::string> &tab_titles)
{
    return std::any_of(tab_titles.begin(), tab_titles.end(),
                       [title](const auto &pair)
                       {
                           return pair.second == title;
                       });
}

void trimString(char *str)
{
    if (!str)
    {
        return;
    }

    size_t len = strlen(str);
    while (len > 0 && isspace(str[len - 1]))
    {
        str[len - 1] = '\0';
        len--;
    }

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

struct PlotWindowState
{
    bool is_maximized = false;
    ImVec2 normal_size = ImVec2(600, 400);
    ImVec2 normal_pos = ImVec2(0, 0);
    ImVec2 screen_size;
    bool need_restore_pos = false;
};

// utility structure for realtime plot
struct ScrollingBuffer
{
    int MaxSize;
    int Offset;
    ImVector<ImVec2> Data;
    explicit ScrollingBuffer(int max_size = 10000)
    {
        MaxSize = max_size;
        Offset = 0;
        Data.reserve(MaxSize);
    }
    void AddPoint(float x, float y)
    {
        if (Data.size() < MaxSize)
            Data.push_back(ImVec2(x, y));
        else
        {
            Data[Offset] = ImVec2(x, y);
            Offset = (Offset + 1) % MaxSize;
        }
    }
    void Erase()
    {
        if (!Data.empty())
        {
            Data.shrink(0);
            Offset = 0;
        }
    }
};

class Handler
{
public:
    static constexpr size_t MAX_CACHE_SIZE = 10000;
    using DataBUffer = RollingBuffer<double, MAX_CACHE_SIZE>;
    ~Handler() = default;

    // plot all channels
    void plotChannelData(const std::string &title, const std::vector<std::string> &channels)
    {
        ImVec2 tabContentRegion = ImGui::GetContentRegionAvail();
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
                                       ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoScrollbar |
                                       ImGuiWindowFlags_NoSavedSettings;
        ImGui::SetNextWindowSize(tabContentRegion, ImGuiCond_Always);
        if (ImGui::BeginChild((title + "##PlotWindow").c_str(), tabContentRegion, ImGuiChildFlags_None, windowFlags))
        {
            ImPlot::SetNextAxisToFit(ImAxis_Y1);

            auto global_x_min = DBL_MAX;
            auto global_x_max = -DBL_MAX;
            auto global_y_min = DBL_MAX;
            auto global_y_max = -DBL_MAX;

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

            ImVec2 contentRegion = ImGui::GetContentRegionAvail();
            if (ImPlot::BeginPlot(("##" + title + "##ChannelPlot").c_str(), contentRegion))
            {
                ImPlot::SetupAxisFormat(ImAxis_X1, "%.3f");
                ImPlot::SetupAxisFormat(ImAxis_Y1, "%.3f");
                ImPlot::SetupAxes("Time(s)", "Value");
                ImPlot::GetPlotDrawList()->Flags |= ImDrawListFlags_AntiAliasedLines;
                if (global_x_min <= global_x_max && global_y_min <= global_y_max)
                {
                    ImPlot::SetupAxisLimits(ImAxis_X1, global_x_min, global_x_max, ImGuiCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, global_y_min, global_y_max, ImGuiCond_Always);
                }
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
                                //ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, -1.0f, color, 4.0f, color);
                                ImPlot::PlotScatter(channel.c_str(), ts.data(), vals.data(), (int)ts.size());
                            }
                        }
                    }
                }
                ImPlot::EndPlot();
            }
        }
        ImGui::EndChild();
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
    //std::unordered_map<std::string, std::pair<DataBUffer, DataBUffer>> channel_data;
    std::unordered_map<std::string, std::pair<std::vector<double>, std::vector<double>>> channel_plot_data;
    std::unordered_set<std::string> all_channels;
    std::unordered_map<int, std::deque<std::string>> tab_selected_channels;
    std::unordered_map<std::string, PlotWindowState> plot_window_states;
    std::shared_mutex mtx;
};

enum class AppState
{
    MainMenu,
    ZcmView,
    McapView
};

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
    AppState currentState{AppState::MainMenu};
    std::string currentMcapPath;
    std::vector<std::string> mcapChannels;
    bool openViewerOfZcmData{true};
    bool openViewerOfMcapData{true};

public:
    explicit DataViewer(const std::string &name = "DataViewer")
    {
        winTitle = name;
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

        //glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // hide window title
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

        switch (currentState)
        {
        case AppState::MainMenu:
            createWelcomeWindow();
            break;
        case AppState::ZcmView:
            openViewerOfZcmData = true;
            createTabBarWithImPlotForZcmChannel();
            break;
        case AppState::McapView:
            openViewerOfMcapData = true;
            createTabBarWithImPlotForMcapFile();
            break;
        }

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

    void createTabBarWithImPlotForZcmChannel()
    {
        if (!openViewerOfZcmData)
        {
            currentState = AppState::MainMenu;
            return;
        }
        int windowWidth, windowHeight;
        glfwGetFramebufferSize(glfwGetCurrentContext(), &windowWidth, &windowHeight);
        ImGui::SetNextWindowSize(ImVec2((float)windowWidth, (float)windowHeight));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
        ImGuiStyle &style = ImGui::GetStyle();
        ImVec2 original_window_padding = style.WindowPadding;
        style.WindowPadding.x = 4.0f; // keep TabItem distance to left window edge is 4
        style.WindowPadding.y = 4.0f; // keep TabItem distance to top window edge is 4
        if (ImGui::Begin("##ViewerOfZcmData", &openViewerOfZcmData, flags))
        {
            ImGui::SetWindowPos(ImVec2(0, 0));
            ImGui::SetWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y));
            //style.Colors[ImGuiCol_Tab] = HexToImVec4("#353333");
            //style.Colors[ImGuiCol_TabActive] = HexToImVec4("#353333");
            //style.Colors[ImGuiCol_WindowBg] = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
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
                if (ImGui::TabItemButton("[x]", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
                {
                    openViewerOfZcmData = false;
                    currentState = AppState::MainMenu;
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
                            bool popup_menu_open = false;
                            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                            {
                                ImGui::OpenPopup(("ChannelMenu_" + std::to_string(tab_id)).c_str());
                            }
                            if (ImGui::BeginPopup(("ChannelMenu_" + std::to_string(tab_id)).c_str()))
                            {
                                popup_menu_open = true;
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
                            //if (!popup_menu_open && !open_new_tab_dialog)
                            {
                                auto &channels = h.tab_selected_channels[tab_id];
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
                ImGui::OpenPopup("##TabItemTitle");
                open_new_tab_dialog = false;
            }

            ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                            ImGuiWindowFlags_NoNav;
            static bool first_open = true;
            if (ImGui::BeginPopupModal("##TabItemTitle", nullptr, window_flags))
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

    void createWelcomeWindow()
    {
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        if (ImGui::Begin("##WelcomeWindow", nullptr, flags))
        {
            // 获取窗口内容区域的尺寸（可用宽度和高度）
            ImVec2 contentRegion = ImGui::GetContentRegionAvail();
            float windowWidth = contentRegion.x;
            float windowHeight = contentRegion.y;

            // 按钮参数
            const float buttonWidth = 300.0f;
            const float buttonHeight = 60.0f;
            const float spacing = ImGui::GetStyle().ItemSpacing.x; // 按钮间水平间距（默认6px）

            // 计算按钮组的总宽度和总高度
            float totalWidth = 2 * buttonWidth + spacing;
            float totalHeight = buttonHeight; // 同一行，总高度等于单个按钮高度

            // 计算水平居中偏移量（窗口中心 - 按钮组中心）
            float offsetX = (windowWidth - totalWidth) * 0.5f;
            // 计算垂直居中偏移量（窗口中心 - 按钮组中心）
            float offsetY = (windowHeight - totalHeight) * 0.5f;

            // 设置光标位置到居中坐标
            ImGui::SetCursorPos(ImVec2(offsetX, offsetY));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(20, 10));
            // 绘制按钮
            if (ImGui::Button("ViewOnlineData", ImVec2(buttonWidth, buttonHeight)))
            {
                currentState = AppState::ZcmView;
            }
            ImGui::SameLine(); // 自动应用ItemSpacing间距
            if (ImGui::Button("ViewOfflineData", ImVec2(buttonWidth, buttonHeight)))
            {
                currentState = AppState::McapView;
            }
            ImGui::PopStyleVar();
        }
        ImGui::End();
    }

    void createTabBarWithImPlotForMcapFile()
    {
        if (!openViewerOfMcapData)
        {
            return;
        }
        int windowWidth, windowHeight;
        glfwGetFramebufferSize(glfwGetCurrentContext(), &windowWidth, &windowHeight);
        ImGui::SetNextWindowSize(ImVec2((float)windowWidth, (float)windowHeight));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
        ImGuiStyle &style = ImGui::GetStyle();
        ImVec2 original_window_padding = style.WindowPadding;
        style.WindowPadding.x = 4.0f; // keep TabItem distance to left window edge is 4
        style.WindowPadding.y = 4.0f; // keep TabItem distance to top window edge is 4
        if (ImGui::Begin("##ViewerOfZcmData", &openViewerOfMcapData, flags))
        {
            ImGui::SetWindowPos(ImVec2(0, 0));
            ImGui::SetWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y));
            //style.Colors[ImGuiCol_Tab] = HexToImVec4("#353333");
            //style.Colors[ImGuiCol_TabActive] = HexToImVec4("#353333");
            //style.Colors[ImGuiCol_WindowBg] = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
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
                if (ImGui::TabItemButton("[x]", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
                {
                    openViewerOfMcapData = false;
                    currentState = AppState::MainMenu;
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
                            bool popup_menu_open = false;
                            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                            {
                                ImGui::OpenPopup(("ChannelMenu_" + std::to_string(tab_id)).c_str());
                            }
                            if (ImGui::BeginPopup(("ChannelMenu_" + std::to_string(tab_id)).c_str()))
                            {
                                popup_menu_open = true;
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
                            //if (!popup_menu_open && !open_new_tab_dialog)
                            {
                                auto &channels = h.tab_selected_channels[tab_id];
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
                ImGui::OpenPopup("##TabItemTitle");
                open_new_tab_dialog = false;
            }

            ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                            ImGuiWindowFlags_NoNav;
            static bool first_open = true;
            if (ImGui::BeginPopupModal("##TabItemTitle", nullptr, window_flags))
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