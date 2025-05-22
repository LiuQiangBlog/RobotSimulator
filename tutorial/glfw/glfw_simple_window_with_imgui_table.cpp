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

        if (ImGui::CollapsingHeader("Window options", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::BeginTable("split", 3))
            {
                ImGui::TableNextColumn(); ImGui::Checkbox("No titlebar", &no_titlebar);
                ImGui::TableNextColumn(); ImGui::Checkbox("No scrollbar", &no_scrollbar);
                ImGui::TableNextColumn(); ImGui::Checkbox("No menu", &no_menu);
                ImGui::TableNextColumn(); ImGui::Checkbox("No move", &no_move);
                ImGui::TableNextColumn(); ImGui::Checkbox("No resize", &no_resize);
                ImGui::TableNextColumn(); ImGui::Checkbox("No collapse", &no_collapse);
                ImGui::TableNextColumn(); ImGui::Checkbox("No close", &no_close);
                ImGui::TableNextColumn(); ImGui::Checkbox("No nav", &no_nav);
                ImGui::TableNextColumn(); ImGui::Checkbox("No background", &no_background);
                ImGui::TableNextColumn(); ImGui::Checkbox("No bring to front", &no_bring_to_front);
                ImGui::TableNextColumn(); ImGui::Checkbox("No docking", &no_docking);
                ImGui::TableNextColumn(); ImGui::Checkbox("Unsaved document", &unsaved_document);
                ImGui::EndTable();
            }
        }

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