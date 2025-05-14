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

class Handler
{
public:
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
                        ImPlot::PlotLine(channel.c_str(), ts.data(), vals.data(), (int)ts.size());
                        ImPlot::PlotScatter("##Debug", ts.data(), vals.data(), (int)ts.size());
                    }
                }
                ImPlot::EndPlot();
            }
            ImGui::End();
        }
    }

    //    void plotChannelData(const std::string &title, const std::vector<std::string> &channels)
    //    {
    //        static bool show_plot = true;
    //        if (show_plot)
    //        {
    //            ImGui::Begin(title.c_str(), &show_plot, ImGuiWindowFlags_AlwaysAutoResize);
    //            ImPlot::SetNextAxisToFit(ImAxis_Y1);
    //            if (ImPlot::BeginPlot("##Scrolling", ImVec2(600, 400)))
    //            {
    //                ImPlot::SetupAxisFormat(ImAxis_X1, "%.3f");
    //                //ImPlot::SetupAxisFormat(ImAxis_Y1, "%.5f");
    //                ImPlot::SetupAxes("Time(s)", "Value");
    //                for (auto &channel : channels)
    //                {
    //                    if (channel_plot_data.count(channel) > 0)
    //                    {
    //                        std::lock_guard<std::shared_mutex> lock(mtx);
    //                        auto &[ts, vals] = channel_plot_data[channel];
    //                        if (!ts.empty() && !vals.empty() && ts.size() == vals.size())
    //                        {
    //                            double min_time = *std::min_element(ts.begin(), ts.end());
    //                            double max_time = *std::max_element(ts.begin(), ts.end());
    //                            ImPlot::SetupAxisLimits(ImAxis_X1, min_time, max_time, ImGuiCond_Always);
    //                            auto [y_min, y_max] = std::minmax_element(vals.begin(), vals.end());
    //                            ImPlot::SetupAxisLimits(ImAxis_Y1, *y_min, *y_max, ImGuiCond_Always);
    //                            ImPlot::PlotLine(channel.c_str(), ts.data(), vals.data(), (int)ts.size());
    //                            ImPlot::PlotScatter("##Debug", ts.data(), vals.data(), (int)ts.size());
    //                        }
    //                    }
    //                }
    //                ImPlot::EndPlot();
    //            }
    //            ImGui::End();
    //        }
    //    }

    //    void plotChannelData(const std::string &title, const std::vector<std::string> &channels)
    //    {
    //        static bool show_plot = true;
    //        if (show_plot)
    //        {
    //            ImGui::Begin(title.c_str(), &show_plot, ImGuiWindowFlags_AlwaysAutoResize);
    //            ImPlot::SetNextAxisToFit(ImAxis_Y1);
    //
    //            // 初始化全局范围
    //            auto global_x_min = DBL_MAX;
    //            auto global_x_max = -DBL_MAX;
    //            auto global_y_min = DBL_MAX;
    //            auto global_y_max = -DBL_MAX;
    //
    //            // 使用静态变量存储各通道的显示状态
    //            static std::unordered_map<std::string, bool> channel_visible;
    //            // 初始化新添加的通道显示状态
    //            for (const auto &channel : channels)
    //            {
    //                if (channel_visible.find(channel) == channel_visible.end())
    //                {
    //                    channel_visible[channel] = true;
    //                }
    //            }
    //
    //            // 第一遍遍历：计算所有通道的全局范围
    //            {
    //                std::shared_lock<std::shared_mutex> lock(mtx);
    //                for (auto &channel : channels)
    //                {
    //                    if (channel_visible[channel] && channel_plot_data.count(channel) > 0)
    //                    {
    //                        auto &[ts, vals] = channel_plot_data[channel];
    //                        if (!ts.empty() && !vals.empty() && ts.size() == vals.size())
    //                        {
    //                            double min_time = *std::min_element(ts.begin(), ts.end());
    //                            double max_time = *std::max_element(ts.begin(), ts.end());
    //                            auto [y_min, y_max] = std::minmax_element(vals.begin(), vals.end());
    //
    //                            global_x_min = std::min(global_x_min, min_time);
    //                            global_x_max = std::max(global_x_max, max_time);
    //                            global_y_min = std::min(global_y_min, *y_min);
    //                            global_y_max = std::max(global_y_max, *y_max);
    //                        }
    //                    }
    //                }
    //            }
    //
    //            if (ImPlot::BeginPlot("##Scrolling", ImVec2(600, 400), ImPlotFlags_None))
    //                {
    //                    ImPlot::SetupAxisFormat(ImAxis_X1, "%.3f");
    //                    ImPlot::SetupAxes("Time(s)", "Value");
    //
    //                    // 设置全局范围
    //                    if (global_x_min <= global_x_max && global_y_min <= global_y_max)
    //                    {
    //                        ImPlot::SetupAxisLimits(ImAxis_X1, global_x_min, global_x_max, ImGuiCond_Always);
    //                        ImPlot::SetupAxisLimits(ImAxis_Y1, global_y_min, global_y_max, ImGuiCond_Always);
    //                    }
    //                    // 存储各通道的颜色
    //                    static std::unordered_map<std::string, ImU32> channel_colors;
    //
    //                    // 第二遍遍历：绘制所有通道
    //                    {
    //                        std::shared_lock<std::shared_mutex> lock(mtx);
    //                        for (auto &channel : channels)
    //                        {
    //                            if (channel_plot_data.count(channel) > 0)
    //                            {
    //                                auto &[ts, vals] = channel_plot_data[channel];
    //                                if (!ts.empty() && !vals.empty() && ts.size() == vals.size())
    //                                {
    //                                    // 使用唯一ID绘制散点图
    //                                    std::string scatter_id = "##Debug_" + channel;
    //
    //                                    ImPlot::PlotLine(channel.c_str(), ts.data(), vals.data(),
    //                                    (int)ts.size()); ImPlot::PlotScatter(scatter_id.c_str(),
    //                                    ts.data(), vals.data(), (int)ts.size());
    //                                }
    //                            }
    //                        }
    //                    }
    //                    ImPlot::EndPlot();
    //                }
    //            ImGui::End();
    //        }
    //    }

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
                                ImPlot::SetNextLineStyle(ImPlot::GetColormapColor(i), 1.f);
//                                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 4.0f);
                                //ImPlot::PlotScatter(scatter_id.c_str(), ts.data(), vals.data(), (int)ts.size());
                                ImPlot::PlotLine(channel.c_str(), ts.data(), vals.data(), (int)ts.size());
                            }
                        }
                    }
                }
                ImPlot::EndPlot();
            }
            ImGui::End();
        }
    }

    void handle(const zcm::ReceiveBuffer *buffer, const std::string &channel, const timed_value *msg)
    {
        channel_data[channel].first.push_back(msg->timestamp);
        channel_data[channel].second.push_back(msg->value);
        if (channel_data[channel].first.size() > 10000)
        {
            channel_data[channel].first.pop_front();
            channel_data[channel].second.pop_front();
        }
        auto timestamps = std::vector<double>(channel_data[channel].first.begin(), channel_data[channel].first.end());
        auto values = std::vector<double>(channel_data[channel].second.begin(), channel_data[channel].second.end());
        {
            std::lock_guard<std::shared_mutex> lock(mtx);
            channel_plot_data[channel].first = timestamps;
            channel_plot_data[channel].second = values;
        }
    }

    std::unordered_map<std::string, std::pair<std::deque<double>, std::deque<double>>> channel_data;
    std::unordered_map<std::string, std::pair<std::vector<double>, std::vector<double>>> channel_plot_data;
    std::shared_mutex mtx;
};

template <typename T>
class CircularBuffer
{
private:
    std::vector<T> buffer;
    size_t head = 0, tail = 0;
    bool full = false;

public:
    explicit CircularBuffer(size_t size) : buffer(size) {}

    void push(const T &item)
    {
        buffer[tail] = item;
        tail = (tail + 1) % buffer.size();
        if (tail == head)
        {
            full = true;
        }
    }

    T pop()
    {
        T item = buffer[head];
        head = (head + 1) % buffer.size();
        full = false;
        return item;
    }
};

class Plotter
{
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
        th.join();
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
        // 1. 创建窗口前设置多重采样
        glfwWindowHint(GLFW_SAMPLES, 4); // 开启 4x MSAA

        // 2. 创建 OpenGL 上下文之后，启用多重采样
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

        glfwSetWindowUserPointer(window, this); // 设置窗口用户指针
        glfwSetMouseButtonCallback(window, mouseClickCallback);
        glfwSetCursorPosCallback(window, mouseMoveCallback);
        glfwSetScrollCallback(window, mouseScrollCallback);
        glfwSetErrorCallback(errorCallback);

        zcm = std::make_unique<zcm::ZCM>("ipcshm://");
        if (!zcm->good())
        {
            return false;
        }
        return true;
    }

    void subscribe(const std::vector<std::string> &channels)
    {
        for (auto &channel : channels)
        {
            availableChannels.insert(channel);
            zcm->subscribe(channel, &Handler::handle, &h);
        }
        th = std::thread(
            [&]()
            {
                zcm->run();
            });
    }

    bool plot(const std::string &title, const std::vector<std::string> &channels)
    {
        for (auto &channel : channels)
        {
            if (availableChannels.count(channel) <= 0)
            {
                return false;
            }
        }
        plotChannels.insert({title, channels});
        return true;
    }

    bool plot(const std::string &title, const std::string &channel)
    {
        if (availableChannels.count(channel) <= 0)
        {
            return false;
        }
        plotChannel.insert({title, channel});
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

        for (auto &[key, val] : plotChannel)
        {
            h.plotChannelData(key, val);
        }
        for (auto &[key, val] : plotChannels)
        {
            h.plotChannelData(key, val);
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
    std::thread th;
    ImVec4 clear{0.45f, 0.55f, 0.60f, 1.00f};
    std::set<std::string> availableChannels;
    std::unordered_map<std::string, std::vector<std::string>> plotChannels;
    std::unordered_map<std::string, std::string> plotChannel;
};

int main(int, char **)
{
    Plotter plot;
    if (!plot.init())
    {
        return -1;
    }
    plot.subscribe({"pos/x", "pos/y", "pos/z"});
    plot.plot("pos/x", "pos/x");
    plot.plot("pos", {"pos/x", "pos/y", "pos/z"});
    while (!plot.shouldClose())
    {
        plot.render();
    }
    return 0;
}
