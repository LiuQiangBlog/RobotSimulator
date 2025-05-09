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
#include <ImGuizmo.h>
#include <unordered_set>
#include "Logging.h"

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

    bool getJointIds(const std::vector<std::string> &jointNames, std::vector<int> &jointIds);

    bool getActuatorIds(const std::vector<std::string> &actuatorNames, std::vector<int> &actuatorIds);

    bool setJointValue(const std::vector<int> &ids, const Eigen::VectorXd &q);

    int getSensorId(const std::string &sensorName) const
    {
        int sensorId = mj_name2id(model, mjOBJ_SENSOR, sensorName.c_str());
        if (sensorId == -1)
        {
            CLOG_ERROR << "Failed to find sensor: " << sensorName;
        }
        return sensorId;
    }

    int getSiteId(const std::string &siteName) const
    {
        int siteId = mj_name2id(model, mjOBJ_SITE, siteName.c_str());
        if (siteId == -1)
        {
            CLOG_ERROR << "Failed to find site: " << siteName;
        }
        return siteId;
    }

    int getGeomId(const std::string &geomName) const
    {
        int geomId = mj_name2id(model, mjOBJ_GEOM, geomName.c_str());
        if (geomId == -1)
        {
            CLOG_ERROR << "Failed to find geom: " << geomName;
        }
        return geomId;
    }

    int getMeshId(const std::string &meshName) const
    {
        int meshId = mj_name2id(model, mjOBJ_MESH, meshName.c_str());
        if (meshId == -1)
        {
            CLOG_ERROR << "Failed to find mesh: " << meshName;
        }
        return meshId;
    }

    int getBodyId(const std::string &bodyName) const
    {
        int bodyId = mj_name2id(model, mjOBJ_BODY, bodyName.c_str());
        if (bodyId == -1)
        {
            CLOG_ERROR << "Failed to find body: " << bodyName;
        }
        return bodyId;
    }

    int getJointId(const std::string &jointName) const
    {
        int jointId = mj_name2id(model, mjOBJ_JOINT, jointName.c_str());
        if (jointId == -1)
        {
            CLOG_ERROR << "Failed to find joint: " << jointName;
        }
        return jointId;
    }

    std::vector<int> getJointIds(const std::vector<std::string> &jointNames) const
    {
        std::vector<int> jointIds;
        for (const auto &jointName : jointNames)
        {
            int jointId = getJointId(jointName);
            if (jointId != -1)
            {
                jointIds.push_back(jointId);
            }
            else
            {
                return {};
            }
        }
        return jointIds;
    }

    int getActuatorId(const std::string &actuatorName) const
    {
        int actuatorId = mj_name2id(model, mjOBJ_ACTUATOR, actuatorName.c_str());
        if (actuatorId == -1)
        {
            CLOG_ERROR << "Failed to find actuator: " << actuatorName;
        }
        return actuatorId;
    }

    std::vector<int> getActuatorIds(const std::vector<std::string> &actuatorNames) const
    {
        std::vector<int> actuatorIds;
        for (const auto &name : actuatorNames)
        {
            int actuatorId = getActuatorId(name);
            if (actuatorId != -1)
            {
                actuatorIds.push_back(actuatorId);
            }
            else
            {
                return {};
            }
        }
        return actuatorIds;
    }

    std::vector<int> jointNameToActuatorIds(const std::string &jointName) const
    {
        int jointId = mj_name2id(model, mjOBJ_JOINT, jointName.c_str());
        if (jointId == -1)
        {
            CLOG_ERROR << "Failed to find joint: " << jointName;
            return {};
        }
        std::vector<int> actuatorIds;
        for (int j = 0; j < model->nu; ++j)
        {
            if (model->actuator_trnid[j * 2] == jointId && model->actuator_trntype[j] == mjTRN_JOINT)
            {
                actuatorIds.push_back(j);
            }
        }
        if (actuatorIds.empty())
        {
            CLOG_ERROR << "Failed to find actuator for joint id: " << jointId;
        }
        return actuatorIds;
    }

    std::vector<int> jointNamesToActuatorIds(const std::vector<std::string> &jointNames) const
    {
        std::vector<int> allActuatorIds;
        for (const auto &name : jointNames)
        {
            auto ids = jointNameToActuatorIds(name);
            if (ids.empty())
            {
                return {};
            }
            allActuatorIds.insert(allActuatorIds.end(), ids.begin(), ids.end());
        }
        return allActuatorIds;
    }

    bool setActuatorCtrlCmd(int actuatorId, double ctrlValue) const
    {
        if (actuatorId < 0 || actuatorId >= model->nu)
        {
            CLOG_ERROR << "Invalid actuator id: " << actuatorId;
            return false;
        }
        double ctrlMin = model->actuator_ctrlrange[actuatorId * 2];
        double ctrlMax = model->actuator_ctrlrange[actuatorId * 2 + 1];
        data->ctrl[actuatorId] = std::clamp(ctrlValue, ctrlMin, ctrlMax);
        return true;
    }

    bool setActuatorsCtrlCmd(const std::vector<std::string> &actuatorNames, const Eigen::VectorXd &ctrlValues) const
    {
        return setActuatorsCtrlCmd(getActuatorIds(actuatorNames), ctrlValues);
    }

    bool setActuatorCtrlCmd(const std::string &actuatorName, double ctrlValue) const
    {
        return setActuatorCtrlCmd(getActuatorId(actuatorName), ctrlValue);
    }

    bool setActuatorsCtrlCmd(const std::vector<int> &actuatorIds, const Eigen::VectorXd &ctrlValues) const
    {
        if (actuatorIds.size() != ctrlValues.size())
        {
            CLOG_ERROR << "actuatorIds.size() != ctrlValues.size()";
            return false;
        }
        for (size_t i = 0; i < actuatorIds.size(); ++i)
        {
            if (!setActuatorCtrlCmd(actuatorIds[i], ctrlValues[Eigen::Index(i)]))
            {
                return false;
            }
        }
        return true;
    }

    bool setJointCtrlCmd(const std::string &jointName, const Eigen::VectorXd &ctrlValue) const
    {
        auto actuatorIds = jointNameToActuatorIds(jointName);
        if (actuatorIds.empty())
        {
            return false;
        }
        if (actuatorIds.size() != static_cast<size_t>(ctrlValue.size()))
        {
            CLOG_ERROR << "actuatorIds.size() != static_cast<size_t>(ctrlValue.size())";
            return false;
        }
        return setActuatorsCtrlCmd(actuatorIds, ctrlValue);
    }

    bool setJointsCtrlCmd(const std::vector<std::string> &jointNames, const Eigen::VectorXd &ctrlValues) const
    {
        int currentIdx = 0;
        for (const auto &name : jointNames)
        {
            auto actuatorIds = jointNameToActuatorIds(name);
            size_t dim = actuatorIds.size();
            if (dim == 0 || currentIdx + dim > static_cast<size_t>(ctrlValues.size()))
            {
                return false;
            }
            Eigen::VectorXd jointCtrl = ctrlValues.segment(currentIdx, dim);
            if (!setActuatorsCtrlCmd(actuatorIds, jointCtrl))
            {
                return false;
            }
            currentIdx += (int)dim;
        }
        return true;
    }

    bool setJointsCtrlCmd(const std::vector<int> &jointIds, const Eigen::VectorXd &q) const
    {
        int requiredCtrlSize = 0;
        std::vector<std::vector<int>> jointActuators(jointIds.size());
        for (size_t i = 0; i < jointIds.size(); ++i)
        {
            int jointId = jointIds[i];
            if (jointId < 0 || jointId >= model->njnt)
            {
                CLOG_ERROR << "Invalid joint id: " << jointId;
                return false;
            }
            std::vector<int> actuatorIds;
            for (int j = 0; j < model->nu; ++j)
            {
                if (model->actuator_trnid[j * 2] == jointId && model->actuator_trntype[j] == mjTRN_JOINT)
                {
                    actuatorIds.push_back(j);
                }
            }
            if (actuatorIds.empty())
            {
                CLOG_ERROR << "Failed to find actuator for joint id: " << jointId;
                return false;
            }
            jointActuators[i] = std::move(actuatorIds);
            requiredCtrlSize += (int)jointActuators[i].size();
        }
        if (q.size() != requiredCtrlSize)
        {
            CLOG_ERROR << "Ctrl info size mismatch, need " << requiredCtrlSize << ", but provided " << q.size();
            return false;
        }
        int ctrl_idx = 0;
        for (const auto &actuatorIds : jointActuators)
        {
            for (int actuatorId : actuatorIds)
            {
                double ctrlValue = q[ctrl_idx++];
                double ctrlMin = model->actuator_ctrlrange[actuatorId * 2];
                double ctrlMax = model->actuator_ctrlrange[actuatorId * 2 + 1];
                data->ctrl[actuatorId] = std::clamp(ctrlValue, ctrlMin, ctrlMax);
            }
        }
        return true;
    }

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

    void getMocapPose(int bodyId, float mat[9]);

    void setMocapPose(int bodyId, const float mat[9]);

    void showBodyTree(int bodyId);

    void setBodyVisibilityRecursively(int bodyId, bool visible);

    void hideGeomsById(const std::unordered_set<int> &geomIdsToRemove);

    void hideGeomById(int geomId);

private:
    std::string title;
    std::vector<std::function<void()>> funcs;
    GLFWwindow *window{nullptr};
    bool button_left = false;
    bool button_middle = false;
    bool button_right = false;
    double last_x = 0;
    double last_y = 0;
    std::vector<mjvGeom> vGeoms;
    int selected_body = -1;
    CameraMode current_mode = FREE_VIEW;
    mjtNum pivot_point[3]{};
    std::unordered_set<int> geomIds; // collect geoms to hide
    std::vector<std::vector<int>> bodyChildren;
    std::vector<char> bodyVisible; // default all are visible
};

#endif // MUJOCO_VIEWER_H
