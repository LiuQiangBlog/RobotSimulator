//
// Created by liuqiang on 25-5-18.
//
#include <imgui.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <mutex>
#include <nvml.h> // NVIDIA Management Library

// 用于存储系统监控数据的结构体
struct SystemMonitorData
{
    // CPU数据
    std::vector<float> cpu_percentages; // 每个核心的使用率
    float total_cpu_percentage;         // 总CPU使用率

    // 内存数据
    unsigned long total_ram; // 总内存 (KB)
    unsigned long used_ram;  // 已用内存 (KB)
    unsigned long free_ram;  // 空闲内存 (KB)

    // GPU数据
    bool gpu_available;                  // 是否有可用GPU
    std::string gpu_name;                // GPU名称
    int gpu_utilization;                 // GPU使用率 (%)
    unsigned long long gpu_memory_total; // 总显存 (MB)
    unsigned long long gpu_memory_used;  // 已用显存 (MB)

    // 数据锁
    std::mutex data_mutex;
};

SystemMonitorData system_data;

// 从/proc/stat读取CPU使用率
void read_cpu_usage()
{
    std::ifstream stat_file("/proc/stat");
    std::string line;

    // 读取总CPU使用率
    if (std::getline(stat_file, line))
    {
        std::istringstream ss(line);
        std::string cpu_label;
        ss >> cpu_label;

        std::vector<unsigned long long> times;
        unsigned long long time;
        while (ss >> time)
        {
            times.push_back(time);
        }

        if (times.size() >= 4)
        {
            static unsigned long long prev_idle = 0;
            static unsigned long long prev_total = 0;

            unsigned long long idle = times[3]; // idle time
            unsigned long long total = 0;
            for (auto t : times)
            {
                total += t;
            }

            unsigned long long idle_delta = idle - prev_idle;
            unsigned long long total_delta = total - prev_total;

            system_data.total_cpu_percentage = 100.0f * (1.0f - (idle_delta / (float)total_delta));

            prev_idle = idle;
            prev_total = total;
        }
    }

    // 读取每个CPU核心的使用率
    system_data.cpu_percentages.clear();
    int core_id = 0;
    while (std::getline(stat_file, line))
    {
        if (line.substr(0, 3) != "cpu")
            break;

        std::istringstream ss(line);
        std::string cpu_label;
        ss >> cpu_label;

        std::vector<unsigned long long> times;
        unsigned long long time;
        while (ss >> time)
        {
            times.push_back(time);
        }

        if (times.size() >= 4)
        {
            static std::vector<unsigned long long> prev_core_idle;
            static std::vector<unsigned long long> prev_core_total;

            if (core_id >= prev_core_idle.size())
            {
                prev_core_idle.resize(core_id + 1, 0);
                prev_core_total.resize(core_id + 1, 0);
            }

            unsigned long long idle = times[3];
            unsigned long long total = 0;
            for (auto t : times)
            {
                total += t;
            }

            unsigned long long idle_delta = idle - prev_core_idle[core_id];
            unsigned long long total_delta = total - prev_core_total[core_id];

            float core_percentage = 100.0f * (1.0f - (idle_delta / (float)total_delta));
            system_data.cpu_percentages.push_back(core_percentage);

            prev_core_idle[core_id] = idle;
            prev_core_total[core_id] = total;
            core_id++;
        }
    }
}

// 从/proc/meminfo读取内存使用情况
void read_memory_usage()
{
    std::ifstream meminfo_file("/proc/meminfo");
    std::string line;

    while (std::getline(meminfo_file, line))
    {
        if (line.find("MemTotal:") == 0)
        {
            std::istringstream ss(line.substr(9));
            ss >> system_data.total_ram;
        }
        else if (line.find("MemFree:") == 0)
        {
            std::istringstream ss(line.substr(9));
            ss >> system_data.free_ram;
        }
        else if (line.find("MemAvailable:") == 0)
        {
            // 忽略，使用更准确的计算
        }
        else if (line.find("Buffers:") == 0)
        {
            // 忽略，简化计算
        }
        else if (line.find("Cached:") == 0)
        {
            // 忽略，简化计算
        }
    }

    // 计算已用内存
    system_data.used_ram = system_data.total_ram - system_data.free_ram;
}

// 初始化NVML并读取GPU信息
bool init_nvml()
{
    nvmlReturn_t result = nvmlInit();
    if (result != NVML_SUCCESS)
    {
        printf("Failed to initialize NVML: %s\n", nvmlErrorString(result));
        return false;
    }

    unsigned int device_count;
    result = nvmlDeviceGetCount(&device_count);
    if (result != NVML_SUCCESS || device_count == 0)
    {
        printf("Failed to get GPU count or no GPUs found: %s\n", nvmlErrorString(result));
        nvmlShutdown();
        return false;
    }

    system_data.gpu_available = true;

    // 获取第一个GPU信息
    nvmlDevice_t device;
    result = nvmlDeviceGetHandleByIndex(0, &device);
    if (result != NVML_SUCCESS)
    {
        printf("Failed to get GPU handle: %s\n", nvmlErrorString(result));
        nvmlShutdown();
        return false;
    }

    char name[64];
    result = nvmlDeviceGetName(device, name, sizeof(name));
    if (result == NVML_SUCCESS)
    {
        system_data.gpu_name = name;
    }

    return true;
}

// 读取GPU使用情况
void read_gpu_usage()
{
    if (!system_data.gpu_available)
        return;

    nvmlDevice_t device;
    nvmlReturn_t result = nvmlDeviceGetHandleByIndex(0, &device);
    if (result != NVML_SUCCESS)
        return;

    // 获取GPU利用率
    nvmlUtilization_t utilization;
    result = nvmlDeviceGetUtilizationRates(device, &utilization);
    if (result == NVML_SUCCESS)
    {
        system_data.gpu_utilization = utilization.gpu;
    }

    // 获取显存使用情况
    nvmlMemory_t memory;
    result = nvmlDeviceGetMemoryInfo(device, &memory);
    if (result == NVML_SUCCESS)
    {
        system_data.gpu_memory_total = memory.total / (1024 * 1024); // 转换为MB
        system_data.gpu_memory_used = memory.used / (1024 * 1024);   // 转换为MB
    }
}

// 系统监控数据更新线程
void monitor_thread()
{
    bool nvml_initialized = init_nvml();
    system_data.gpu_available = false;

    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(system_data.data_mutex);
            read_cpu_usage();
            read_memory_usage();
            if (nvml_initialized)
            {
                read_gpu_usage();
            }
        }

        // 每秒更新一次
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// 绘制CPU使用率图表
void draw_cpu_usage()
{
    ImGui::Text("CPU Usage");
    ImGui::Separator();

    // 总CPU使用率
    ImGui::Text("Total CPU: %.1f%%", system_data.total_cpu_percentage);
    ImGui::ProgressBar(system_data.total_cpu_percentage / 100.0f, ImVec2(-1, 20));

    // 每个核心的使用率
    for (size_t i = 0; i < system_data.cpu_percentages.size(); i++)
    {
        ImGui::Text("CPU Core %zu: %.1f%%", i, system_data.cpu_percentages[i]);
        ImGui::ProgressBar(system_data.cpu_percentages[i] / 100.0f, ImVec2(-1, 10));
    }
}

// 绘制内存使用率图表
void draw_memory_usage()
{
    ImGui::Text("Memory Usage");
    ImGui::Separator();

    float used_percentage = (float)system_data.used_ram / (float)system_data.total_ram * 100.0f;

    ImGui::Text("Total: %.2f GB", system_data.total_ram / (1024.0f * 1024.0f));
    ImGui::Text("Used: %.2f GB (%.1f%%)", system_data.used_ram / (1024.0f * 1024.0f), used_percentage);
    ImGui::Text("Free: %.2f GB", system_data.free_ram / (1024.0f * 1024.0f));

    ImGui::ProgressBar(used_percentage / 100.0f, ImVec2(-1, 20));
}

// 绘制GPU使用率图表
void draw_gpu_usage()
{
    if (!system_data.gpu_available)
    {
        ImGui::Text("GPU not available or NVIDIA driver not installed");
        return;
    }

    ImGui::Text("GPU Usage: %s", system_data.gpu_name.c_str());
    ImGui::Separator();

    ImGui::Text("GPU Utilization: %d%%", system_data.gpu_utilization);
    ImGui::ProgressBar(system_data.gpu_utilization / 100.0f, ImVec2(-1, 20));

    float memory_percentage = (float)system_data.gpu_memory_used / (float)system_data.gpu_memory_total * 100.0f;
    ImGui::Text("GPU Memory: %llu MB / %llu MB (%.1f%%)", system_data.gpu_memory_used, system_data.gpu_memory_total,
                memory_percentage);
    ImGui::ProgressBar(memory_percentage / 100.0f, ImVec2(-1, 20));
}

int main(int, char **)
{
    // 初始化GLFW
    if (!glfwInit())
    {
        return 1;
    }

    // 创建窗口
    GLFWwindow *window = glfwCreateWindow(800, 600, "System Monitor", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // 启用垂直同步

    // 初始化ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    // 设置ImGui样式
    ImGui::StyleColorsDark();

    // 初始化平台/渲染后端
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 启动监控线程
    std::thread monitor(monitor_thread);
    monitor.detach();

    // 主循环
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // 开始ImGui帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 创建主窗口
        ImGui::Begin("System Monitor", NULL, ImGuiWindowFlags_AlwaysAutoResize);

        // 绘制系统监控信息
        draw_cpu_usage();
        ImGui::Spacing();
        draw_memory_usage();
        ImGui::Spacing();
        draw_gpu_usage();

        ImGui::End();

        // 渲染
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // 清理资源
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    // 关闭NVML（如果已初始化）
    if (system_data.gpu_available)
    {
        nvmlShutdown();
    }

    return 0;
}