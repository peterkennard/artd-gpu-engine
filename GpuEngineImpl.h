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
#include "artd/Camera.h"
#include "artd/CachedMeshLoader.h"
#include "artd/GpuBufferManager.h"
#include "artd/TimingContext.h"

#include <array>
#include <chrono>


#include "PixelReader.h"

ARTD_BEGIN

class MeshNode;

#define INL ARTD_ALWAYS_INLINE

struct SceneUniforms {
	// scene frame specific items
    glm::mat4x4 projectionMatrix;
    glm::mat4x4 viewMatrix;
    glm::mat4x4 modelMatrix;  // model specific

    // below here is really frame global specific
    std::array<float, 4> color;
    float time;
    float _pad[3];
};

// Have the compiler check byte alignment
static_assert(sizeof(SceneUniforms) % 16 == 0);

struct InstanceData  {
    glm::mat4x4 modelMatrix;  // model specific
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
    uint32_t width_;
    uint32_t height_;
    Instance instance = nullptr;
    Surface surface = nullptr;
    Adapter adapter = nullptr;
    Device device = nullptr;
    std::unique_ptr<ErrorCallback> errorCallback_;
    Queue queue = nullptr;
    Texture targetTexture = nullptr;
    TextureView targetTextureView = nullptr;
    ShaderModule shaderModule = nullptr;
    RenderPipeline pipeline = nullptr;
    SwapChain swapChain = nullptr;
   
    TextureView depthTextureView = nullptr;
    Texture depthTexture = nullptr;

    glm::mat4x4 T1; // object position ( translation ) as full matrix !!!
    glm::mat4x4 S;  // scaling of object

    // global for whole scene camera lights etc
    Buffer sceneUniformBuffer_ = nullptr;

    // global scene uniforms ?? )( I htink has model matrix in int too !!
    BindGroup bindGroup = nullptr;
    Buffer uniformBuffer = nullptr;

    SceneUniforms uniforms;

    // test of instancing for multiple objects
    Buffer instanceDataBuffer_ = nullptr;

    
    // resource management items
    ObjectPtr<GpuBufferManager> bufferManager_;
    ObjectPtr<CachedMeshLoader> meshLoader_;
    ObjectPtr<ShaderManager>    shaderManager_;

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

    // scene graph items
    ObjectPtr<Viewport> viewport_;
    ObjectPtr<CameraNode> camNode_;
    ObjectPtr<TransformNode> coneGroup_;
    // TODO: to be grouped by shader/pipeline  Just a hack for now.
    std::vector<MeshNode*> drawables_;

    GpuEngineImpl() {

        bufferManager_ = GpuBufferManager::create(this);
        viewport_ = ObjectBase::make<Viewport>();
        camNode_ = ObjectBase::make<CameraNode>();
        auto cam = ObjectBase::make<Camera>();
        camNode_->setCamera(cam);
        meshLoader_ = ObjectBase::make<CachedMeshLoader>();
        cam->setViewport(viewport_);
        // TODO: we need a scene
        coneGroup_ = ObjectBase::make<TransformNode>();
    };
    ~GpuEngineImpl() {
        releaseResources();
    }

    /**
     * A structure that describes the data layout in the vertex buffer
     * We do not instantiate it but use it in `sizeof` and `offsetof`
     */
    struct VertexAttributes {
        Vec3f position;
        Vec3f normal;
        Vec3f color;
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
