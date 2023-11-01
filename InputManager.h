#pragma once
#include "artd/gpu_engine.h"

struct GLFWwindow;

ARTD_BEGIN

class GpuEngineImpl;

class ARTD_API_GPU_ENGINE InputManager {
    GpuEngineImpl &owner_;

    static GpuEngineImpl &getOwner(GLFWwindow * window);

    static void cursorEnterCallback(GLFWwindow* window, int entered);
    static void cursorPositionCallback(GLFWwindow* window, double xPos, double yPos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

public:
    float test[16];  // for testing input handling.

    InputManager(GpuEngineImpl *owner);
    ~InputManager();
    void setGlfwWindowCallbacks(GLFWwindow *window);
};


ARTD_END
