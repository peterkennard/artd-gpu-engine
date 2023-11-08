#include "GpuEngineImpl.h"

struct GLFWwindow;

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE

InputManager::InputManager(GpuEngineImpl *owner) : owner_(*owner)  {

}
InputManager::~InputManager() {

}

INL GpuEngineImpl &
InputManager::getOwner(GLFWwindow * window) {
    return(reinterpret_cast<InputManager *>(glfwGetWindowUserPointer(window))->owner_);
}

void
InputManager::cursorEnterCallback(GLFWwindow* /*window*/, int entered)
{
    if (entered)
    {
        // The cursor entered the content area of the window
    }
    else
    {
        // The cursor left the content area of the window
    }
}
void
InputManager::cursorPositionCallback(GLFWwindow* /*window*/, double xPos, double yPos)
{
    AD_LOG(print) << "Mouse: " << glm::vec2(xPos, yPos);
}
void
InputManager::mouseButtonCallback(GLFWwindow* /*window*/, int /*button*/, int /*action*/, int /*mods*/)
{
//            if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
//                popup_menu();
}
void
InputManager::scrollCallback(GLFWwindow* /*window*/,
                    double xoffset,
                    double yoffset)
{
    // mouse wheel or touch screen
    AD_LOG(print) << "Wheel: " << glm::vec2(xoffset, yoffset);
}
void
InputManager::keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int mods) {
    // max glfw key is  #define GLFW_KEY_LAST               GLFW_KEY_MENU = 348
    // ALL GLFW MODIFIERS == 0X03F - A BYTE IS OK
    // ACTION IS GLFW_PRESS, GLFW_RELEASE, OR GLFW_REPEAT ( 0 OR 1 OR 2)

    getOwner(window).inputQueue_->postEvent(window, [key, action, mods](void *arg) {

        static int index = 0;

        if((action & 1) != 0) {
            return(false);
        }
        auto &owner = getOwner((GLFWwindow*)arg);
        float inc = 0;
        if(key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
            index = (key - GLFW_KEY_0);
        } else {

            switch(key) {
                case GLFW_KEY_F:
                    owner.freezeAnimation_ = !owner.freezeAnimation_;
                    return(false);
                case GLFW_KEY_RIGHT:
                    inc = .005;
                    break;
                case GLFW_KEY_LEFT:
                    inc = -.005;
                    break;
                case GLFW_KEY_DOWN:
                    inc = -.05;
                    break;
                case GLFW_KEY_UP:
                    inc = .05;
                    break;
                default:
                    AD_LOG(print) << "Processing key " << key << " mods " << std::hex << mods;
                    return(false);
            }
        }
        inc += owner.uniforms.test[index];
        if(inc < 0.0) {
            inc = 0.0;
        } else if(inc > 1.0) {
            inc = 1.0;
        }
        AD_LOG(print) << "val[" << index << "] = " << inc;
        owner.uniforms.test[index] = inc;

        if(index == 0) {
            owner.lights_[0]->setAreaWrap(inc);
        }

        return(false);
    });
}

void
InputManager::windowResizeCallback(GLFWwindow* window, int width, int height) {

    getOwner(window).inputQueue_->postEvent(window, [width, height](void *arg) {

        auto &owner = getOwner((GLFWwindow*)arg);
        owner.resizeSwapChain(width,height);
        return(false);
    });
}

void
InputManager::setGlfwWindowCallbacks(GLFWwindow *window) {

    glfwSetCursorPosCallback(window, &cursorPositionCallback);
    glfwSetMouseButtonCallback(window, &mouseButtonCallback);
    glfwSetCursorEnterCallback(window, &cursorEnterCallback);
    glfwSetScrollCallback(window, &scrollCallback);
    glfwSetKeyCallback(window, &keyCallback);
    
    // window even callbacks
    glfwSetFramebufferSizeCallback(window, &windowResizeCallback);

    // glfwSetCharCallback(GLFWwindow* handle, GLFWcharfun cbfun)
 //   glfwSetCharModsCallback(GLFWwindow* handle, GLFWcharmodsfun cbfun);
}


ARTD_END
