#pragma once

#include "artd/GpuEngine.h"
#include "artd/ShaderManager.h"

#include <webgpu/webgpu.hpp>
#include <GLFW/glfw3.h>

#include "artd/Logger.h"
#include "artd/ObjectBase.h"
#include "artd/WaitableSignal.h"
#include "artd/Thread.h"
#include "artd/Viewport.h"
#include "artd/CameraNode.h"
#include "artd/LightNode.h"
#include "artd/Camera.h"
#include "artd/CachedMeshLoader.h"
#include "artd/GpuBufferManager.h"
#include "artd/TimingContext.h"
#include "artd/LambdaEventQueue.h"
#include "./InputManager.h"
#include "./FpsMonitor.h"
#include "./Material.h"

#include <array>
#include <chrono>


#include "PixelReader.h"

ARTD_BEGIN

class MeshNode;
class PickerPass;

#define INL ARTD_ALWAYS_INLINE

struct SceneUniforms {
	// scene frame specific items
    glm::mat4x4 projectionMatrix;
    glm::mat4x4 viewMatrix;
    glm::mat4x4 vpMatrix;  // projection * view
    glm::mat4x4 eyePose; // align 16 boundary - consumes 48
    float test[16];
    float time;

    uint32_t numLights;
    float _pad[2];

    static const int MaxLights = 64;
};

// Have the compiler check byte alignment
static_assert(sizeof(SceneUniforms) % 16 == 0);

struct InstanceData  {
    glm::mat4x4 modelMatrix;  // model specific
    uint32_t materialId;
    uint32_t objectId;
    uint32_t _pad[2];
};

static_assert(sizeof(InstanceData) % 16 == 0);



using namespace wgpu;

// was through step030 of webgpu tutorial

class GpuEngineImpl
    : public GpuEngine
{
protected:

    friend class GpuBufferManagerImpl;
    friend class GpuBufferManager;

    bool headless_ = true;
    GLFWwindow* window = nullptr;

    int initGlfwDisplay();

    uint32_t width_;
    uint32_t height_;
    Instance instance = nullptr;
    Surface surface = nullptr;
    Adapter adapter = nullptr;
    Device device = nullptr;
    std::unique_ptr<ErrorCallback> errorCallback_;
    std::unique_ptr<DeviceLostCallback> deviceLostCallback_;
    Queue queue = nullptr;
    Texture targetTexture = nullptr;
    TextureView targetTextureView = nullptr;
    ShaderModule shaderModule = nullptr;
    RenderPipeline pipeline = nullptr;
    SwapChain swapChain = nullptr;
   
    TextureView depthTextureView = nullptr;
    Texture depthTexture = nullptr;

    BindGroup bindGroup = nullptr;
    BindGroup textureBindGroup = nullptr;

    // global scene uniforms camera and lights, test data things constant for a single frame of animation/render.
    SceneUniforms uniforms;
    
    ObjectPtr<LambdaEventQueue> inputQueue_;
    ObjectPtr<LambdaEventQueue> updateQueue_;
   friend class PickerPass;
   ObjectPtr<PickerPass>        pickerPass_;

    // resource management items
    friend class InputManager;
    ObjectPtr<InputManager>     inputManager_;
    ObjectPtr<ResourceManager>  resourceManager_;
    ObjectPtr<GpuBufferManager> bufferManager_;
    ObjectPtr<CachedMeshLoader> meshLoader_;
    ObjectPtr<ShaderManager>    shaderManager_;

    // there won't be too many of these ?? they are 128 bits !!
    ObjectPtr<BufferChunk>      uniformBuffer_;
    ObjectPtr<BufferChunk>      instanceBuffer_;
    ObjectPtr<BufferChunk>      materialBuffer_;

    class RenderTimingContext
        : public TimingContext
    {
        double debugTime_ = 0;
        double debugInterval_ = 1.1;
        std::chrono::time_point<std::chrono::high_resolution_clock> firstTime_;
        std::chrono::time_point<std::chrono::high_resolution_clock> lastHRTime_;

        double deltaTime() {
            std::chrono::time_point<std::chrono::high_resolution_clock> hereNow = std::chrono::high_resolution_clock::now();
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(hereNow - lastHRTime_); // Microsecond (as integer)
            lastHRTime_ = hereNow;
            double usd = static_cast<double>(us.count());
            return(usd/1000000.0);
        }

    public:
        RenderTimingContext() {            
        }

        double currentTime() {
            auto hereNow = std::chrono::high_resolution_clock::now();
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(hereNow - firstTime_); // Microsecond (as integer)
            return(static_cast<double>(us.count())/1000000.0);
        }

        void init(int initialFrame)
        {
            frameNumber_ = initialFrame;
            firstTime_ = lastHRTime_ = std::chrono::high_resolution_clock::now();
            frameTime_ = currentTime(); // note this is not adjusted for epoch
            elapsedSinceLast_ = 0;
            debugFrame_ = false;
        }

        void tickFrame()
        {
            elapsedSinceLast_ = deltaTime();
            frameNumber_ = frameNumber_ + 1;

           // if(elapsedSinceLast_ <= .001) {
           //     log.info(" really small frame time " + elapsedSinceLast_);
           // }

            // todo apply epocch start to this ? keep monotonic accurate for clock time ?
            frameTime_ += elapsedSinceLast_;
            debugTime_ += elapsedSinceLast_;
            if(debugTime_ > debugInterval_) {
                debugFrame_ = true;
                debugTime_ = 0;
            } else {
                debugFrame_ = false;
            }
        }
    };

    RenderTimingContext timing_;
    FpsMonitor fpsMonitor_;

    // scene graph items
    ObjectPtr<Viewport> viewport_;
    ObjectPtr<CameraNode> camNode_;
    ObjectPtr<TransformNode> ringGroup_;
    // TODO: to be grouped by shader/pipeline  Just a hack for now.
    std::vector<MeshNode*> drawables_;

    // TODO: active vs: turned off ( visible invisible ) with or without parents ...
    std::vector<ObjectPtr<LightNode>> lights_;
    // TODO: Integrate into ResourceManager, or create a material manager to handle dynamism creating/deleting
    std::vector<ObjectPtr<Material>> materials_;

    GpuEngineImpl();
    ~GpuEngineImpl();

    /**
     * A structure that describes the data layout in the vertex buffer
     * We do not instantiate it but use it in `sizeof` and `offsetof`
     */
    struct VertexAttributes {
        Vec3f position;
        Vec3f normal;
        glm::vec2 uv;
    };

// end added 056 items

public:

    static GpuEngineImpl &getInstance() {
        static GpuEngineImpl wgpu;
        return(wgpu);
    }
protected:

    WaitableSignal pixelLockLock_;
    WaitableSignal pixelUnLockLock_;
    
    bool processEvents();
    void presentImage(TextureView /*texture*/ );
    TextureView getNextTexture();

public:

    int init(bool headless, int width, int height);
    int renderFrame();
    INL const TimingContext &timing() const { return(timing_); }
    void releaseResources();
    ObjectPtr<PixelReader> pixelGetter_;
    int getPixels(uint32_t *pBuf);
    const int *lockPixels(int timeoutMillis);
    void unlockPixels();
};

#undef INL

ARTD_END
