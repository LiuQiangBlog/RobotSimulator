//
// Created by liuqiang on 25-5-13.
//

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <cstdio>
#include <zcm/zcm-cpp.hpp>
#include "all_timed_value.hpp"
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

static inline bool contains(const std::string &str, const std::string &substring)
{
    return str.find(substring) != std::string::npos;
}

static inline bool contains(const std::string &str, const char character)
{
    return contains(str, std::string(1, character));
}

static inline bool starts_with(const std::string &str, const std::string &prefix)
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

// std::vector<std::string> expand_range_expression(const std::string &expr)
//{
//     std::vector<std::string> result;
//     std::regex pattern(R"(^([^\d]+)(\d+)-(\d+)$)");
//     std::smatch matches;
//     if (std::regex_match(expr, matches, pattern))
//     {
//         if (matches.size() != 4)
//         {
//             throw std::invalid_argument("无效的范围表达式格式");
//         }
//         std::string prefix = matches[1].str();
//         int start = std::stoi(matches[2].str());
//         int end = std::stoi(matches[3].str());
//         if (start > end)
//         {
//             throw std::invalid_argument("起始数字不能大于结束数字");
//         }
//         size_t width = matches[2].str().length();
//         for (int i = start; i <= end; ++i)
//         {
//             std::stringstream ss;
//             ss << prefix << std::setw(int(width)) << std::setfill('0') << i;
//             result.push_back(ss.str());
//         }
//     }
//     else
//     {
//         result.push_back(expr);
//     }
//     return result;
// }

std::vector<std::string> expand_range_expression(const std::string &expr)
{
    std::vector<std::string> result;
    // 区分带方括号和不带方括号的情况
    std::regex pattern_with_brackets(R"(^([^\[]+)\[(\d+)-(\d+)\]$)");
    std::regex pattern_without_brackets(R"(^([^\d]+)(\d+)-(\d+)$)");
    std::smatch matches;

    // 检查是否匹配带方括号的格式
    if (std::regex_match(expr, matches, pattern_with_brackets))
    {
        if (matches.size() != 4)
        {
            throw std::invalid_argument("无效的带方括号范围表达式格式");
        }
        std::string prefix = matches[1].str();
        int start = std::stoi(matches[2].str());
        int end = std::stoi(matches[3].str());
        if (start > end)
        {
            throw std::invalid_argument("起始数字不能大于结束数字");
        }

        for (int i = start; i <= end; ++i)
        {
            // 带方括号的格式不进行零填充
            result.push_back(prefix + "[" + std::to_string(i) + "]");
        }
    }
    // 检查是否匹配不带方括号的格式
    else if (std::regex_match(expr, matches, pattern_without_brackets))
    {
        if (matches.size() != 4)
        {
            throw std::invalid_argument("无效的不带方括号范围表达式格式");
        }
        std::string prefix = matches[1].str();
        int start = std::stoi(matches[2].str());
        int end = std::stoi(matches[3].str());
        if (start > end)
        {
            throw std::invalid_argument("起始数字不能大于结束数字");
        }

        // 计算最大宽度
        size_t start_width = matches[2].str().length();
        size_t end_width = matches[3].str().length();
        size_t max_width = std::max(start_width, end_width);

        for (int i = start; i <= end; ++i)
        {
            std::stringstream ss;
            ss << prefix;
            std::string num_str = std::to_string(i);

            // 不带方括号的格式，根据起始和结束数字的宽度决定是否零填充
            if (start_width == end_width)
            {
                // 如果起始和结束数字宽度相同，按该宽度填充
                ss << std::setw(int(start_width)) << std::setfill('0') << i;
            }
            else
            {
                // 如果宽度不同，仅在数字位数小于最大宽度时填充
                if (num_str.length() < max_width)
                {
                    ss << std::setw(int(max_width)) << std::setfill('0') << i;
                }
                else
                {
                    ss << i;
                }
            }
            result.push_back(ss.str());
        }
    }
    // 不匹配任何模式，直接返回原表达式
    else
    {
        result.push_back(expr);
    }

    return result;
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

    //    // plot all channels
    //    void plotChannelData(const std::string &title, const std::vector<std::string> &channels)
    //    {
    //        for (const auto &channel : channels)
    //        {
    //            if (channel_plot_data.count(channel) > 0)
    //            {
    //                auto &[ts, vals] = channel_plot_data[channel];
    //                if (!ts.empty())
    //                {
    //                    std::cout << channel
    //                              << ": start=" << ts.front()
    //                              << ", end=" << ts.back()
    //                              << ", size=" << ts.size()
    //                              << std::endl;
    //                }
    //            }
    //        }
    //        if (plot_bool[title])
    //        {
    //            // 获取当前屏幕尺寸
    //            ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    //
    //            // 设置窗口布局
    //            if (state[title].is_maximized)
    //            {
    //                ImGui::SetNextWindowPos(ImVec2(0, 0));
    //                ImGui::SetNextWindowSize(screen_size);
    //            }
    //            else if (state[title].need_restore_pos)
    //            {
    //                ImGui::SetNextWindowPos(state[title].normal_pos);
    //                ImGui::SetNextWindowSize(state[title].normal_size);
    //                state[title].need_restore_pos = false;
    //            }
    //
    //            ImGui::Begin(title.c_str(), &plot_bool[title],
    //                         ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
    //
    //            // 添加自定义按钮
    //            ImGui::SameLine(ImGui::GetWindowWidth() - 120); // 右侧定位
    //            if (ImGui::Button(state[title].is_maximized ? "[ ]" : "[_]", ImVec2(30, 30)))
    //            {
    //                state[title].is_maximized = !state[title].is_maximized;
    //                state[title].screen_size = screen_size;
    //                if (!state[title].is_maximized)
    //                {
    //                    state[title].need_restore_pos = true;
    //                }
    //            }
    //
    //            // 保存正常状态下的窗口信息
    //            if (!state[title].is_maximized)
    //            {
    //                state[title].normal_pos = ImGui::GetWindowPos();
    //                state[title].normal_size = ImGui::GetWindowSize();
    //            }
    //            ImVec2 plot_size = state[title].is_maximized ?
    //                                                         ImVec2(state[title].screen_size.x - 20,
    //                                                         state[title].screen_size.y - 80) : ImVec2(600, 400);
    //
    //
    ////            ImGui::Begin(title.c_str(), &plot_bool[title], ImGuiWindowFlags_AlwaysAutoResize);
    //            ImPlot::SetNextAxisToFit(ImAxis_Y1);
    //
    //            // 初始化全局范围
    //            auto global_x_min = DBL_MAX;
    //            auto global_x_max = -DBL_MAX;
    //            auto global_y_min = DBL_MAX;
    //            auto global_y_max = -DBL_MAX;
    //
    //            // 计算所有通道的全局范围
    //            {
    //                std::shared_lock<std::shared_mutex> lock(mtx);
    //                for (const auto &channel : channels)
    //                {
    //                    if (channel_plot_data.count(channel) > 0)
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
    //            // 绘制图表
    //            if (ImPlot::BeginPlot("##ChannelPlot", plot_size))
    //            {
    //                ImPlot::SetupAxisFormat(ImAxis_X1, "%.3f");
    //                ImPlot::SetupAxes("Time(s)", "Value");
    //                ImPlot::GetPlotDrawList()->Flags |= ImDrawListFlags_AntiAliasedLines;
    //                // 设置全局范围
    //                if (global_x_min <= global_x_max && global_y_min <= global_y_max)
    //                {
    //                    ImPlot::SetupAxisLimits(ImAxis_X1, global_x_min, global_x_max, ImGuiCond_Always);
    //                    ImPlot::SetupAxisLimits(ImAxis_Y1, global_y_min, global_y_max, ImGuiCond_Always);
    //                }
    //
    //                // 绘制所有通道
    //                {
    //                    std::shared_lock<std::shared_mutex> lock(mtx);
    //                    for (size_t i = 0; i < channels.size(); i++)
    //                    {
    //                        const auto &channel = channels[i];
    //                        if (channel_plot_data.count(channel) > 0)
    //                        {
    //                            auto &[ts, vals] = channel_plot_data[channel];
    //                            if (!ts.empty() && !vals.empty() && ts.size() == vals.size())
    //                            {
    //                                ImVec4 color = ImPlot::GetColormapColor(int(i));
    //                                ImPlot::SetNextLineStyle(color, 2.0f);
    //                                ImPlot::PlotLine(channel.c_str(), ts.data(), vals.data(), (int)ts.size());
    //                                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 2.0f, color, -1.0f, color);
    //                                ImPlot::PlotScatter(channel.c_str(), ts.data(), vals.data(), (int)ts.size());
    //                            }
    //                        }
    //                    }
    //                }
    //                ImPlot::EndPlot();
    //            }
    //            ImGui::End();
    //        }
    //    }

    // plot all channels
    void plotChannelData(const std::string &title, const std::vector<std::string> &channels)
    {
//        for (const auto &channel : channels)
//        {
//            if (channel_plot_data.count(channel) > 0)
//            {
//                auto &[ts, vals] = channel_plot_data[channel];
//                if (!ts.empty())
//                {
//                    std::cout << channel << ": start=" << ts.front() << ", end=" << ts.back() << ", size=" << ts.size()
//                              << std::endl;
//                }
//            }
//        }

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

    //    void plotChannelData(const std::string &title, const std::vector<std::string> &channels) {
    //        // 初始化窗口状态（首次创建时）
    //        if (plot_window_states.find(title) == plot_window_states.end()) {
    //            plot_window_states[title] = PlotWindowState();
    //        }
    //        auto& state = plot_window_states[title];
    //
    //        // 窗口标志：保留标题栏（用于显示最大化按钮）
    //        ImGuiWindowFlags flags = ImGuiWindowFlags_None;
    //        if (state.is_maximized) {
    //            flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;  // 最大化时禁用拖动/缩放
    //        }
    //
    //        // 开始窗口（显示标题栏和装饰按钮）
    //        ImGui::Begin(title.c_str(), &plot_bool[title], flags);
    //
    //        // ---------------------- 标题栏右侧添加最大化按钮 ----------------------
    //        if (ImGui::BeginMenuBar()) {
    //            if (ImGui::BeginMenu("")) {  // 空菜单用于右对齐按钮
    //                ImGui::EndMenu();
    //            }
    //            ImGui::SameLine(0, -ImGui::GetStyle().ItemSpacing.x);  // 右对齐
    //
    //            // 最大化/恢复按钮（模仿系统原生图标，可用文字或 FontAwesome 图标）
    //            const char* button_label = state.is_maximized ? "🗕" : "🗔";  // 🗕=恢复，🗔=最大化
    //            if (ImGui::Button(button_label, ImVec2(20, 20))) {
    //                state.is_maximized = !state.is_maximized;
    //                if (state.is_maximized) {
    //                    // 保存当前状态并最大化
    //                    state.normal_size = ImGui::GetWindowSize();
    //                    state.normal_pos = ImGui::GetWindowPos();
    //                    ImGuiIO& io = ImGui::GetIO();
    //                    ImGui::SetWindowPos(title.c_str(), ImVec2(0, 0));
    //                    ImGui::SetWindowSize(title.c_str(), io.DisplaySize);
    //                } else {
    //                    // 恢复正常状态
    //                    ImGui::SetWindowPos(title.c_str(), state.normal_pos);
    //                    ImGui::SetWindowSize(title.c_str(), state.normal_size);
    //                }
    //            }
    //            ImGui::SameLine();
    //
    //            // 原生关闭按钮（若需要保留 ImGui 自带的关闭按钮）
    //            if (ImGui::Button("×", ImVec2(20, 20))) {
    //                plot_bool[title] = false;
    //            }
    //            ImGui::EndMenuBar();
    //        }
    //        // ---------------------------------------------------------------------
    //
    //        // 初始化全局范围
    //        auto global_x_min = DBL_MAX;
    //        auto global_x_max = -DBL_MAX;
    //        auto global_y_min = DBL_MAX;
    //        auto global_y_max = -DBL_MAX;
    //        // 计算所有通道的全局范围
    //        {
    //            std::shared_lock<std::shared_mutex> lock(mtx);
    //            for (const auto &channel : channels)
    //            {
    //                if (channel_plot_data.count(channel) > 0)
    //                {
    //                    auto &[ts, vals] = channel_plot_data[channel];
    //                    if (!ts.empty() && !vals.empty() && ts.size() == vals.size())
    //                    {
    //                        double min_time = *std::min_element(ts.begin(), ts.end());
    //                        double max_time = *std::max_element(ts.begin(), ts.end());
    //                        auto [y_min, y_max] = std::minmax_element(vals.begin(), vals.end());
    //                        global_x_min = std::min(global_x_min, min_time);
    //                        global_x_max = std::max(global_x_max, max_time);
    //                        global_y_min = std::min(global_y_min, *y_min);
    //                        global_y_max = std::max(global_y_max, *y_max);
    //                    }
    //                }
    //            }
    //        }
    //
    //        // 绘图逻辑（与原代码一致，调整绘图区域自适应窗口大小）
    //        ImVec2 plot_size = ImGui::GetContentRegionAvail();  // 自动填充窗口剩余空间
    //        if (ImPlot::BeginPlot("##ChannelPlot", plot_size)) {
    //            ImPlot::SetupAxisFormat(ImAxis_X1, "%.3f");
    //            ImPlot::SetupAxes("Time(s)", "Value");
    //            ImPlot::GetPlotDrawList()->Flags |= ImDrawListFlags_AntiAliasedLines;
    //            // 设置全局范围
    //            if (global_x_min <= global_x_max && global_y_min <= global_y_max)
    //            {
    //                ImPlot::SetupAxisLimits(ImAxis_X1, global_x_min, global_x_max, ImGuiCond_Always);
    //                ImPlot::SetupAxisLimits(ImAxis_Y1, global_y_min, global_y_max, ImGuiCond_Always);
    //            }
    //            // 绘制所有通道
    //            {
    //                std::shared_lock<std::shared_mutex> lock(mtx);
    //                for (size_t i = 0; i < channels.size(); i++)
    //                {
    //                    const auto &channel = channels[i];
    //                    if (channel_plot_data.count(channel) > 0)
    //                    {
    //                        auto &[ts, vals] = channel_plot_data[channel];
    //                        if (!ts.empty() && !vals.empty() && ts.size() == vals.size())
    //                        {
    //                            ImVec4 color = ImPlot::GetColormapColor(int(i));
    //                            ImPlot::SetNextLineStyle(color, 2.0f);
    //                            ImPlot::PlotLine(channel.c_str(), ts.data(), vals.data(), (int)ts.size());
    //                            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 2.0f, color, -1.0f, color);
    //                            ImPlot::PlotScatter(channel.c_str(), ts.data(), vals.data(), (int)ts.size());
    //                        }
    //                    }
    //                }
    //            }
    //            ImPlot::EndPlot();
    //        }
    //
    //        ImGui::End();
    //    }

//    void handle(const zcm::ReceiveBuffer *buffer, const std::string &channel, const timed_value *msg)
//    {
//        double timestamp = msg->timestamp;
//
//        // 聚合每一帧
//        frame_buffer[timestamp][channel] = msg->value;
//
//        // 如果这一帧的所有通道都收到了
//        if (frame_buffer[timestamp].size() == msg->cnt)
//        {
//            for (const auto &[ch, val] : frame_buffer[timestamp])
//            {
//                channel_data[ch].first.push_back(timestamp);
//                channel_data[ch].second.push_back(val);
//                if (channel_data[ch].first.size() > MAX_CACHE_SIZE)
//                {
//                    channel_data[ch].first.pop_front();
//                    channel_data[ch].second.pop_front();
//                }
//
//                // 更新绘图数据
//                auto timestamps = std::vector<double>(channel_data[ch].first.begin(), channel_data[ch].first.end());
//                auto values = std::vector<double>(channel_data[ch].second.begin(), channel_data[ch].second.end());
//                {
//                    std::lock_guard<std::shared_mutex> lck(mtx);
//                    channel_plot_data[ch].first = timestamps;
//                    channel_plot_data[ch].second = values;
//                }
//            }
//            frame_buffer.erase(timestamp); // 清理
//        }
//    }

    void handle(const zcm::ReceiveBuffer *buffer, const std::string &channel, const all_timed_value *msg)
    {
//        CLOG_INFO << "msg->cnt: " << msg->cnt;
        static bool initialized{false};
        for (const auto &item : msg->channels)
        {
//            CLOG_INFO << item.name;
            channel_data[item.name].first.push_back(item.timestamp);
            channel_data[item.name].second.push_back(item.value);
            if (channel_data[item.name].first.size() > MAX_CACHE_SIZE)
            {
                channel_data[item.name].first.pop_front();
                channel_data[item.name].second.pop_front();
            }
            // 更新绘图数据
            auto timestamps = std::vector<double>(channel_data[item.name].first.begin(), channel_data[item.name].first.end());
            auto values = std::vector<double>(channel_data[item.name].second.begin(), channel_data[item.name].second.end());
            {
                std::lock_guard<std::shared_mutex> lck(mtx);
                channel_plot_data[item.name].first = timestamps;
                channel_plot_data[item.name].second = values;
            }

            if (!initialized || msg->channel_updated)
            {
                for (auto &[key, value] : plot_channels) // old channel will keep
                {
                    if (contains(key, "*"))
                    {
                        auto lhd = split_last(key, '*').first;
                        if (contains(item.name, lhd))
                        {
                            plot_channels[key].push_back(item.name);
                        }
                    }
                    if (contains(key, "-"))
                    {
                        for (auto &element : expand_range_expression(key))
                        {
                            if (item.name == element)
                            {
                                plot_channels[key].push_back(item.name);
                            }
                        }
                    }
                }
            }
        }
        if (!initialized)
        {
            initialized = true;
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
    //        auto timestamps = std::vector<double>(channel_data[channel].first.begin(),
    //        channel_data[channel].first.end()); auto values =
    //        std::vector<double>(channel_data[channel].second.begin(), channel_data[channel].second.end());
    //        {
    //            std::lock_guard<std::shared_mutex> lck(mtx);
    //            channel_plot_data[channel].first = timestamps;
    //            channel_plot_data[channel].second = values;
    //        }
    //    }

    //    void handle(const zcm::ReceiveBuffer *buffer, const std::string &channel, const timed_value *msg)
    //    {
    //        channel_data[channel].first.push_back(msg->timestamp);
    //        channel_data[channel].second.push_back(msg->value);
    //        {
    //            std::lock_guard<std::shared_mutex> lck(mtx);
    //            channel_plot_data[channel].first = channel_data[channel].first.data();
    //            channel_plot_data[channel].second = channel_data[channel].second.data();
    //        }
    //    }

//    void new_channel(const zcm::ReceiveBuffer *buffer, const std::string &channel, const data_channel *msg)
//    {
//        std::lock_guard<std::shared_mutex> lck(mtx);
//        zcm->pause();
//        zcm->subscribe(msg->channel, &Handler::handle, this);
//        zcm->resume();
//        all_channels.insert(msg->channel);
//        for (auto &[key, value] : plot_channels)
//        {
//            if (contains(key, "*"))
//            {
//                auto lhd = split_last(key, '*').first;
//                if (contains(msg->channel, lhd))
//                {
//                    plot_channels[key].push_back(msg->channel);
//                }
//            }
//            if (contains(key, "-"))
//            {
//                for (auto &element : expand_range_expression(key))
//                {
//                    if (msg->channel == element)
//                    {
////                        CLOG_INFO << "element: " << element;
//                        plot_channels[key].push_back(msg->channel);
//                    }
//                }
//            }
//        }
////        CLOG_INFO << msg->channel << ", " << msg->cnt;
//    }

//    void channels_rep(const zcm::ReceiveBuffer *buffer, const std::string &channel, const data_fields *msg)
//    {
//        std::lock_guard<std::shared_mutex> lck(mtx);
//        for (auto &field : msg->channels)
//        {
//            zcm->pause();
//            zcm->subscribe(field, &Handler::handle, this);
//            zcm->resume();
//            all_channels.insert(field);
//            for (auto &[key, value] : plot_channels)
//            {
//                if (contains(key, "*"))
//                {
//                    auto lhd = split_last(key, '*').first;
//                    if (contains(field, lhd))
//                    {
//                        plot_channels[key].push_back(field);
//                    }
//                }
//                if (contains(key, "-"))
//                {
//                    for (auto &element : expand_range_expression(key))
//                    {
////                        CLOG_INFO << "element: " << element;
//                        if (field == element)
//                        {
//                            plot_channels[key].push_back(field);
//                        }
//                    }
//                }
//            }
////            CLOG_INFO << field;
//        }
//    }

    std::unordered_map<double, std::unordered_map<std::string, double>> frame_buffer;
    std::unordered_map<std::string, std::pair<std::deque<double>, std::deque<double>>> channel_data;
    //    std::unordered_map<std::string, std::pair<DataBUffer, DataBUffer>> channel_data;
    std::unordered_map<std::string, std::pair<std::vector<double>, std::vector<double>>> channel_plot_data;
    std::unordered_set<std::string> all_channels;
    std::unordered_map<std::string, std::vector<std::string>> plot_channels;
    std::unordered_map<std::string, bool> plot_bool;
    //    std::unordered_map<std::string, PlotWindowState> plotWindowStates;
    std::unordered_map<std::string, PlotWindowState> plot_window_states;
//    std::unordered_map<std::string, PlotWindowState> state;
    //    std::vector<std::string> channels;

//    zcm::ZCM *zcm{nullptr};
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
    std::unique_ptr<zcm::ZCM> zcm;
    Handler h;
    std::thread th_zcm;
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
        h.plot_bool[channel] = true;
        h.plot_window_states[channel];
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
        //            //
        //            如何上面的ImGui::Begin()是在ImPlot()窗口中，那么鼠标右键点击是不是关联的就是此窗口中ImPlot的数据
        //            static std::vector<std::string> all_channels = {"camera/left", "camera/right", "lidar/scan",
        //            "imu/data"}; static std::unordered_map<std::string, bool> channel_enabled;
        //            ImGui::GetStyle().Colors[ImGuiCol_PopupBg] = ImVec4(0.2f, 0.2f, 0.2f, 0.4f);
        //            if (shiftHeld && ImGui::BeginPopupContextWindow("MyWindowPopup",
        //            ImGuiPopupFlags_MouseButtonRight))
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
    pt.plot("Pos*");
    pt.plot("Rot*");
    pt.plot("Joint/0-3");
    pt.plot("Pos/*");
    while (!pt.shouldClose())
    {
        pt.render();
    }
    return 0;
}
