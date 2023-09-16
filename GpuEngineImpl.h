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
#include <array>


#include "PixelReader.h"

ARTD_BEGIN

struct MyUniforms {
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
static_assert(sizeof(MyUniforms) % 16 == 0);

using namespace wgpu;

// was through step030 of webgpu tutorial

class GpuEngineImpl
    : public GpuEngine
{
protected:

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

    // added 056 items
    // Mesh stuff for pyramid mesh object

    std::vector<float> pointData;
    std::vector<uint16_t> indexData;
	Buffer vertexBuffer = nullptr;
	Buffer indexBuffer = nullptr;
    glm::mat4x4 T1; // object position ( translation ) as full matrix !!!
    glm::mat4x4 S;  // scaling of object
    
	
    // global scene uniforms ?? )( I htink has model matrix in int too !!
    BindGroup bindGroup = nullptr;
    Buffer uniformBuffer = nullptr;
    MyUniforms uniforms;
    
    // scene graph items
    ObjectPtr<CachedMeshLoader> meshLoader_;
    ObjectPtr<Viewport> viewport_;
    ObjectPtr<CameraNode> camNode_;
    ObjectPtr<ShaderManager> shaderManager_;

    GpuEngineImpl() {

        viewport_ = ObjectBase::make<Viewport>();
        camNode_ = ObjectBase::make<CameraNode>();
        auto cam = ObjectBase::make<Camera>();
        camNode_->setCamera(cam);
        meshLoader_ = ObjectBase::make<CachedMeshLoader>();
        cam->setViewport(viewport_);
    };
    ~GpuEngineImpl() {
        releaseResources();
    }

// added 056 items

    static constexpr float PI = 3.14159265358979323846f;

    /**
     * The same structure as in the shader, replicated in C++
     */
    struct MyUniforms {
        // scene/camera global transform matrices
        Matrix4f projectionMatrix;
        Matrix4f viewMatrix;

        // per model items
        Matrix4f modelMatrix;
        std::array<float, 4> color;

        float time;
        float _pad[3];  // TODO make this a macro somehow to not need assert
    };

    // Have the compiler check byte alignment
    static_assert(sizeof(MyUniforms) % 16 == 0);
    
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
    void releaseResources();
    ObjectPtr<PixelReader> pixelGetter_;
    int getPixels(uint32_t *pBuf);
    const int *lockPixels(int timeoutMillis);
    void unlockPixels();
};


ARTD_END
