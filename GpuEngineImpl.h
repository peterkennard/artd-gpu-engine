#pragma once

#include "artd/GpuEngine.h"

#include <webgpu/webgpu.hpp>
#include <GLFW/glfw3.h>

#include "artd/Logger.h"
#include "artd/ObjectBase.h"
#include "artd/WaitableSignal.h"
#include "artd/Thread.h"
#include "artd/Viewport.h"
#include "artd/CameraNode.h"
#include "artd/Camera.h"


#include "PixelReader.h"

ARTD_BEGIN


using namespace wgpu;


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

    // scene items
    ObjectPtr<Viewport> viewport_;
    ObjectPtr<CameraNode> camNode_;

    GpuEngineImpl() {
        viewport_ = ObjectBase::make<Viewport>();
        camNode_ = ObjectBase::make<CameraNode>();
        auto cam = ObjectBase::make<Camera>();
        camNode_->setCamera(cam);
        cam->setViewport(viewport_);
    };
    ~GpuEngineImpl() {
        releaseResources();
    }

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
