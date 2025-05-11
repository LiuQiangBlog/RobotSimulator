//
// Created by liuqiang on 25-5-9.
//

#include "Viewer.h"

Viewer::Viewer(mjModel *m, mjData *d, std::string windowTitle)
    : model(m), data(d), title(std::move(windowTitle))
{
    bodyVisible.resize(model->nbody, true);
    bodyChildren.resize(model->nbody);
    bodyExpanded.resize(model->nbody, true);
    for (int i = 1; i < model->nbody; ++i) // body 0 is world
    {
        int parent = model->body_parentid[i];
        bodyChildren[parent].push_back(i);
    }
    q.resize(model->njnt, 0);
    lower.resize(model->njnt, 0);
    upper.resize(model->njnt, 0);
    for (int jnt_id = 0; jnt_id < model->njnt; ++jnt_id)
    {
        auto type = (mjtJoint)model->jnt_type[jnt_id];
        if (type != mjJNT_FREE && type != mjJNT_BALL)
        {
            if (type == mjJNT_HINGE)
            {
                lower[jnt_id] = model->jnt_limited[jnt_id] ? model->jnt_range[jnt_id * 2] : -mjPI;
                upper[jnt_id] = model->jnt_limited[jnt_id] ? model->jnt_range[jnt_id * 2 + 1] : mjPI;
            }
            else
            {
                lower[jnt_id] = model->jnt_limited[jnt_id] ? model->jnt_range[jnt_id * 2] : -1.0;
                upper[jnt_id] = model->jnt_limited[jnt_id] ? model->jnt_range[jnt_id * 2 + 1] : 1.0;
            }
            q[jnt_id] = data->qpos[model->jnt_qposadr[jnt_id]];
        }
    }
}

Viewer::~Viewer()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    mjr_freeContext(&con);
    mjv_freeScene(&scn);
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Viewer::init()
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0); // or 3
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
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.FontGlobalScale = 1.2f;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    glfwSetWindowUserPointer(window, this); // 设置窗口用户指针
    glfwSetMouseButtonCallback(window, mouseClickCallback);
    glfwSetCursorPosCallback(window, mouseMoveCallback);
    glfwSetScrollCallback(window, mouseScrollCallback);
    glfwSetKeyCallback(window, keyboardKeyDownCallback);
    mjv_defaultCamera(&cam);
    cam.type = mjCAMERA_FREE;
    mjv_defaultOption(&opt);
    mjv_defaultScene(&scn);
    mjr_defaultContext(&con);
    mjv_makeScene(model, &scn, 2000);
    mjr_makeContext(model, &con, mjFONTSCALE_150);
    return true;
}

void Viewer::render()
{
    mjv_updateScene(model, data, &opt, nullptr, &cam, mjCAT_ALL, &scn);

    drawBodyFrame(vGeoms, getBodyId("table"), 0.5);
    hideGeomsById(geomIds);
    hideGeomsByVisibleMask(bodyVisible);
    showBodyFrame(vGeoms);

    mjrRect viewport = {0, 0, 0, 0};
    glfwGetFramebufferSize(window, &viewport.width, &viewport.height);
    mjr_render(viewport, &scn, &con);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // init gizmo
    ImGuizmo::BeginFrame();
    ImGuizmo::AllowAxisFlip(false);
    ImGuiIO &io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    float view[16], proj[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, view);
    glGetFloatv(GL_PROJECTION_MATRIX, proj);

    // call mjr_overlay must after getting view and proj
    mjr_overlay(mjFONT_NORMAL, mjGRID_TOPRIGHT, viewport,
                current_mode == FREE_VIEW      ? "Camera: FREE"
                : current_mode == FIXED_TOP    ? "Camera: TOP"
                : current_mode == FIXED_BOTTOM ? "Camera: BOTTOM"
                : current_mode == FIXED_LEFT   ? "Camera: LEFT"
                : current_mode == FIXED_RIGHT  ? "Camera: RIGHT"
                : current_mode == FIXED_FRONT  ? "Camera: FRONT"
                : current_mode == FIXED_BACK   ? "Camera: BACK" : "Camera: UNKNOWN", nullptr, &con);

    for (auto &func : funcs)
    {
        func();
    }

    showGizmo(viewport, view, proj);

    showBodyTreeView();

    // ImGui render
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glfwPollEvents(); // process events
}

Eigen::Vector3d Viewer::getBodyPosition(const std::string &bodyName) const
{
    auto bodyId = mj_name2id(model, mjOBJ_BODY, bodyName.c_str());
    if (bodyId != -1)
    {
        Eigen::Map<Eigen::Vector3d> pos(data->xpos + bodyId * 3, 3);
        return pos;
    }
    return Eigen::Vector3d::Zero();
}

Eigen::Matrix4d Viewer::getBodyPose(const std::string &bodyName) const
{
    auto bodyId = mj_name2id(model, mjOBJ_BODY, bodyName.c_str());
    Eigen::Matrix4d res;
    res.setIdentity();
    if (bodyId != -1)
    {
        Eigen::Map<Eigen::Vector3d> pos(data->xpos + bodyId * 3, 3);
        Eigen::Map<Eigen::Matrix<double, 3, 3, Eigen::RowMajor>> mat(data->xmat + bodyId * 9, 3, 3);
        res.block<3, 1>(0, 3) = pos;
        res.block<3, 3>(0, 0) = mat;
    }
    return res;
}

bool Viewer::setJointValue(const std::vector<int> &jointIds, const Eigen::VectorXd &q) const
{
    int i = 0;
    for (auto &id : jointIds)
    {
        int joint_type = model->jnt_type[id];
        int cnt;
        switch (joint_type)
        {
        case mjJNT_FREE:
            cnt = 7;
            break;
        case mjJNT_BALL:
            cnt = 4;
            break;
        case mjJNT_HINGE:
        case mjJNT_SLIDE:
            cnt = 1;
            break;
        default:
            CLOG_ERROR << "Unsupported joint type.";
            return false;
        }
        if (i + cnt > q.size())
        {
            CLOG_ERROR << "Joint value vector too short (need " << (i + cnt) << ", got " << q.size() << ")";
            return false;
        }
        for (int j = 0; j < cnt; ++j)
        {
            data->qpos[model->jnt_qposadr[id] + i] = q[i + j];
        }
        if (joint_type == mjJNT_BALL)
        {
            mju_normalize4(data->qpos + model->jnt_qposadr[id]);
        }
        else if (joint_type == mjJNT_FREE)
        {
            mju_normalize4(data->qpos + model->jnt_qposadr[id] + 3);
        }
        i += cnt;  // move to next joint
    }
    mj_forward(model, data);
    return true;
}

int Viewer::getSensorId(const std::string &sensorName) const
{
    int sensorId = mj_name2id(model, mjOBJ_SENSOR, sensorName.c_str());
    if (sensorId == -1)
    {
        CLOG_ERROR << "Failed to find sensor: " << sensorName;
    }
    return sensorId;
}

int Viewer::getSiteId(const std::string &siteName) const
{
    int siteId = mj_name2id(model, mjOBJ_SITE, siteName.c_str());
    if (siteId == -1)
    {
        CLOG_ERROR << "Failed to find site: " << siteName;
    }
    return siteId;
}

int Viewer::getGeomId(const std::string &geomName) const
{
    int geomId = mj_name2id(model, mjOBJ_GEOM, geomName.c_str());
    if (geomId == -1)
    {
        CLOG_ERROR << "Failed to find geom: " << geomName;
    }
    return geomId;
}

int Viewer::getMeshId(const std::string &meshName) const
{
    int meshId = mj_name2id(model, mjOBJ_MESH, meshName.c_str());
    if (meshId == -1)
    {
        CLOG_ERROR << "Failed to find mesh: " << meshName;
    }
    return meshId;
}

int Viewer::getBodyId(const std::string &bodyName) const
{
    int bodyId = mj_name2id(model, mjOBJ_BODY, bodyName.c_str());
    if (bodyId == -1)
    {
        CLOG_ERROR << "Failed to find body: " << bodyName;
    }
    return bodyId;
}

int Viewer::getJointId(const std::string &jointName) const
{
    int jointId = mj_name2id(model, mjOBJ_JOINT, jointName.c_str());
    if (jointId == -1)
    {
        CLOG_ERROR << "Failed to find joint: " << jointName;
    }
    return jointId;
}

bool Viewer::getJointId(const std::string &jointName, int &jointId) const
{
    jointId = mj_name2id(model, mjOBJ_JOINT, jointName.c_str());
    if (jointId == -1)
    {
        CLOG_ERROR << "Failed to find joint: " << jointName;
    }
    return jointId;
}

bool Viewer::getJointIds(const std::vector<std::string> &jointNames, std::vector<int> &jointIds) const
{
    for (auto &jointName : jointNames)
    {
        int jointId = mj_name2id(model, mjOBJ_JOINT, jointName.c_str());
        if (jointId == -1)
        {
            CLOG_ERROR << "Failed to find joint: " << jointName;
            return false;
        }
        jointIds.push_back(jointId);
    }
    return true;
}

std::vector<int> Viewer::getJointIds(const std::vector<std::string> &jointNames) const
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

int Viewer::getActuatorId(const std::string &actuatorName) const
{
    int actuatorId = mj_name2id(model, mjOBJ_ACTUATOR, actuatorName.c_str());
    if (actuatorId == -1)
    {
        CLOG_ERROR << "Failed to find actuator: " << actuatorName;
    }
    return actuatorId;
}

std::vector<int> Viewer::getActuatorIds(const std::vector<std::string> &actuatorNames) const
{
    std::vector<int> actuatorIds;
    for (const auto &actuatorName : actuatorNames)
    {
        int actuatorId = mj_name2id(model, mjOBJ_ACTUATOR, actuatorName.c_str());
        if (actuatorId == -1)
        {
            return {};
        }
        actuatorIds.push_back(actuatorId);
    }
    return actuatorIds;
}

bool Viewer::getActuatorIds(const std::vector<std::string> &actuatorNames, std::vector<int> &actuatorIds) const
{
    for (auto &actuatorName : actuatorNames)
    {
        int actuatorId = mj_name2id(model, mjOBJ_ACTUATOR, actuatorName.c_str());
        if (actuatorId == -1)
        {
            return false;
        }
        actuatorIds.push_back(actuatorId);
    }
    return true;
}

std::vector<int> Viewer::jointNameToActuatorIds(const std::string &jointName) const
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

std::vector<int> Viewer::jointNamesToActuatorIds(const std::vector<std::string> &jointNames) const
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

bool Viewer::setActuatorCtrlCmd(int actuatorId, double ctrlValue) const
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

bool Viewer::setActuatorCtrlCmd(const std::vector<std::string> &actuatorNames, const Eigen::VectorXd &ctrlValues) const
{
    return setActuatorCtrlCmd(getActuatorIds(actuatorNames), ctrlValues);
}

bool Viewer::setActuatorCtrlCmd(const std::string &actuatorName, double ctrlValue) const
{
    return setActuatorCtrlCmd(getActuatorId(actuatorName), ctrlValue);
}

bool Viewer::setActuatorCtrlCmd(const std::vector<int> &actuatorIds, const Eigen::VectorXd &ctrlValues) const
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

bool Viewer::setJointCtrlCmd(const std::string &jointName, const Eigen::VectorXd &ctrlValue) const
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
    return setActuatorCtrlCmd(actuatorIds, ctrlValue);
}

bool Viewer::setJointsCtrlCmd(const std::vector<std::string> &jointNames, const Eigen::VectorXd &ctrlValues) const
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
        if (!setActuatorCtrlCmd(actuatorIds, jointCtrl))
        {
            return false;
        }
        currentIdx += (int)dim;
    }
    return true;
}

bool Viewer::setJointsCtrlCmd(const std::vector<int> &jointIds, const Eigen::VectorXd &q) const
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

void Viewer::mouseClickCallback(GLFWwindow *win, int button, int action, int mods)
{
    auto *self = static_cast<Viewer *>(glfwGetWindowUserPointer(win));
    if (self)
    {
        //        self->mouse_button(win, button, action, mods);
        self->mouseClick(win, button, action, mods);
    }
}

void Viewer::mouseMoveCallback(GLFWwindow *win, double xPos, double yPos)
{
    auto *self = static_cast<Viewer *>(glfwGetWindowUserPointer(win));
    if (self)
    {
        self->mouseMove(win, xPos, yPos);
    }
}

void Viewer::mouseScrollCallback(GLFWwindow *win, double xOffset, double yOffset)
{
    auto *self = static_cast<Viewer *>(glfwGetWindowUserPointer(win));
    if (self)
    {
        self->mouseScroll(win, xOffset, yOffset);
    }
}

void Viewer::keyboardKeyDownCallback(GLFWwindow *win, int key, int scancode, int action, int mods)
{
    auto *self = static_cast<Viewer *>(glfwGetWindowUserPointer(win));
    if (self)
    {
        self->keyboard(win, key, scancode, action, mods);
    }
}

void Viewer::mouseClick(GLFWwindow *win, int button, int action, int mods)
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

void Viewer::mouseMove(GLFWwindow *win, double xPos, double yPos)
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

    double dx = xPos - last_x;
    double dy = yPos - last_y;
    last_x = xPos;
    last_y = yPos;

    int width, height;
    glfwGetWindowSize(win, &width, &height);

    int leftShiftKey = glfwGetKey(win, GLFW_KEY_LEFT_SHIFT);
    int rightShiftKey = glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT);
    bool modifyShift = (leftShiftKey == GLFW_PRESS || rightShiftKey == GLFW_PRESS);

    mjtMouse action;
    if (button_right)
    {
        action = modifyShift ? mjMOUSE_MOVE_H : mjMOUSE_MOVE_V;
    }
    else if (button_left)
    {
        action = modifyShift ? mjMOUSE_ROTATE_H : mjMOUSE_ROTATE_V;
    }
    else
    {
        action = mjMOUSE_ZOOM;
    }
    mjv_moveCamera(model, action, dx / height, dy / height, &scn, &cam);
}

void Viewer::mouseScroll(GLFWwindow *win, double xOffset, double yOffset)
{
    ImGui_ImplGlfw_ScrollCallback(win, xOffset, yOffset);
    if (ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }
    mjv_moveCamera(model, mjMOUSE_ZOOM, 0, -0.05 * yOffset, &scn, &cam);
}

void Viewer::switchCamera(CameraMode mode)
{
    current_mode = mode;
    if (mode == FREE_VIEW)
    {
        cam.type = mjCAMERA_FREE;
    }
    else
    {
        cam.type = mjCAMERA_FIXED;
        cam.fixedcamid = getFixedCameraId(mode);
    }
}

void Viewer::keyboard(GLFWwindow *win, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_F1)
        {
            switchCamera(FREE_VIEW);
        }
        else if (key == GLFW_KEY_F2)
        {
            switchCamera(FIXED_TOP);
        }
        else if (key == GLFW_KEY_F3)
        {
            switchCamera(FIXED_LEFT);
        }
        else if (key == GLFW_KEY_F4)
        {
            switchCamera(FIXED_RIGHT);
        }
    }
}

int Viewer::getFixedCameraId(CameraMode mode)
{
    switch (mode)
    {
    case FIXED_TOP:
        return 0;
    case FIXED_BOTTOM:
        return 1;
    case FIXED_LEFT:
        return 2;
    case FIXED_RIGHT:
        return 3;
    case FIXED_FRONT:
        return 4;
    case FIXED_BACK:
        return 5;
    default:
        return 0;
    }
}

void Viewer::getMocapPose(int bodyId, float *matrix) const
{
    int mocap_id = model->body_mocapid[bodyId];
    if (mocap_id < 0)
    {
        return;
    }
    const double *pos = data->mocap_pos + 3 * mocap_id;
    const double *quat = data->mocap_quat + 4 * mocap_id;

    double rot[9];
    mju_quat2Mat(rot, quat);

    // Column Major（OpenGL/ImGizmo Style）
    // X Axis
    matrix[0] = (float)rot[0];
    matrix[1] = (float)rot[3];
    matrix[2] = (float)rot[6];
    matrix[3] = 0.0f;
    // Y Axis
    matrix[4] = (float)rot[1];
    matrix[5] = (float)rot[4];
    matrix[6] = (float)rot[7];
    matrix[7] = 0.0f;
    // Z Axis
    matrix[8] = (float)rot[2];
    matrix[9] = (float)rot[5];
    matrix[10] = (float)rot[8];
    matrix[11] = 0.0f;
    // Position
    matrix[12] = (float)pos[0];
    matrix[13] = (float)pos[1];
    matrix[14] = (float)pos[2];
    matrix[15] = 1.0f;
}

void Viewer::setMocapPose(int bodyId, const float mat[9]) const
{
    int mocap_id = model->body_mocapid[bodyId];
    if (mocap_id < 0)
    {
        return;
    }
    data->mocap_pos[3 * mocap_id] = mat[12];
    data->mocap_pos[3 * mocap_id + 1] = mat[13];
    data->mocap_pos[3 * mocap_id + 2] = mat[14];
    double rot[9] = {mat[0], mat[4], mat[8], mat[1], mat[5], mat[9], mat[2], mat[6], mat[10]};
    mju_mat2Quat(data->mocap_quat + 4 * mocap_id, rot);
}

void Viewer::showBodyTree(int bodyId)
{
    const char *bodyName = model->names + model->name_bodyadr[bodyId];
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow;

    bool shouldBeOpen = globalExpand || bodyExpanded[bodyId];
    if (shouldBeOpen)
    {
        nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    bool nodeOpen = ImGui::TreeNodeEx(bodyName, nodeFlags);

    ImGui::SameLine();
    std::string id = "##visible" + std::to_string(bodyId);
    bool changed = ImGui::Checkbox(id.c_str(), reinterpret_cast<bool *>(&bodyVisible[bodyId]));
    if (changed)
    {
        setBodyVisibilityRecursively(bodyId, bodyVisible[bodyId]);
    }

    if (nodeOpen != bodyExpanded[bodyId])
    {
        bodyExpanded[bodyId] = nodeOpen;
        globalExpand = false;
    }

    if (nodeOpen)
    {
        for (int childId : bodyChildren[bodyId])
        {
            showBodyTree(childId);
        }
        ImGui::TreePop();
    }
}

void Viewer::showBodyTreeView()
{
    static bool open{true};
    if (open)
    {
        if (ImGui::Begin("TreeView", &open, ImGuiWindowFlags_NoCollapse))
        {
            if (ImGui::Button("Expand"))
            {
                globalExpand = true;
                for (int i = 0; i < model->nbody; ++i)
                {
                    bodyExpanded[i] = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Collapse"))
            {
                globalExpand = false;
                for (int i = 0; i < model->nbody; ++i)
                {
                    bodyExpanded[i] = false;
                }
            }
            int base_id = mj_name2id(model, mjOBJ_BODY, "base_Link");
            showBodyTree(base_id);
        }
        ImGui::End();
    }
}

void Viewer::syncJointState()
{
    for (int jnt_id = 0; jnt_id < model->njnt; ++jnt_id)
    {
        auto type = (mjtJoint)model->jnt_type[jnt_id];
        if (type != mjJNT_FREE && type != mjJNT_BALL)
        {
            data->qpos[model->jnt_qposadr[jnt_id]] = q[jnt_id];
        }
    }
}

void Viewer::showJointSliders(std::mutex &mtx)
{
    static bool open{true};
    if (!open)
    {
        return;
    }
    if (ImGui::Begin("SliderView", &open, ImGuiWindowFlags_MenuBar))
    {
        for (int jnt_id = 0; jnt_id < model->njnt; ++jnt_id)
        {
            auto type = (mjtJoint)model->jnt_type[jnt_id];
            if (type != mjJNT_FREE && type != mjJNT_BALL)
            {
                ImGui::SetNextItemWidth(400);
                auto label = mj_id2name(model, mjOBJ_JOINT, jnt_id);
                if (ImGui::SliderScalar(label, ImGuiDataType_Double, &q[jnt_id], &lower[jnt_id], &upper[jnt_id]))
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    syncJointState();
                }
            }
        }
        ImGui::End();
    }
}

void Viewer::showMocapGizmo(const std::string &mocapBodyName)
{
    int mocapId = getBodyId(mocapBodyName);
    if (mocapId != -1)
    {
        mocapGizmos.push_back(mocapId);
    }
}

void Viewer::showGizmo(const mjrRect &viewport, float view[16], float proj[16])
{
    for (auto &target_id : mocapGizmos)
    {
        if (target_id >= 0 || target_id < model->nbody)
        {
            ImGui::Begin(("MocapControl" + std::to_string(target_id)).c_str());
            static ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
            if (ImGui::RadioButton("Translate", op == ImGuizmo::TRANSLATE))
            {
                op = ImGuizmo::TRANSLATE;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Rotate", op == ImGuizmo::ROTATE))
            {
                op = ImGuizmo::ROTATE;
            }

            static ImGuizmo::MODE mode = ImGuizmo::WORLD;
            if (ImGui::RadioButton("Local", mode == ImGuizmo::LOCAL))
            {
                mode = ImGuizmo::LOCAL;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("World", mode == ImGuizmo::WORLD))
            {
                mode = ImGuizmo::WORLD;
            }
            mjvGLCamera &glcam = scn.camera[0];
            glcam.orthographic = 0; // perspective
            ImGuizmo::SetOrthographic(glcam.orthographic);
            ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
            ImGuizmo::SetRect(0, 0, (float)viewport.width, (float)viewport.height);

            float rotX90[16] = {
                1, 0,  0, 0,
                0, 0, -1, 0,
                0, 1,  0, 0,
                0, 0,  0, 1
            };
            ImGuizmo::DrawGrid(view, proj, rotX90, 4.0f);

            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos();
            draw_list->AddCircle(ImVec2(p.x + 50, p.y + 50), 30.0f, IM_COL32(255, 0, 0, 255), 32, 2.0f);

            float trans[16];
            getMocapPose(target_id, trans);

            if (ImGuizmo::Manipulate(view, proj, op, mode, trans))
            {
                setMocapPose(target_id, trans);
            }
            ImGui::End();
        }
    }
}

void Viewer::plotChannelData(const std::string &channelName, const std::shared_ptr<DataTamer::PlotSink>& sink) const
{
    static bool show_plot = true;
    if (show_plot)
    {
        ImGui::Begin(("PlotLine: " + channelName).c_str(), &show_plot, ImGuiWindowFlags_AlwaysAutoResize);
        if (ImPlot::BeginPlot("##Scrolling", ImVec2(600, 400)))
        {
            ImPlot::SetupAxes("Time (s)", "Value");
            ImPlot::SetupAxisLimits(ImAxis_X1, data->time - 10.0, data->time, ImGuiCond_Always);
            std::lock_guard<std::shared_mutex> lock(sink->schema_mutex);
            if (sink->channel_data.count(channelName) > 0)
            {
                auto timestamps = sink->channel_plot_data[channelName].first;
                auto values = sink->channel_plot_data[channelName].second;
                ImPlot::PlotLine(channelName.c_str(), timestamps.data(), values.data(), (int)timestamps.size());
            }
            ImPlot::EndPlot();
        }
        ImGui::End();
    }
}

void Viewer::setBodyVisibilityRecursively(int bodyId, bool visible)
{
    bodyVisible[bodyId] = (char)visible;
    for (int childId : bodyChildren[bodyId])
    {
        setBodyVisibilityRecursively(childId, visible);
    }
}

bool Viewer::setBodyVisible(const std::string &bodyName, bool visible)
{
    auto bodyId = mj_name2id(model, mjOBJ_BODY, bodyName.c_str());
    if (bodyId != -1)
    {
        return setBodyVisible(bodyId, visible);
    }
    return false;
}

bool Viewer::setBodyVisible(int bodyId, bool visible)
{
    if (bodyId < 0 || bodyId >= model->nbody)
    {
        CLOG_ERROR << "Invalid body ID: " << bodyId;
        return false;
    }
    int geomStart = model->body_geomadr[bodyId];
    int geomCount = model->body_geomnum[bodyId];
    if (geomStart == -1 || geomCount == 0)
    {
        CLOG_INFO << "Body " << bodyId << " has no associated geoms.";
        return false;
    }
    if (!visible)
    {
        for (int i = 0; i < geomCount; ++i)
        {
            geomIds.insert(geomStart + i);
        }
    }
    return true;
}

void Viewer::hideGeomsById(const std::unordered_set<int>& geomIdsToRemove)
{
    int j = 0;
    for (int i = 0; i < scn.ngeom; ++i)
    {
        const mjvGeom &g = scn.geoms[i];
        bool shouldRemove = (g.objtype == mjOBJ_GEOM) && (geomIdsToRemove.count(g.objid) > 0);
        if (!shouldRemove)
        {
            if (i != j)
            {
                scn.geoms[j] = scn.geoms[i];
            }
            ++j;
        }
    }
    scn.ngeom = j;
}

void Viewer::hideGeomsByVisibleMask(const std::vector<char> &geomVisibleMask)
{
    int j = 0;
    for (int i = 0; i < scn.ngeom; ++i)
    {
        const mjvGeom &g = scn.geoms[i];
        bool keep = true;
        if (g.objtype == mjOBJ_GEOM)
        {
            int geomId = g.objid;
            int bodyId = model->geom_bodyid[geomId];
            if (!geomVisibleMask[bodyId])
            {
                keep = false;
            }
        }
        if (keep)
        {
            if (i != j)
            {
                scn.geoms[j] = scn.geoms[i];
            }
            ++j;
        }
    }
    scn.ngeom = j;
}

void Viewer::showBodyFrame(const std::vector<mjvGeom> &geoms)
{
    for (const auto& g : geoms)
    {
        if (scn.ngeom < scn.maxgeom)
        {
            scn.geoms[scn.ngeom++] = g;
        }
    }
}

void Viewer::hideGeomById(int geomId)
{
    hideGeomsById({geomId});
}

void Viewer::drawBodyFrame(std::vector<mjvGeom> &geoms, int bodyId, double scale) const
{
    Vec3d pos(data->xpos + 3 * bodyId);
    Quaterniond quat(data->xquat[4 * bodyId + 0], data->xquat[4 * bodyId + 1], data->xquat[4 * bodyId + 2],
                     data->xquat[4 * bodyId + 3]);
    drawFrame(geoms, pos, quat.toRotationMatrix(), scale);
}

void Viewer::drawFrame(std::vector<mjvGeom> &geoms, const Vec3d &pos, const Mat3d &rot, double scale)
{
    const double shaft_diam = 0.01 * scale;
    const double head_diam = 0.02 * scale;
    const double head_len = 0.005 * scale;

    auto drawAxis = [&](const Vec3d &dir, const Vec4f &color)
    {
        Vec3d to = pos + rot * dir * scale;
        drawArrow(geoms, pos, to, shaft_diam, head_diam, head_len, color.data());
    };

    drawAxis(Vec3d::UnitX(), Vec4f(1, 0, 0, 1)); // Red
    drawAxis(Vec3d::UnitY(), Vec4f(0, 1, 0, 1)); // Green
    drawAxis(Vec3d::UnitZ(), Vec4f(0, 0, 1, 1)); // Blue
}

void Viewer::drawArrow(std::vector<mjvGeom> &geoms, const Vec3d &from, const Vec3d &to, double shaft_diam,
                      double head_diam, double head_len, const float rgba[4])
{
    mjvGeom geom{};
    int type = mjGEOM_ARROW;
    if (head_diam == 0.0 || head_len == 0.0)
    {
        type = mjGEOM_CYLINDER;
    }
    else if (head_diam == shaft_diam)
    {
        type = mjGEOM_ARROW1;
    }

    Eigen::Vector3d dir = to - from;
    double length = dir.norm();
    if (length < 1e-6)
    {
        return;
    }

    Vec3d z = dir.normalized();
    Quaterniond q = Quaterniond::FromTwoVectors(Vec3d::UnitZ(), z);
    RowMajorMat3d orient = q.toRotationMatrix();
    double size[3] = {shaft_diam, shaft_diam, length / 2.0};
    mjv_initGeom(&geom, type, size, from.data(), orient.data(), rgba);
    geom.emission    = 0.5f;
    geom.specular    = 0.0f;
    geom.shininess   = 0.1f;
    geom.reflectance = 0.1f;
    geoms.push_back(geom);
}

