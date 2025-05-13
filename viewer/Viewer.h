//
// Created by liuqiang on 25-5-9.
//

#ifndef MUJOCO_VIEWER_H
#define MUJOCO_VIEWER_H

#include <GLFW/glfw3.h>
#include <Eigen/Geometry>
#include <mujoco/mujoco.h>
#include <imgui.h>
#include <implot.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <iostream>
#include <stdexcept>
#include <any>
#include <mutex>
#include <atomic>
#include <ImGuizmo.h>
#include <unordered_set>
#include "Logging.h"
#include "Types.h"
#include <data_tamer/data_tamer.hpp>
#include <data_tamer/sinks/plot_sink.hpp>

enum CameraMode
{
    FREE_VIEW = 0,
    FIXED_TOP,
    FIXED_BOTTOM,
    FIXED_LEFT,
    FIXED_RIGHT,
    FIXED_FRONT,
    FIXED_BACK
};

class Viewer
{
public:
    mjModel *model{nullptr};
    mjData *data{nullptr};
    mjvCamera cam{};
    mjvOption opt{};
    mjvScene scn{};
    mjrContext con{};

public:
    Viewer(mjModel *m, mjData *d, std::string windowTitle = "Untitled");

    ~Viewer();

    bool init();

    void render();

    bool shouldClose() const
    {
        return glfwWindowShouldClose(window);
    }

    Eigen::Vector3d getBodyPosition(const std::string &bodyName) const;

    Eigen::Matrix4d getBodyPose(const std::string &bodyName) const;

    bool setJointValue(const std::vector<int> &jointIds, const Eigen::VectorXd &q) const;

    int getSensorId(const std::string &sensorName) const;

    int getSiteId(const std::string &siteName) const;

    int getGeomId(const std::string &geomName) const;

    int getMeshId(const std::string &meshName) const;

    int getBodyId(const std::string &bodyName) const;

    int getJointId(const std::string &jointName) const;

    bool getJointId(const std::string &jointName, int &jointId) const;

    bool getJointIds(const std::vector<std::string> &jointNames, std::vector<int> &jointIds) const;

    std::vector<int> getJointIds(const std::vector<std::string> &jointNames) const;

    int getActuatorId(const std::string &actuatorName) const;

    std::vector<int> getActuatorIds(const std::vector<std::string> &actuatorNames) const;

    bool getActuatorIds(const std::vector<std::string> &actuatorNames, std::vector<int> &actuatorIds) const;

    std::vector<int> jointNameToActuatorIds(const std::string &jointName) const;

    std::vector<int> jointNamesToActuatorIds(const std::vector<std::string> &jointNames) const;

    bool setActuatorCtrlCmd(int actuatorId, double ctrlValue) const;

    bool setActuatorCtrlCmd(const std::vector<std::string> &actuatorNames, const Eigen::VectorXd &ctrlValues) const;

    bool setActuatorCtrlCmd(const std::string &actuatorName, double ctrlValue) const;

    bool setActuatorCtrlCmd(const std::vector<int> &actuatorIds, const Eigen::VectorXd &ctrlValues) const;

    bool setJointCtrlCmd(const std::string &jointName, const Eigen::VectorXd &ctrlValue) const;

    bool setJointsCtrlCmd(const std::vector<std::string> &jointNames, const Eigen::VectorXd &ctrlValues) const;

    bool setJointsCtrlCmd(const std::vector<int> &jointIds, const Eigen::VectorXd &q) const;

    bool setBodyVisible(const std::string &bodyName, bool visible);

    bool setBodyVisible(int bodyId, bool visible);

    template <typename Func, typename... Args>
    void addFunction(Func &&f, Args &&...args)
    {
        auto boundFunc = [f = std::forward<Func>(f), args = std::make_tuple(std::forward<Args>(args)...)]() mutable
        {
            std::apply(f, args);
        };
        funcs.emplace_back(std::move(boundFunc));
    }

    template <typename Func>
    void addFunction(Func &&f)
    {
        funcs.emplace_back(std::forward<Func>(f));
    }

    void syncJointState();

    void showJointSliders(std::mutex &mtx);

    void showMocapGizmo(const std::string &mocapBodyName);

    void showGizmo(const mjrRect &viewport, float view[16], float proj[16]);

    void plotChannelData(const std::string &channelName, const std::shared_ptr<DataTamer::PlotSink>& sink) const;

    void drawBodyFrame(const std::string &bodyName);

protected:
    static void mouseClickCallback(GLFWwindow *win, int button, int action, int mods);

    static void mouseMoveCallback(GLFWwindow *win, double xPos, double yPos);

    static void mouseScrollCallback(GLFWwindow *win, double xOffset, double yOffset);

    static void keyboardKeyDownCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

    void mouseClick(GLFWwindow *win, int button, int action, int mods);

    void mouseMove(GLFWwindow *win, double xPos, double yPos);

    void mouseScroll(GLFWwindow *win, double xOffset, double yOffset);

    void switchCamera(CameraMode mode);

    void keyboard(GLFWwindow *win, int key, int scancode, int action, int mods);

    static int getFixedCameraId(CameraMode mode);

    void getMocapPose(int bodyId, float mat[9]) const;

    void setMocapPose(int bodyId, const float mat[9]) const;

    void showBodyTree(int bodyId);

    void showBodyTreeView();

    void setBodyVisibilityRecursively(int bodyId, bool visible);

    void hideGeomsById(const std::unordered_set<int> &geomIdsToRemove);

    void hideGeomsByVisibleMask(const std::vector<char> &geomVisibleMask);

    void showBodyFrame(const std::vector<mjvGeom> &geoms);

    void hideGeomById(int geomId);

    void drawBodyFrame(std::vector<mjvGeom> &geoms, int bodyId, double scale = 0.2) const;

    static void drawFrame(std::vector<mjvGeom> &geoms, const Vec3d &pos, const Mat3d &rot, double scale = 0.2);

    static void drawArrow(std::vector<mjvGeom> &geoms, const Vec3d &from, const Vec3d &to,
                          double shaft_diam, double head_diam, double head_len, const float rgba[4]);

private:
    std::string title;
    std::vector<std::function<void()>> funcs;
    GLFWwindow *window{nullptr};
    bool button_left{false};
    bool button_middle{false};
    bool button_right{false};
    double last_x{0};
    double last_y{0};
    std::vector<mjvGeom> vGeoms;
    int selected_body{-1};
    CameraMode current_mode = FREE_VIEW;
    mjtNum pivot_point[3]{};
    std::unordered_set<int> geomIds; // collect geoms to hide
    std::vector<std::vector<int>> bodyChildren;
    std::vector<char> bodyVisible; // default all are visible
    std::vector<bool> bodyExpanded;
    bool globalExpand{true};
    std::vector<mjtNum> q, lower, upper; // store joint values from imgui slider
    std::vector<int> mocapGizmos;
    std::vector<int> drawFrameBodyId;
};

#endif // MUJOCO_VIEWER_H
