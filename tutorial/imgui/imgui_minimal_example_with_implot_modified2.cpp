//
// Created by liuqiang on 25-5-13.
//

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <cstdio>
#include <zcm/zcm-cpp.hpp>
#include "timed_value.hpp"
#include <deque>
#include <implot.h>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include "GLFW/glfw3.h"
#include <Logging.h>
#include <set>
#include <atomic>
#include <regex>
#include <iomanip>
#include <unordered_set>
#include "data_fields.hpp"
#include "data_channel.hpp"
#include "rolling_buffer.h"

static inline bool contains(const std::string & str, const std::string & substring)
{
    return str.find(substring) != std::string::npos;
}

static inline bool contains(const std::string & str, const char character)
{
    return contains(str, std::string(1, character));
}

static inline bool starts_with(const std::string & str, const std::string & prefix)
{
    return str.rfind(prefix, 0) == 0;
}

static inline bool ends_with(const std::string &str, const char suffix)
{
    return !str.empty() && (str.back() == suffix);
}

static inline std::vector<std::string> split(const std::string &str, const char delim)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);

    std::string token;
    while (std::getline(ss, token, delim))
    {
        tokens.push_back(token);
    }

    // Match semantics of split(str,str)
    if (str.empty() || ends_with(str, delim))
    {
        tokens.emplace_back();
    }

    return tokens;
}

static inline std::vector<std::string> split(const std::string &str, const std::string &delim)
{
    size_t pos_start = 0, pos_end, delim_len = delim.length();
    std::string token;
    std::vector<std::string> tokens;

    while ((pos_end = str.find(delim, pos_start)) != std::string::npos)
    {
        token = str.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        tokens.push_back(token);
    }

    tokens.push_back(str.substr(pos_start));
    return tokens;
}

std::pair<std::string, std::string> split_last(const std::string &str, char delim)
{
    size_t last_slash_pos = str.find_last_of(delim);
    if (last_slash_pos == std::string::npos)
    {
        return {"", str};
    }
    std::string before = str.substr(0, last_slash_pos);
    std::string after = str.substr(last_slash_pos + 1);
    return {before, after};
}

std::vector<std::string> expand_range_expression(const std::string &expr)
{
    std::vector<std::string> result;
    std::regex pattern(R"(^([^\d]+)(\d+)-(\d+)$)");
    std::smatch matches;
    if (std::regex_match(expr, matches, pattern))
    {
        if (matches.size() != 4)
        {
            throw std::invalid_argument("无效的范围表达式格式");
        }
        std::string prefix = matches[1].str();
        int start = std::stoi(matches[2].str());
        int end = std::stoi(matches[3].str());
        if (start > end)
        {
            throw std::invalid_argument("起始数字不能大于结束数字");
        }
        size_t width = matches[2].str().length();
        for (int i = start; i <= end; ++i)
        {
            std::stringstream ss;
            ss << prefix << std::setw(int(width)) << std::setfill('0') << i;
            result.push_back(ss.str());
        }
    }
    else
    {
        result.push_back(expr);
    }
    return result;
}

class Handler
{
public:
    static constexpr size_t MAX_CACHE_SIZE = 10000;
    using DataBUffer = RollingBuffer<double, MAX_CACHE_SIZE>;
    ~Handler() = default;

    void plotChannelData(const std::string &title, const std::string &channel)
    {
        static bool show_plot = true;
        if (show_plot)
        {
            ImGui::Begin(title.c_str(), &show_plot, ImGuiWindowFlags_AlwaysAutoResize);
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
        static bool show_plot = true;
        if (show_plot)
        {
            ImGui::Begin(title.c_str(), &show_plot, ImGuiWindowFlags_AlwaysAutoResize);
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

            // 绘制图表
            if (ImPlot::BeginPlot("##ChannelPlot", ImVec2(600, 400)))
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

//    void handle(const zcm::ReceiveBuffer *buffer, const std::string &channel, const timed_value *msg)
//    {
//        channel_data[channel].first.push_back(msg->timestamp);
//        channel_data[channel].second.push_back(msg->value);
//        if (channel_data[channel].first.size() > MAX_CACHE_SIZE)
//        {
//            channel_data[channel].first.pop_front();
//            channel_data[channel].second.pop_front();
//        }
//        auto timestamps = std::vector<double>(channel_data[channel].first.begin(), channel_data[channel].first.end());
//        auto values = std::vector<double>(channel_data[channel].second.begin(), channel_data[channel].second.end());
//        CLOG_INFO << "values: " << values.size();
//        {
//            channel_plot_data[channel].first = timestamps;
//            channel_plot_data[channel].second = values;
//        }
//    }

    void handle(const zcm::ReceiveBuffer *buffer, const std::string &channel, const timed_value *msg)
    {
        channel_data[channel].first.push_back(msg->timestamp);
        channel_data[channel].second.push_back(msg->value);
        {
            std::lock_guard<std::shared_mutex> lck(mtx);
            channel_plot_data[channel].first = channel_data[channel].first.data();
            channel_plot_data[channel].second = channel_data[channel].second.data();
        }
        CLOG_INFO << channel << "msg->timestamp " << msg->timestamp << ", " << msg->value;
    }

    void new_channel(const zcm::ReceiveBuffer *buffer, const std::string &channel, const data_channel *msg)
    {
        std::lock_guard<std::shared_mutex> lck(mtx);
        zcm->pause();
        zcm->subscribe(msg->channel, &Handler::handle, this);
        zcm->resume();
        all_channels.insert(msg->channel);
        for (auto &[key, value] : plot_channels)
        {
            if (contains(key, "*"))
            {
                auto lhd = split_last(key, '*').first;
                if (contains(msg->channel, lhd))
                {
                    plot_channels[key].push_back(msg->channel);
                }
            }
            if (contains(key, "-"))
            {
                for (auto &element : expand_range_expression(key))
                {
                    if (msg->channel == element)
                    {
                        plot_channels[key].push_back(msg->channel);
                    }
                }
            }
        }
        CLOG_INFO << msg->channel;
    }

    void channels_rep(const zcm::ReceiveBuffer *buffer, const std::string &channel, const data_fields *msg)
    {
        std::lock_guard<std::shared_mutex> lck(mtx);
        for (auto &field : msg->channels)
        {
            zcm->pause();
            zcm->subscribe(field, &Handler::handle, this);
            zcm->resume();
            all_channels.insert(field);
            for (auto &[key, value] : plot_channels)
            {
                if (contains(key, "*"))
                {
                    auto lhd = split_last(key, '*').first;
                    if (contains(field, lhd))
                    {
                        plot_channels[key].push_back(field);
                    }
                }
                if (contains(key, "-"))
                {
                    for (auto &element : expand_range_expression(key))
                    {
                        if (field == element)
                        {
                            plot_channels[key].push_back(field);
                        }
                    }
                }
            }
            CLOG_INFO << field;
        }
    }

//    std::unordered_map<std::string, std::pair<std::deque<double>, std::deque<double>>> channel_data;
    std::unordered_map<std::string, std::pair<DataBUffer, DataBUffer>> channel_data;
    std::unordered_map<std::string, std::pair<std::vector<double>, std::vector<double>>> channel_plot_data;
    std::unordered_set<std::string> all_channels;
    std::unordered_map<std::string, std::vector<std::string>> plot_channels;
//    std::vector<std::string> channels;

    zcm::ZCM *zcm{nullptr};
    std::shared_mutex mtx;
};

class Plotter
{
private:
    std::string winTitle;
    GLFWwindow *window{nullptr};
    bool button_left{false};
    bool button_middle{false};
    bool button_right{false};
    double last_x{0};
    double last_y{0};
    std::unique_ptr<zcm::ZCM> zcm, zcm_channels;
    Handler h;
    std::thread th_zcm, th_zcm_channels;
    ImVec4 clear{0.45f, 0.55f, 0.60f, 1.00f};
    std::set<std::string> availableChannels;
    std::atomic_bool exit{false};

public:
    explicit Plotter(const std::string &name)
    {
        winTitle = name;
        zcm::RegisterAllPlugins();
    }
    Plotter()
    {
        winTitle = "Plotter";
        zcm::RegisterAllPlugins();
    }

    ~Plotter()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
        zcm->stop();
        zcm_channels->stop();
        exit = true;
        if (th_zcm.joinable())
        {
            th_zcm.join();
        }
        if (th_zcm_channels.joinable())
        {
            th_zcm_channels.join();
        }
    }

    bool init()
    {
        if (!glfwInit())
        {
            CLOG_ERROR << "Init GLFW Failed.";
            return false;
        }
        const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        int screen_width = mode->width;
        int screen_height = mode->height;
        int window_width = 1920;
        int window_height = 1080;
        int x_pos = (screen_width - window_width) / 2;
        int y_pos = (screen_height - window_height) / 2;

        const char *glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);                // or 3
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE); // or GLFW_OPENGL_COMPAT_PROFILE
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        window = glfwCreateWindow(window_width, window_height, "Viewer", nullptr, nullptr);
        if (!window)
        {
            glfwTerminate();
            CLOG_ERROR << "Failed to create GLFW Window.";
            return false;
        }
        glfwSetWindowPos(window, x_pos, y_pos);
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
        ImGui_ImplOpenGL3_Init(glsl_version);

        glfwSetWindowUserPointer(window, this);
        glfwSetMouseButtonCallback(window, mouseClickCallback);
        glfwSetCursorPosCallback(window, mouseMoveCallback);
        glfwSetScrollCallback(window, mouseScrollCallback);
        glfwSetErrorCallback(errorCallback);

        zcm = std::make_unique<zcm::ZCM>("ipcshm://");
        zcm_channels = std::make_unique<zcm::ZCM>("ipcshm://");
        if (!(zcm->good() && zcm_channels->good()))
        {
            zcm.reset();
            zcm_channels.reset();
            return false;
        }
        h.zcm = zcm.get();
        data_fields channels_req;
        channels_req.cnt = 0;
        zcm_channels->publish("channels_req", &channels_req);
        zcm_channels->subscribe("channels_rep", &Handler::channels_rep, &h);
        zcm_channels->subscribe("new_channel", &Handler::new_channel, &h);
        th_zcm_channels = std::thread(
            [&]()
            {
                zcm_channels->run();
            });
        th_zcm = std::thread(
            [&]()
            {
                zcm->run();
            });
        return true;
    }

    // support fuzzy subscription e.g. "pos/*", "q/1-7", but do not support the inclusion of both "*" and "-"
    bool plot(const std::string &channel)
    {
        if (!(contains(channel, "*") || contains(channel, "-")))
        {
            h.plot_channels[channel].push_back(channel);
        }
        if (contains(channel, "*") && contains(channel, "-"))
        {
            CLOG_ERROR << R"(Do not support the inclusion of both "*" and "-")";
            return false;
        }
        if (contains(channel, "*"))
        {
            auto [lhd, rhd] = split_last(channel, '*');
            if (contains(lhd, "*"))
            {
                CLOG_ERROR << "Do not support the inclusion of multiple \"*\"";
                return false;
            }
        }
        if (contains(channel, "-"))
        {
            auto [lhd, rhd] = split_last(channel, '-');
            if (contains(lhd, "-"))
            {
                CLOG_ERROR << "Do not support the inclusion of multiple \"-\"";
                return false;
            }
        }
        h.plot_channels[channel];
        return true;
    }

    void render()
    {
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            return;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //        for (auto &[key, val] : plotChannel)
        //        {
        //            h.plotChannelData(key, val);
        //        }
        for (auto &[key, val] : h.plot_channels)
        {
            h.plotChannelData(key, val);
        }
        //        ImGui::Button("Right Click Me");
        //        static bool isEnabled{false};
        //        if (ImGui::BeginPopupContextItem("MyPopup"))
        //        {
        //            // 菜单项
        //            if (ImGui::MenuItem("Item One"))
        //            {
        //                // 处理逻辑
        //            }
        //            if (ImGui::MenuItem("Item Two", nullptr, false, isEnabled))
        //            {
        //                // 处理逻辑
        //            }
        //            ImGui::EndPopup();
        //        }

//        ImGuiIO &io = ImGui::GetIO();
//        ImVec2 display_size = io.DisplaySize;
//
//        ImGui::SetNextWindowPos(ImVec2(0, 0));
//        ImGui::SetNextWindowSize(display_size);
//        bool shiftHeld = (io.KeyMods & ImGuiMod_Ctrl) != 0;
//
//        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
//                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
//                                        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
//                                        ImGuiWindowFlags_NoBackground;
//
//        if (ImGui::Begin("##Fullscreen", nullptr, window_flags))
//        {
//            bool show_popup = false;
//            static bool popup_open = false;
//            ImPlot::SetNextAxisToFit(ImAxis_Y1);
//            ImVec2 plot_size(600, 400);
//            ImGui::InvisibleButton("PlotOverlay", plot_size);
//            bool hovered = ImGui::IsItemHovered();
//            bool ctrl_right_click =
//                hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && (io.KeyMods & ImGuiMod_Ctrl);
//            if (ctrl_right_click)
//            {
//                ImGui::OpenPopup("ChannelPopup");
//                // 设置一个标志位，表示我们正在“自定义交互”，需要屏蔽 ImPlot 行为
//            }
//            if (ImPlot::BeginPlot("##Scrolling", ImVec2(600, 400)))
//            {
//                if (ImGui::IsPopupOpen("ChannelPopup"))
//                {
//                }
//
//                ImPlot::SetupAxisFormat(ImAxis_X1, "%.3f");
//                ImPlot::SetupAxes("Time (s)", "Value");
//                ImPlot::EndPlot();
//            }
//
//            // 如何上面的ImGui::Begin()是在ImPlot()窗口中，那么鼠标右键点击是不是关联的就是此窗口中ImPlot的数据
//            static std::vector<std::string> all_channels = {"camera/left", "camera/right", "lidar/scan", "imu/data"};
//            static std::unordered_map<std::string, bool> channel_enabled;
//            ImGui::GetStyle().Colors[ImGuiCol_PopupBg] = ImVec4(0.2f, 0.2f, 0.2f, 0.4f);
//            if (shiftHeld && ImGui::BeginPopupContextWindow("MyWindowPopup", ImGuiPopupFlags_MouseButtonRight))
//            {
//                ImGui::Text("Hello World");
//                ImGui::Separator();
//                for (const std::string &channel : all_channels)
//                {
//                    // 初始化 map 中的默认值（第一次使用）
//                    if (channel_enabled.find(channel) == channel_enabled.end())
//                    {
//                        channel_enabled[channel] = true; // 默认开启
//                    }
//                    std::string unique_id = "##" + channel; // 不显示 ID，只用于内部唯一性
//                    // 将复选框和名称放在一行
//                    bool *checked = &channel_enabled[channel];
//                    ImGui::Checkbox(unique_id.c_str(), checked); // 显示复选框
//                    ImGui::SameLine();
//                    ImGui::TextUnformatted(channel.c_str()); // 显示通道名
//                }
//                ImGui::EndPopup();
//            }
//        }
//        ImGui::End();

        // ImGui render
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear.x * clear.w, clear.y * clear.w, clear.z * clear.w, clear.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (!ImGui::GetIO().WantCaptureMouse)
        {
        }
        glfwSwapBuffers(window);
        glfwPollEvents(); // process events
    }

    bool shouldClose() const
    {
        return glfwWindowShouldClose(window);
    }

protected:
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

    static void mouseClickCallback(GLFWwindow *win, int button, int action, int mods)
    {
        auto *self = static_cast<Plotter *>(glfwGetWindowUserPointer(win));
        if (self)
        {
            self->mouseClick(win, button, action, mods);
        }
    }

    static void mouseMoveCallback(GLFWwindow *win, double xPos, double yPos)
    {
        auto *self = static_cast<Plotter *>(glfwGetWindowUserPointer(win));
        if (self)
        {
            self->mouseMove(win, xPos, yPos);
        }
    }

    static void mouseScrollCallback(GLFWwindow *win, double xOffset, double yOffset)
    {
        auto *self = static_cast<Plotter *>(glfwGetWindowUserPointer(win));
        if (self)
        {
            Plotter::mouseScroll(win, xOffset, yOffset);
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

int main(int, char **)
{
    Plotter pt;
    if (!pt.init())
    {
        return -1;
    }
    pt.plot("pos/*");
    pt.plot("Joint/*");
//    pt.plot("joint_position", "q/1-7");
    while (!pt.shouldClose())
    {
        pt.render();
    }
    return 0;
}
