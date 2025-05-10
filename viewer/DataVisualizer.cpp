#include "data_tamer/data_tamer.hpp"
#include "data_tamer/sinks/mcap_sink.hpp"
#include <mcap/reader.hpp>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "implot.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath>

// 模拟数据记录
void recordData()
{
    auto mcap_sink = std::make_shared<DataTamer::MCAPSink>("test.mcap");
    DataTamer::ChannelsRegistry::Global().addDefaultSink(mcap_sink);

    auto channel = DataTamer::ChannelsRegistry::Global().getChannel("channel");
    double value = 0.0;
    channel->registerValue("value", &value);

    for (int i = 0; i < 1000; ++i)
    {
        value = std::sin(i * 0.01);
        channel->takeSnapshot();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// 从MCAP文件读取数据
std::vector<double> readDataFromMCAP()
{
    std::vector<double> data;
    mcap::McapReader reader;
    auto status = reader.open("test.mcap");
    if (!status.ok())
    {
        std::cerr << "Failed to open MCAP file: " << status.message << std::endl;
        return data;
    }
    std::unordered_map<mcap::SchemaId, size_t> schema_id_to_hash;
    for (const auto &[schemaId, mcapSchema] : reader.schemas())
    {
        std::string schema_text(reinterpret_cast<const char *>(mcapSchema->data.data()), mcapSchema->data.size());
        // 这里可以根据具体的 schema 解析数据
        // 假设数据是一个简单的 double 类型
        for (const auto &[channelId, channel] : reader.channels())
        {
            if (channel->schemaId == schemaId)
            {
                // 使用 readMessages 迭代器遍历消息
                for (const auto &msg : reader.readMessages())
                {
                    if (msg.channel->id == channelId)
                    {
                        // 这里简单假设数据是一个 double 类型
                        double value = *reinterpret_cast<const double *>(msg.message.data);
                        data.push_back(value);
                    }
                }
            }
        }
    }
    reader.close();
    return data;
}

// 可视化数据
void visualizeData(const std::vector<double> &data)
{
    if (!glfwInit())
    {
        return;
    }

    GLFWwindow *window = glfwCreateWindow(800, 600, "ImPlot Data Visualization", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Data Visualization");
        if (ImPlot::BeginPlot("Data Plot", ImVec2(-1, -1)))
        {
            ImPlot::PlotLine("Data", data.data(), data.size());
            ImPlot::EndPlot();
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

int main()
{
    recordData();
    auto data = readDataFromMCAP();
    visualizeData(data);
    return 0;
}


#include "data_tamer/data_tamer.hpp"
#include "data_tamer/sinks/mcap_sink.hpp"
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "implot.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

// 自定义的 DataSink，用于存储数据以便实时可视化
class RealTimeVisualizationSink : public DataTamer::DataSinkBase {
public:
    std::unordered_map<std::string, std::vector<double>> data_storage;

    void addChannel(std::string const& name, DataTamer::Schema const& schema) override {
        // 这里可以处理通道添加逻辑
    }

    bool storeSnapshot(const DataTamer::Snapshot& snapshot) override {
        for (const auto& [name, value] : snapshot.values) {
            if (auto num_value = std::get_if<double>(&value)) {
                data_storage[snapshot.channel_name].push_back(*num_value);
            }
        }
        return true;
    }
};

// 模拟数据生成和记录
void recordData(std::shared_ptr<DataTamer::MCAPSink> mcap_sink, std::shared_ptr<RealTimeVisualizationSink> vis_sink) {
    auto channel = DataTamer::ChannelsRegistry::Global().getChannel("chan");
    channel->addDataSink(mcap_sink);
    channel->addDataSink(vis_sink);

    double value = 0.0;
    channel->registerValue("value", &value);

    for (int i = 0; i < 1000; ++i) {
        value = std::sin(i * 0.01);
        channel->takeSnapshot();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// 可视化数据
void visualizeData(std::shared_ptr<RealTimeVisualizationSink> vis_sink) {
    if (!glfwInit()) {
        return;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "ImPlot Data Visualization", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Data Visualization");
        for (const auto& [channel_name, data] : vis_sink->data_storage) {
            if (ImPlot::BeginPlot(channel_name.c_str(), ImVec2(-1, -1))) {
                ImPlot::PlotLine(channel_name.c_str(), data.data(), data.size());
                ImPlot::EndPlot();
            }
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

int main() {
    auto mcap_sink = std::make_shared<DataTamer::MCAPSink>("test.mcap");
    auto vis_sink = std::make_shared<RealTimeVisualizationSink>();

    std::thread record_thread(recordData, mcap_sink, vis_sink);
    visualizeData(vis_sink);

    record_thread.join();
    return 0;
}

// 模拟数据生成和记录
void recordData(std::shared_ptr<DataTamer::MCAPSink> mcap_sink) {
    auto channel = DataTamer::ChannelsRegistry::Global().getChannel("chan");
    channel->addDataSink(mcap_sink);

    double value = 0.0;
    auto id = channel->registerValue("value", &value);

    // 设置快照回调
    channel->setSnapshotCallback(snapshotCallback);

    for (int i = 0; i < 1000; ++i) {
        value = std::sin(i * 0.01);
        channel->takeSnapshot();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}