//
// Created by liuqiang on 25-5-21.
//
#include <GLFW/glfw3.h>
#include <imgui.h>
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
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // hide window first
    GLFWwindow *window = glfwCreateWindow(1280, 720, "Hello ImGui", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    centerWindow(window); // move window to center
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

    bool show_demo_window = true;
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

//        ImGui::ShowDemoWindow(&show_demo_window);

//        if (ImGui::TreeNodeEx("Capture/Logging", ImGuiTreeNodeFlags_DefaultOpen))
//        {
//            ImGui::SameLine();
//            ImGui::TextDisabled("(?)");
//            if (ImGui::BeginItemTooltip())
//            {
//                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
//                ImGui::TextUnformatted("Instruct backend to not alter mouse cursor shape and visibility.");
//                ImGui::PopTextWrapPos();
//                ImGui::EndTooltip();
//            }
//            ImGui::LogButtons();
//            if (ImGui::Button("Copy \"Hello, world!\" to clipboard"))
//            {
//                ImGui::LogToClipboard();
//                ImGui::LogText("Hello, world!");
//                ImGui::LogFinish();
//            }
//            ImGui::TreePop();
//        }



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