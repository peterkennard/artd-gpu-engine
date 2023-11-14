#pragma once
#include "artd/gpu_engine.h"

struct GLFWwindow;

ARTD_BEGIN

class GpuEngineImpl;

class ARTD_API_GPU_ENGINE InputManager {
    GpuEngineImpl &owner_;
    int recurse_ = 0;
    int lastWidth_ = -1;
    int lastHeight_ = -1;
    
    float test[16];  // for testing.

    static GpuEngineImpl &getOwner(GLFWwindow * window);

    static void cursorEnterCallback(GLFWwindow* window, int entered);
    static void cursorPositionCallback(GLFWwindow* window, double xPos, double yPos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void charModsCallback(GLFWwindow* window, unsigned int codepoint, int mods);
    static void windowResizeCallback(GLFWwindow* window, int width, int height);

public:

    InputManager(GpuEngineImpl *owner);
    ~InputManager();
    void setGlfwWindowCallbacks(GLFWwindow *window);
    bool pollInputs();
};


ARTD_END
