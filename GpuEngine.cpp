/**
 * This file is originally based on  the  "Learn WebGPU for C++" book.
 *   https://github.com/eliemichel/LearnWebGPU
 *   'tis a major fork and redone a lot.
 * 
 * MIT License
 * Copyright (c) 2022-2023 Elie Michel
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <glfw3webgpu.h>

// 056
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_FORCE_LEFT_HANDED

#define WEBGPU_CPP_IMPLEMENTATION  // declares method implementations
#include <webgpu/webgpu.hpp>
#undef WEBGPU_CPP_IMPLEMENTATION  // declares method implementations

#include "GpuEngineImpl.h"

#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <array>
#include <chrono>

#include "artd/GpuEngine-PanamaExports.h"
#include "artd/Matrix4f.h"
#include "artd/Color3f.h"
#include "MeshNode.h"
#include "DrawableMesh.h"
#include "artd/pointer_math.h"
#include "./FpsMonitor.h"
#include "./PickerPass.h"
#include "./GpuErrorHandler.h"
#include "./TextureManager.h"

ARTD_BEGIN

using namespace wgpu;


GpuEngine::GpuEngine()
{

};
GpuEngine::~GpuEngine() {

};

//static void doNutin(void *addr) {
//    if(!addr) {
//        AD_LOG(print) << "null";
//    }
//}

namespace fs = std::filesystem;


int
GpuEngineImpl::initGlfwDisplay() {
    
    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window = glfwCreateWindow(width_, height_, "WebGPU Engine", NULL, NULL);
    if (!window) {
        AD_LOG(info) << "Could not open window!";
        return 1;
    }
    glfwSetWindowUserPointer(window, reinterpret_cast<void *>(inputManager_.get()));
    surface = glfwGetWGPUSurface(instance, window);
    inputManager_->setGlfwWindowCallbacks(window);
    return(0);
}

GpuEngineImpl::GpuEngineImpl()
{
    bufferManager_ = GpuBufferManager::create(this);
    viewport_ = ObjectPtr<Viewport>::make();

    defaultCamera_ = ObjectPtr<CameraNode>::make();
    auto camera = ObjectBase::make<Camera>();
    defaultCamera_->setCamera(camera);
    meshLoader_ = ObjectPtr<CachedMeshLoader>::make(this);
    camera->setViewport(viewport_);
    camera->setNearClip(0.01f);
    camera->setFarClip(100.0f);
    camera->setFocalLength(2.0);

    std::memset(&uniforms,0,sizeof(uniforms));
}

GpuEngineImpl::~GpuEngineImpl()
{
    releaseResources();
}
void
GpuEngineImpl::releaseResources() {
    if(instance) {

        releaseDepthBuffer();
        releaseSwapChain();

        textureManager_->shutdown();
        meshLoader_ = nullptr;
        bufferManager_->shutdown();

        device.release();
        adapter.release();
        instance.release();
        instance = nullptr;
    }
    if(headless_) {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}

int
GpuEngineImpl::initSwapChain(int width, int height) {

    queue = device.getQueue();
    swapChainFormat_ = TextureFormat::BGRA8Unorm;

    if(headless_) {
        // No more swap chain, but still a target format and a target texture

        TextureDescriptor targetTextureDesc;
        targetTextureDesc.label = "Render texture";
        targetTextureDesc.dimension = TextureDimension::_2D;
        // Any size works here, this is the equivalent of the window size
        targetTextureDesc.size = { width_, height_, 1 };
        // Use the same format here and in the render pipeline's color target
        targetTextureDesc.format = swapChainFormat_;
        // No need for MIP maps
        targetTextureDesc.mipLevelCount = 1;
        // You may set up supersampling here
        targetTextureDesc.sampleCount = 1;
        // At least RenderAttachment usage is needed. Also add CopySrc to be able
        // to retrieve the texture afterwards.
        targetTextureDesc.usage = TextureUsage::RenderAttachment | TextureUsage::CopySrc;
        targetTextureDesc.viewFormats = nullptr;
        targetTextureDesc.viewFormatCount = 0;
        targetTexture = device.createTexture(targetTextureDesc);

        TextureViewDescriptor targetTextureViewDesc;
        targetTextureViewDesc.label = "Render texture view";
        // Render to a single layer
        targetTextureViewDesc.baseArrayLayer = 0;
        targetTextureViewDesc.arrayLayerCount = 1;
        // Render to a single mip level
        targetTextureViewDesc.baseMipLevel = 0;
        targetTextureViewDesc.mipLevelCount = 1;
        // Render to all channels
        targetTextureViewDesc.aspect = TextureAspect::All;
        targetTextureView = targetTexture.createView(targetTextureViewDesc);

    } else {

        AD_LOG(info) << "Creating swapchain...";
#ifdef WEBGPU_BACKEND_WGPU
        swapChainFormat = surface.getPreferredFormat(adapter);
#endif
        SwapChainDescriptor swapChainDesc;
        swapChainDesc.width = static_cast<uint32_t>(width);
        swapChainDesc.height = static_cast<uint32_t>(height);
        swapChainDesc.usage = TextureUsage::RenderAttachment;
        swapChainDesc.format = swapChainFormat_;
        swapChainDesc.presentMode = PresentMode::Fifo;
        swapChain = device.createSwapChain(surface, swapChainDesc);
        AD_LOG(info) << "Swapchain created " << swapChain;
    }
    width_ = width;
    height_ = height;
    return(0);
}

void
GpuEngineImpl::releaseSwapChain() {
    if(!headless_) {
        if(swapChain) {
            swapChain.release();
        }
    } else {

    }
}

int
GpuEngineImpl::initDepthBuffer() {
    // Create the depth texture

    TextureDescriptor depthTextureDesc;
    depthTextureDesc.dimension = TextureDimension::_2D;
    depthTextureDesc.format = depthTextureFormat_;
    depthTextureDesc.mipLevelCount = 1;
    depthTextureDesc.sampleCount = 1;
    depthTextureDesc.size = {width_, height_, 1};
    depthTextureDesc.usage = TextureUsage::RenderAttachment;
    depthTextureDesc.viewFormatCount = 1;
    depthTextureDesc.viewFormats = (WGPUTextureFormat*)&depthTextureFormat_;
    AD_LOG(info) << "Creating Depth texture";
    depthTexture = device.createTexture(depthTextureDesc);
    AD_LOG(info) << "Depth texture: " << depthTexture;

	// Create the view of the depth texture manipulated by the rasterizer

    TextureViewDescriptor depthTextureViewDesc;
    depthTextureViewDesc.aspect = TextureAspect::DepthOnly;
    depthTextureViewDesc.baseArrayLayer = 0;
    depthTextureViewDesc.arrayLayerCount = 1;
    depthTextureViewDesc.baseMipLevel = 0;
    depthTextureViewDesc.mipLevelCount = 1;
    depthTextureViewDesc.dimension = TextureViewDimension::_2D;
    depthTextureViewDesc.format = depthTextureFormat_;
    AD_LOG(info) << "Creating Depth texture view";
    depthTextureView = depthTexture.createView(depthTextureViewDesc);
    AD_LOG(info) << "Depth texture view: " << depthTextureView;

    if(depthTextureView == nullptr) {
        return(1);
    }
    return(0);
}

void
GpuEngineImpl::releaseDepthBuffer() {
    if(depthTextureView) {
        depthTextureView.release();
    }
    if(depthTexture) {
        depthTexture.destroy();
        depthTexture.release();
    }
}

int
GpuEngineImpl::resizeSwapChain(int width, int height) {
    AD_LOG(print) << "Resizing window to " << glm::vec2(width,height);

    releaseDepthBuffer();
    releaseSwapChain();

    initSwapChain(width,height);
    initDepthBuffer();

    viewport_->setRect(0,0,width,height);

    return(1);
}

int
GpuEngineImpl::init(bool headless, int width, int height) {

    {
        auto eventPool = ObjectPtr<LambdaEventQueue::LambdaEventPool>::make();
        inputQueue_ = ObjectPtr<LambdaEventQueue>::make(eventPool);
        updateQueue_ = ObjectPtr<LambdaEventQueue>::make(eventPool);
    }
    
    
    width_ = width;
    height_ = height;
    headless_ = headless;
    int ret;  // return code
    
    AD_LOG(info) << "initializing " << (headless_ ? "headless" : "windowed") << " test engine";
    
    InstanceDescriptor instanceDescriptor = {};
    
    std::vector<const char*> enabledToggles = {
        "allow_unsafe_apis",
    };
    DawnTogglesDescriptor dawnToggles;
    dawnToggles.chain.next = nullptr;
    dawnToggles.chain.sType = SType::DawnTogglesDescriptor;
    dawnToggles.enabledTogglesCount = enabledToggles.size();
    dawnToggles.enabledToggles = enabledToggles.data();
    dawnToggles.disabledTogglesCount = 0;
    
    instanceDescriptor.nextInChain = &dawnToggles.chain;
    
    
    instance = createInstance(instanceDescriptor);
    if (!instance) {
        AD_LOG(error) << "Could not initialize WebGPU!" << std::endl;
        return 1;
    }

    inputManager_ = ObjectPtr<InputManager>::make(this);
    
    if(headless_) {
        surface = nullptr;  // done in constructor
    } else {
        ret = initGlfwDisplay();
        if(ret) {
            return(ret);
        }
    }
    
    AD_LOG(info) << "Requesting adapter...";
    
    RequestAdapterOptions adapterOpts{};
    adapterOpts.compatibleSurface = surface;
    
    adapter = instance.requestAdapter(adapterOpts);
    std::cout << "Got adapter: " << adapter << std::endl;
    
    // specify device limits - we need to figure out a dynamic or "max" way of setting this up.
    SupportedLimits supportedLimits;
    adapter.getLimits(&supportedLimits);
    
    AD_LOG(info) << "Requesting device...";
    RequiredLimits requiredLimits = Default;
    requiredLimits.limits.maxVertexAttributes = 3;
    //                                          ^ This was a 2
    requiredLimits.limits.maxVertexBuffers = 1;  // max here is 8 TODO: we need the buffer namager to split these up !
    requiredLimits.limits.maxBufferSize = 256 * sizeof(VertexAttributes);
    
    requiredLimits.limits.maxVertexBufferArrayStride = sizeof(VertexAttributes) * 2;  // cx ? does bigger hurt
    
    requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
    requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
    requiredLimits.limits.maxInterStageShaderComponents = 16; // this was 3 PK
    
    requiredLimits.limits.maxBindGroups = 4;  // max supported seems to be 4
    requiredLimits.limits.maxUniformBuffersPerShaderStage = 8; // 12 is max here
    // Update max uniform buffer size:
    requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float) * 2;  // 2x added enough ?

    requiredLimits.limits.maxSamplersPerShaderStage = 4; // 16 is max allowed

    // so is this a definition of the maximum renderer viewport size ?
    requiredLimits.limits.maxTextureDimension1D = height_;
    requiredLimits.limits.maxTextureDimension2D = width_;
    requiredLimits.limits.maxTextureArrayLayers = 1;
    
    DeviceDescriptor deviceDesc;
    deviceDesc.label = "My Device";
    deviceDesc.requiredFeaturesCount = 0;
    
    deviceDesc.requiredLimits = &requiredLimits;
    deviceDesc.defaultQueue.label = "The default queue";
    device = adapter.requestDevice(deviceDesc);
    AD_LOG(info) << "Got device: " << device;
        
    // Add an error callback for more debug info
    errorCallback_ = GpuErrorHandler::initErrorCallback(device);

    deviceLostCallback_ = device.setDeviceLostCallback([](DeviceLostReason /*reason*/, char const * /*message*/) {
    });

    
    shaderManager_ = ObjectPtr<ShaderManager>::make(device);
    resourceManager_ = ResourceManager::create();
    textureManager_ = TextureManager::create(this);

    ret = initSwapChain(width,height);
    if(ret) {
        return(ret);
    }

    viewport_->setRect(0,0,width,height);

    depthTextureFormat_ = TextureFormat::Depth24Plus;
    ret = initDepthBuffer();
    if(ret) {
        return(ret);
    }
    
    pickerPass_ = ObjectPtr<PickerPass>::make(this);
    
    AD_LOG(info) << "Creating shader module...";
    shaderModule = shaderManager_->loadShaderModule("testShader1.wgsl");
    AD_LOG(info)  << "Shader module: " << shaderModule;
    
    AD_LOG(info) << "Creating render pipeline...";
    RenderPipelineDescriptor pipelineDesc;
    
    std::vector<VertexAttribute> vertexAttribs(3);
    // Position attribute
    vertexAttribs[0].shaderLocation = 0;
    vertexAttribs[0].format = VertexFormat::Float32x3;
    vertexAttribs[0].offset = 0;
    
    // Normal attribute
    vertexAttribs[1].shaderLocation = 1;
    vertexAttribs[1].format = VertexFormat::Float32x3;
    vertexAttribs[1].offset = offsetof(VertexAttributes, normal);
    
    // UV attribute
    vertexAttribs[2].shaderLocation = 2;
    vertexAttribs[2].format = VertexFormat::Float32x2;
    vertexAttribs[2].offset = offsetof(VertexAttributes, uv);
    
    VertexBufferLayout vertexBufferLayout;
    vertexBufferLayout.attributeCount = vertexAttribs.size();
    vertexBufferLayout.attributes = vertexAttribs.data();
    vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
    vertexBufferLayout.stepMode = VertexStepMode::Vertex;
    
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
    
    // Vertex shader
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;
    
    // Each sequence of 3 vertices is considered as a triangle
    pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;
    // connected. When not specified, vertices are considered sequentially.
    pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
    // The face orientation is defined by assuming that when looking
    // from the front of the face, its corner vertices are enumerated
    // in the counter-clockwise (CCW) order.
    //    pipelineDesc.primitive.frontFace = FrontFace::CCW;
    pipelineDesc.primitive.frontFace = FrontFace::CCW;  // right handed convention
    // cull (i.e. "hide") the faces pointing away from us (which is often
    // used for optimization).
    pipelineDesc.primitive.cullMode = // CullMode::None;
    CullMode::Back;  // this is for the "opaque" pass
    
    // Fragment shader
    static FragmentState fragmentState;
    pipelineDesc.fragment = &fragmentState;
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;
    
    // Configure blend state
    static BlendState blendState;
    // Usual alpha blending for the color:
    blendState.color.srcFactor = BlendFactor::SrcAlpha;
    blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
    blendState.color.operation = BlendOperation::Add;
    // We leave the target alpha untouched:
    blendState.alpha.srcFactor = BlendFactor::Zero;
    blendState.alpha.dstFactor = BlendFactor::One;
    blendState.alpha.operation = BlendOperation::Add;
    
    static ColorTargetState colorTarget;
    
    colorTarget.format = swapChainFormat_;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = ColorWriteMask::All; // We could write to only some of the color channels.
    
    // We have only one target because our render pass has only one output color
    // attachment.
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    DepthStencilState depthStencilState = Default;
    depthStencilState.depthCompare = CompareFunction::Less;
    depthStencilState.depthWriteEnabled = true;
    depthStencilState.format = depthTextureFormat_;
    depthStencilState.stencilReadMask = 0;
    depthStencilState.stencilWriteMask = 0;
    
    pipelineDesc.depthStencil = &depthStencilState;
    
    // Multi-sampling
    // Samples per pixel
    pipelineDesc.multisample.count = 1;
    pipelineDesc.multisample.mask = ~0u;
    pipelineDesc.multisample.alphaToCoverageEnabled = false;
    
    // Create binding layout (don't forget to = Default)
    
    // bindGroupLayout = nullptr;
    BindGroupLayoutEntry bindingLayouts[4];
    {
        AD_LOG(info) << "sizeof(SceneUniforms) = " << sizeof(SceneUniforms) << "  sizeof(LightData) = " << sizeof(LightShaderData);

        bindingLayouts[0] = Default;
        bindingLayouts[0].binding = 0;
        bindingLayouts[0].visibility = ShaderStage::Vertex | ShaderStage::Fragment;
        bindingLayouts[0].buffer.type = BufferBindingType::Uniform;
        bindingLayouts[0].buffer.minBindingSize = sizeof(SceneUniforms) + (SceneUniforms::MaxLights * sizeof(LightShaderData));
        
        bindingLayouts[1] = Default;
        bindingLayouts[1].binding = 1;
        bindingLayouts[1].visibility = ShaderStage::Vertex | ShaderStage::Fragment;
        bindingLayouts[1].buffer.type = BufferBindingType::ReadOnlyStorage;
        bindingLayouts[1].buffer.minBindingSize = sizeof(InstanceData);

        bindingLayouts[2] = Default;
        bindingLayouts[2].binding = 2;
        bindingLayouts[2].visibility = ShaderStage::Vertex | ShaderStage::Fragment;
        bindingLayouts[2].buffer.type = BufferBindingType::ReadOnlyStorage;
        bindingLayouts[2].buffer.minBindingSize = sizeof(MaterialShaderData);

        // for now putting this in the uniforms
        bindingLayouts[3] = Default;
        bindingLayouts[3].binding = 3;
        bindingLayouts[3].visibility = ShaderStage::Fragment;
        bindingLayouts[3].sampler.type = SamplerBindingType::Filtering;
    }
    
    // Create a bind group layout
    BindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.entryCount = 4; // todo take from data struct
    bindGroupLayoutDesc.entries =  bindingLayouts;
    BindGroupLayout bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

// create bind group layout for texture !
#ifdef WEBGPU_BACKEND_DAWN
        // Check for pending error callbacks
        device.tick();
#endif

    
	// The material bind group layout.
    
    BindGroupLayoutEntry bindingLayouts2[1];
	BindGroupLayoutEntry& textureBindingLayout = bindingLayouts2[0];
    textureBindingLayout = Default;
	textureBindingLayout.binding = 0;
	textureBindingLayout.visibility = ShaderStage::Fragment;
	textureBindingLayout.texture.sampleType = TextureSampleType::Float;
	textureBindingLayout.texture.viewDimension = TextureViewDimension::_2D;

    BindGroupLayoutDescriptor bindGroupLayoutDesc1{};
    bindGroupLayoutDesc1.entryCount = 1; // todo take from data struct
    bindGroupLayoutDesc1.entries =  bindingLayouts2;
    materialBindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc1);
        
#ifdef WEBGPU_BACKEND_DAWN
        // Check for pending error callbacks
        device.tick();
#endif


    // Create the pipeline layout
    {
        BindGroupLayout layouts[2] { bindGroupLayout, materialBindGroupLayout  };
        
        PipelineLayoutDescriptor layoutDesc{};
        layoutDesc.bindGroupLayoutCount = 2;
        layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)layouts;
     //   layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)&bindGroupLayout;


        // Pipeline layout
        PipelineLayout layout = device.createPipelineLayout(layoutDesc);
        pipelineDesc.layout = layout;
    }
    
#ifdef WEBGPU_BACKEND_DAWN
        // Check for pending error callbacks
        device.tick();
#endif


    AD_LOG(info) << "Creating Render pipeline";
    pipeline = device.createRenderPipeline(pipelineDesc);
    AD_LOG(info) << "Render pipeline: " << pipeline;

    // not really needed here first thing done when rendering a frame
	uniforms.time = 1.0f;

	// Create bindings for test objects
	{
        uniformBuffer_ = bufferManager_->allocUniformChunk(sizeof(SceneUniforms) + (64 * sizeof(LightShaderData)) );
        
        BindGroupEntry bindings[4];

        {
            BufferChunk &b = *uniformBuffer_;
            bindings[0].binding = 0;
            bindings[0].buffer = b.getBuffer();
            bindings[0].offset = b.getStartOffset();
            bindings[0].size = b.getSize();
        }

        instanceBuffer_ = bufferManager_->allocStorageChunk(128 * sizeof(InstanceData));

        {
            BufferChunk &b = *instanceBuffer_;
            bindings[1].binding = 1;
            bindings[1].buffer = b.getBuffer();
            bindings[1].offset = b.getStartOffset();
            bindings[1].size = b.getSize();
        }

        materialBuffer_ = bufferManager_->allocStorageChunk(64 * sizeof(MaterialShaderData));

        {
            BufferChunk &b = *materialBuffer_;
            bindings[2].binding = 2;
            bindings[2].buffer = b.getBuffer();
            bindings[2].offset = b.getStartOffset();
            bindings[2].size = b.getSize();
        }

        {
            // Create a sampler
            SamplerDescriptor samplerDesc;
            samplerDesc.addressModeU = AddressMode::ClampToEdge;
            samplerDesc.addressModeV = AddressMode::ClampToEdge;
            samplerDesc.addressModeW = AddressMode::ClampToEdge;
            samplerDesc.magFilter = FilterMode::Linear;
            samplerDesc.minFilter = FilterMode::Linear;
            samplerDesc.mipmapFilter = MipmapFilterMode::Linear;
            samplerDesc.lodMinClamp = 0.0f;
            samplerDesc.lodMaxClamp = 1.0f;
            samplerDesc.compare = CompareFunction::Undefined;
            samplerDesc.maxAnisotropy = 1;
            Sampler sampler0_ = device.createSampler(samplerDesc);

            bindings[3].binding = 3;
            bindings[3].sampler = sampler0_;
        }

        // A bind group contains one or multiple bindings
        BindGroupDescriptor bindGroupDesc;
        bindGroupDesc.layout = bindGroupLayout;
        bindGroupDesc.entryCount = 4;
        bindGroupDesc.entries = bindings;
        bindGroup = device.createBindGroup(bindGroupDesc);
    }

#ifdef WEBGPU_BACKEND_DAWN
        // Check for pending error callbacks
        device.tick();
#endif

    // this is only for main window

    if(headless_) {
        pixelGetter_ = artd::ObjectPtr<PixelReader>::make(device, width_,height_);
        pixelUnLockLock_.signal(); // start enabled !!
    }

    // create defaultMaterial
    {
        ObjectPtr<Material> mat = ObjectPtr<Material>::make(this);
        defaultMaterial_ = mat;
        auto pMat = defaultMaterial_.get();
        pMat->setDiffuse(Color3f(30,30,30));
        pMat->bindings_ = createMaterialBindGroup(*pMat);
    }

    ret = initScene();
    AD_LOG(info) << "init complete !";
    timing_.init(0);
    return(ret);
}

wgpu::BindGroup
GpuEngineImpl::createMaterialBindGroup(Material &forM) {

    TextureView *diffuse = forM.getDiffuseTexture().get();
    BindGroupEntry bindings[1];

    if(!diffuse) {
        diffuse = textureManager_->getNullTextureView().get();
    }
    
    bindings[0].binding = 0;
    bindings[0].textureView = diffuse->getView();

    BindGroupDescriptor bindGroupDesc;
    bindGroupDesc.layout = materialBindGroupLayout;
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries = bindings;

    return(device.createBindGroup(bindGroupDesc));
}


// TODO: load from some description or have something "initialize" it and set it.
int
GpuEngineImpl::initScene() {

    currentScene_ = ObjectPtr<Scene>::make(this);
    Scene *scene = currentScene_.get();

    ObjectPtr<TransformNode> ringGroup = ObjectPtr<TransformNode>::make();

    {
        ObjectPtr<CameraNode> cameraNode = ObjectPtr<CameraNode>::make();
        auto camera = ObjectBase::make<Camera>();
        // this could be cleaned up and done more automatically, but we will have diffent types of cameras !
        cameraNode->setCamera(camera);

        glm::mat4 camPose(1.0);
        camPose = glm::rotate(camPose, glm::pi<float>()/8, glm::vec3(1.0,0,0)); // glm::vec4(1.0,0,-3,1);
        camPose = glm::translate(camPose, glm::vec3(0,1.0,-5.1));

        cameraNode->setLocalTransform(camPose);

        camera->setNearClip(0.01f);
        camera->setFarClip(100.0f);
        camera->setFocalLength(2.0);
        scene->setCurrentCamera(cameraNode);
    }

    scene->addChild(ringGroup);
    {
        class AnimTask
            : public AnimationTask
        {
        public:
            bool onAnimationTick(AnimationTaskContext &ac) override {

                TransformNode *owner = (TransformNode *)ac.owner();

                // Matrix4f lt = ringGroup_->getLocalTransform();
                Matrix4f lt = owner->getLocalTransform();

                Matrix4f rot;
                angle += ac.timing().lastFrameDt() * .01;
                if( angle > (glm::pi<float>()*2)) {
                    angle -= (glm::pi<float>()*2);
                }

                rot = glm::rotate(rot, -angle, glm::vec3(0,1.0,0));
                lt[0] = rot[0];
                lt[1] = rot[1];
                lt[2] = rot[2];

                owner->setLocalTransform(lt);

                return(true);

            }
            float angle = 0.0;
        };
        scene->addAnimationTask(ringGroup, ObjectPtr<AnimTask>::make());
    }

    // this holds references until we assign them to drawable nodes;
    // TODO: store by name in cache ? optional ?
    // Currently unless referenced, when all objects with materials in them
    // are deleted the material goes away too.
    
    std::vector<ObjectPtr<Material>> materials;

    // initialize a scene (big hack)
    {
        // test colors
        Color3f colors[] = {
            { 36,133,234 },
            { 177,20,31 },
            { 203,205,195 },
            { 152, 35, 24 },
            { 154, 169, 153 },
            { 179, 126, 64 },
            { 50, 74, 84 },
            { 226, 175, 105 }
        };

        for(int i = 0; i < (int)ARTD_ARRAY_LENGTH(colors); ++i) {
            materials.push_back(ObjectPtr<Material>::make(this));
            auto pMat = materials[materials.size()-1].get();
            pMat->setDiffuse(colors[i]);
            pMat->setShininess(.001);
        }

        // create/load two meshes cone and sphere

        ObjectPtr<DrawableMesh> coneMesh = meshLoader_->loadMesh("cone");
        if(!coneMesh) {
            return(1);
        }

        ObjectPtr<DrawableMesh> sphereMesh = meshLoader_->loadMesh("sphere");
        if (!sphereMesh) {
            return 1;
        }

        // lay out a bunch of instances

        Matrix4f lt(1.0);
        
        lt = glm::translate(lt, glm::vec3(0.0, 0.0, 3.0));
        ringGroup->setLocalTransform(lt);
        
        AD_LOG(info) << lt;

        
        glm::mat4 drot(1.0);
        drot = glm::rotate(drot, -glm::pi<float>()/6, glm::vec3(0,1.0,0)); // rotate about Y
        
        Vec3f trans = glm::vec3(0.0, 0.0, 3.5);

        AD_LOG(info) << drot;

        // Create some lights
        
        {
            ObjectPtr<LightNode> light = ObjectPtr<LightNode>::make();
            light->setLightType(LightNode::directional);
            light->setDirection(Vec3f(0.5, .5, 0.1));
            light->setDiffuse(Color3f(1.f,1.f,1.f));
            light->setAreaWrap(.25);
            currentScene_->addChild(light);

            // this rotates the light direction around the center of ths scene
            class AnimTask
                : public AnimationTask
            {
            public:
                bool onAnimationTick(AnimationTaskContext &ac) override {

                    TransformNode *owner = (TransformNode *)ac.owner();

                    // Matrix4f lt = ringGroup_->getLocalTransform();
                    Matrix4f lt = owner->getLocalTransform();

                    Matrix4f rot;
                    angle += ac.timing().lastFrameDt() * .2;
                    if( angle > (glm::pi<float>()*2)) {
                        angle -= (glm::pi<float>()*2);
                    }

                    rot = glm::rotate(rot, -angle, glm::vec3(0,1.0,0));
                    lt[0] = rot[0];
                    lt[1] = rot[1];
                    lt[2] = rot[2];

                    owner->setLocalTransform(lt);

                    return(true);

                }
                float angle = 0.0;
            };
            currentScene_->addAnimationTask(light, ObjectPtr<AnimTask>::make());
        }

        // layout some objects in a ring around the ringGroup_ node
        // assign one of the test materials to it.
        uint32_t materialId = 0;
        uint32_t maxI = 12;
        for(uint32_t i = 0; i < maxI; ++i)  {
        
            MeshNode *node = (MeshNode *)ringGroup->addChild(ObjectPtr<MeshNode>::make());
            node->setId(i + 10);

            lt = glm::mat4(1.0);
            lt[3] = glm::vec4(trans,1.0);
            node->setLocalTransform(lt);
            trans = glm::mat3(drot) * trans;
            
            node->setMaterial(materials[materialId]);
            if(++materialId >= materials.size()) {
                materialId = 0;
            }
            
            if((i & 1) != 0) {
                node->setMesh(coneMesh);
            } else {
                node->setMesh(sphereMesh);
            }
        }

        ObjectPtr<DrawableMesh> cubeMesh = meshLoader_->loadMesh("cube");
        if (!cubeMesh) {
            return 1;
        }

        {
            materials.push_back(ObjectBase::make<Material>(this));
            auto pMat = materials[materials.size()-1];
            pMat->setDiffuse(Color3f(180,180,180));
            pMat->setShininess(.001);

            textureManager_->loadBindableTexture("test0", [this, pMat](ObjectPtr<TextureView> tView) {
                if(pMat) {
                    pMat->setDiffuseTex(tView);
                    // todo: creating bindings should be automatic and handled internally
                    // and triggered by modifying values.
                    pMat->bindings_ = createMaterialBindGroup(*(pMat.get()));
                }
            });
            
            lt = glm::mat4(1.0);
            MeshNode *node = (MeshNode *)ringGroup->addChild(ObjectPtr<MeshNode>::make());
            node->setLocalTransform(lt);
            node->setId(maxI + 10);
            node->setMesh(cubeMesh);
            node->setMaterial(pMat);

            {
                class AnimTask
                    : public AnimationTask
                {
                public:

                    bool highlit = false;
                    float toggleTime = 2.1;

                    bool onAnimationTick(AnimationTaskContext &ac) override {
                        
                        MeshNode *owner = (MeshNode *)ac.owner();
                        double dt = ac.timing().lastFrameDt();

                        Matrix4f rot;
                        angle += dt * .1;
                        if( angle > (glm::pi<float>()*2)) {
                            angle -= (glm::pi<float>()*2);
                        }

                        rot = glm::rotate(rot, angle, glm::vec3(0,1.0,0));
                        rot = glm::rotate(rot, angle*2.5f, glm::normalize(glm::vec3(0,1.0,1.0)));
                                
                        owner->setLocalTransform(rot);

                        toggleTime -= dt;
                        if(toggleTime < 0)  {
                            Material *mat = owner->getMaterial().get();
                            highlit = !highlit;
                            if(highlit)  {
                                mat->setEmissive(Color3f(1.f,1.f,1.f));
                                toggleTime = .2;
                            } else {
                                mat->setEmissive(Color3f(0,0,0));
//                                mat->setDiffuse(Color3f(180,180,180));
                                toggleTime += 2.1;
                            }
                        }
                        
                        return(true);

                    }
                    float angle = 0.0;
                };
                
                currentScene_->addAnimationTask(node, ObjectPtr<AnimTask>::make());
            }
        }
    }
    return(0);
}

void
GpuEngineImpl::presentImage(wgpu::TextureView /*texture*/ )  {
    if(headless_) {
        pixelLockLock_.signal();
    } else {
        swapChain.present();
    }
}
wgpu::TextureView
GpuEngineImpl::getNextTexture() {
    if(headless_) {
        return(targetTextureView);
    } else {
        return(swapChain.getCurrentTextureView());
    }
}

int
GpuEngineImpl::getPixels(uint32_t *pBuf) {
    return(pixelGetter_->lockPixels(&pBuf, targetTexture));
}

const int *
GpuEngineImpl::lockPixels(int timeoutMillis) {
    uint32_t * pBuf = nullptr;
    if(pixelLockLock_.waitOnSignal(timeoutMillis) == 0) {
        // we got the lock
        pixelGetter_->lockPixels(&pBuf, targetTexture);
        // AD_LOG(error) << "pixels locked ######### " << (void *)pBuf;
        return((const int *)pBuf);
    }
    return((const int *)pBuf);
}

void
GpuEngineImpl::unlockPixels()  {
    pixelUnLockLock_.signal();
    pixelGetter_->unlockPixels();
}

bool
GpuEngineImpl::processEvents() {

    if(!(inputManager_->pollInputs())) {
        return(false);
    }
    timing_.tickFrame();

    if(!headless_) {
        // this handles being iconified, does not handle window being occluded
        // and does not handle computer going to sleep.

        int visible = (window && glfwGetWindowAttrib(window, GLFW_VISIBLE));
        if(!visible) {
            Thread::sleep(1000/15);
        }
//        if(timing_.isDebugFrame()) {
//            int iconified = glfwGetWindowAttrib(window, GLFW_ICONIFIED);
//            AD_LOG(print) << "iconified: " << iconified << " visible " << visible;
//        }
           
    }

    fpsMonitor_.tickFrame(timing_);
    inputQueue_->executeEvents();
    currentScene_->tickAnimations(timing_);
    updateQueue_->executeEvents();
    return(true);
}
static void doBuf(size_t, Buffer) {
    
}

int
GpuEngineImpl::renderFrame()  {

    // TODO: process input and other events here before
    // we update the transforms.

    if(headless_) {
        // TODO: should we wait here ?? or before processing events and binding instances ??
        // TODO retick animations if waiting time is too long. ( waiting on Java or whatever )
        int ret = pixelUnLockLock_.waitOnSignal(10000);  // 10 seconds enough ? shoudl handle event quent queue in here
        if(ret != 0 && instance) {
            AD_LOG(error) << " signal error " << ret;
            return(0);
        }
        if(!processEvents()) {
            return(1);
        }
    } else {
        if(!processEvents()) {
            return(1);
        }
        if(window && !glfwGetWindowAttrib(window, GLFW_VISIBLE)) {
            return(0);  // don't bother rendering !!
        }
    }

    if(!instance) {
        return(-1);
    }
    
  //  Queue queue = device.getQueue();


    {
        static const int bufferSize = 0x2000;
        std::unique_ptr<uint8_t> workBuffer(new uint8_t[bufferSize]); // todo pick optimum buffer size ?

        // update data on GPU for active (visible) object instances.
 
        // upload active material data array
        {
            
            Buffer iBuffer = materialBuffer_->getBuffer();
            auto offset = materialBuffer_->getStartOffset();

            int uploadCount = 0;   // TODO: for now we just assume the buffer is allocated big enough !!!
            int materialIndex = 0;

            const int maxCount = (int)(bufferSize/sizeof(MaterialShaderData));
            MaterialShaderData *iData = (MaterialShaderData *)workBuffer.get();

            for(auto it = currentScene_->activeMaterials_->begin(); it != currentScene_->activeMaterials_->end(); ++it) {
                Material &mat = *it;

                if(uploadCount < maxCount) {
                    mat.loadShaderData(iData[uploadCount]);
                    mat.setIndex(materialIndex);
                    ++materialIndex;
                    ++uploadCount;
                } else {
                    queue.writeBuffer(iBuffer, offset, &iData[0], uploadCount * sizeof(*iData) );
                    uploadCount = 0;
                }
            }
            if(uploadCount > 0) {
                 queue.writeBuffer(iBuffer, offset, &iData[0], uploadCount * sizeof(*iData) );
            }
        }

        // upload instance data array, done after material indices are assigned and data uploaded
        {
            Buffer iBuffer = instanceBuffer_->getBuffer();
            auto offset = instanceBuffer_->getStartOffset();
            doBuf(offset,iBuffer);

            int countLeft = (int)currentScene_->drawables_.size();
            // temp buffer to upload
            const int maxCount = (int)(bufferSize/sizeof(InstanceData));
            int uploadCount = (int)std::min(maxCount,countLeft);
            InstanceData *iData = (InstanceData *)workBuffer.get();

            while(countLeft > 0) {
                int i = 0;
                
                for(; i < uploadCount; ++i) {
                    currentScene_->drawables_[i]->loadInstanceData(iData[i]);
                }
                countLeft -= i;
                // upload a buffer full
                i *= sizeof(*iData);
                queue.writeBuffer(iBuffer, offset, &iData[0], i );
                offset += i;
                uploadCount = (int)std::min(maxCount,countLeft);
            }
        }
        
        // upload uniforms data "scene globals" PLUS array of lights
        {
            const int headerSize = sizeof(SceneUniforms);
            
            // update the global uniform data - camera transforms, lights etc
            auto camera = currentScene_->currentCamera_->getCamera();
            uniforms.viewMatrix = camera->getView();
            uniforms.projectionMatrix = camera->getProjection();
            uniforms.eyePose = camera->getPose(); // glm::inverse(camera->getView());
            uniforms.vpMatrix = uniforms.projectionMatrix * uniforms.viewMatrix;

            uniforms.numLights = (uint32_t)currentScene_->lights_.size();

            const int initialMax = (int)((bufferSize-headerSize)/sizeof(LightShaderData));
            int countLeft = uniforms.numLights;
            int uploadCount = (int)std::min(initialMax,countLeft);
            

            const int maxCount = (int)(bufferSize/sizeof(LightShaderData));
 
            Buffer iBuffer = uniformBuffer_->getBuffer();
            auto offset = uniformBuffer_->getStartOffset();
            doBuf(offset,iBuffer);

            uint8_t *outBytes = workBuffer.get();
            (*(SceneUniforms *)outBytes) = uniforms;
            outBytes += headerSize;
            
            int lightIx = 0;
                
            for(;;) {
                for(int i = 0; i < uploadCount; ++i) {
                    currentScene_->lights_[lightIx]->loadShaderData(*(LightShaderData*)outBytes);
                    ++lightIx;
                    outBytes += sizeof(LightShaderData);
                }
                if(outBytes > workBuffer.get()) {
                    int writeSize = (int)(outBytes - workBuffer.get());
                    outBytes = workBuffer.get();
                    queue.writeBuffer(iBuffer, offset, outBytes, writeSize );
                    countLeft -= uploadCount;
                    if(countLeft <= 0) {
                        break;
                    }
                    offset += writeSize;
                    uploadCount = (int)std::min(maxCount,countLeft);
                }
            }
        }

    }
    
#ifdef WEBGPU_BACKEND_DAWN
        // Check for pending error callbacks
        device.tick();
#endif
   

    
    wgpu::TextureView nextTexture = getNextTexture();
    if (!nextTexture) {
        AD_LOG(error) << "Cannot acquire next swap chain texture";
        return(1);
    }

    CommandEncoderDescriptor commandEncoderDesc;
    commandEncoderDesc.label = "Command Encoder";
    CommandEncoder encoder = device.createCommandEncoder(commandEncoderDesc);

    RenderPassDescriptor renderPassDesc{};

    renderPassDesc.label = "OneFramePass";
    
    RenderPassColorAttachment renderPassColorAttachment{};
    renderPassColorAttachment.view = nextTexture;
    renderPassColorAttachment.resolveTarget = nullptr;
    renderPassColorAttachment.loadOp = LoadOp::Clear;
    renderPassColorAttachment.storeOp = StoreOp::Store;
    {
        auto &c = currentScene_->backgroundColor_;
        renderPassColorAttachment.clearValue = Color(c.r,c.g,c.b,c.a);
    }
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;

    RenderPassDepthStencilAttachment depthStencilAttachment;
    depthStencilAttachment.view = depthTextureView;
    depthStencilAttachment.depthClearValue = 1.0f;
    depthStencilAttachment.depthLoadOp = LoadOp::Clear;
    depthStencilAttachment.depthStoreOp = StoreOp::Store;
    depthStencilAttachment.depthReadOnly = false;
    depthStencilAttachment.stencilClearValue = 0;
    #ifdef WEBGPU_BACKEND_WGPU
        depthStencilAttachment.stencilLoadOp = LoadOp::Clear;
        depthStencilAttachment.stencilStoreOp = StoreOp::Store;
    #else
        depthStencilAttachment.stencilLoadOp = LoadOp::Undefined;
        depthStencilAttachment.stencilStoreOp = StoreOp::Undefined;
    #endif
    depthStencilAttachment.stencilReadOnly = true;
    renderPassDesc.depthStencilAttachment = &depthStencilAttachment;

    renderPassDesc.timestampWriteCount = 0;
    renderPassDesc.timestampWrites = nullptr;

    RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

    // Select which render pipeline to use
    renderPass.setPipeline(pipeline);

    { // draw the models ( needs to be organized )}

        wgpu::BindGroup lastMaterialBindings = getDefaultMaterial()->getBindings();

//        if(timing().isDebugFrame()) {
//            AD_LOG(info)  << "DEBUG FRAME " <<  timing().frameNumber();
//        }
                
        // set group for scene specific data being used.
        renderPass.setBindGroup(0, bindGroup, 0, nullptr);
        renderPass.setBindGroup(1, lastMaterialBindings, 0, nullptr); // default texture
        for(size_t i = 0; i < currentScene_->drawables_.size(); ++i) {

            auto drawable = currentScene_->drawables_[i];
            Material *matl = drawable->getMaterial().get();
            auto bindings = matl->getBindings();
                        
            if(bindings && ( (void*)bindings) != ((void*)lastMaterialBindings) ) {
//                if(bindings && (bindings != lastMatlBindings) ) {
//                if(bindings && ( ((WGPUBindGroupImpl *)bindings) != (WGPUBindGroupImpl *)lastMatlBindings) ) {
                lastMaterialBindings = bindings;
                renderPass.setBindGroup(1, bindings, 0, nullptr);
            }
            DrawableMesh *mesh = drawable->getMesh();

            
            if(mesh) {
                const BufferChunk &iChunk = mesh->iChunk_;
                const BufferChunk &vChunk = mesh->vChunk_;

                renderPass.setVertexBuffer(0, vChunk.getBuffer(), vChunk.getStartOffset(), vChunk.getSize());
                renderPass.setIndexBuffer(iChunk.getBuffer(), IndexFormat::Uint16, iChunk.getStartOffset(), iChunk.getSize());
                
                renderPass.drawIndexed(mesh->indexCount(), 1, 0, 0, (uint32_t)i);
            }
        }
    }
    
    renderPass.end();

    CommandBufferDescriptor cmdBufferDescriptor{};
    cmdBufferDescriptor.label = "Command buffer";
    CommandBuffer command = encoder.finish(cmdBufferDescriptor);
    queue.submit(command);
    presentImage(nextTexture);


    command.release();
    renderPass.release();
    encoder.release();
//    queue.release();

#ifdef WEBGPU_BACKEND_DAWN
        // Check for pending error callbacks
        device.tick();
#endif
    return(0);
}


ARTD_END

#ifndef _WIN32
    __attribute__((constructor))
    static void initializer(void) {
        AD_LOG(info) << "library loaded into process";
    }

    // Finalizer.
    __attribute__((destructor))
    static void finalizer(void) {
        AD_LOG(info) << "library unloaded from process";
    }
#endif


extern "C" {

    typedef artd::GpuEngineImpl Engine;

    int initGPUTest(int width, int height) {
        AD_LOG(print) << "\n\n\n";
        return(Engine::getInstance().init(true, width,height));
    }

    int renderGPUTestFrame()  {
        return(Engine::getInstance().renderFrame());
    }

    int getPixels(int *pBuf) {
        return(Engine::getInstance().getPixels((uint32_t*)pBuf));
    }

    const int *lockPixels(int timeoutMillis) {
        return(Engine::getInstance().lockPixels(timeoutMillis));
    }

    void unlockPixels() {
        return(Engine::getInstance().unlockPixels());
    }

    void shutdownGPUTest()  {
        AD_LOG(info) << "shutting down WebGPU engine";
        Engine::getInstance().releaseResources();
    }
}
    
class ImageReceiver
    : public artd::Runnable
{
public:
    bool running_ = true;

    typedef artd::GpuEngineImpl Engine;

    void run() {
        
        AD_LOG(info) << "Image receiver thread started !";
  
        artd::Thread::sleep(100);

        while(running_) {
            const int *pPix = Engine::getInstance().lockPixels(100);
            artd::Thread::sleep(5);
            if(pPix) {
                Engine::getInstance().unlockPixels();
            }
        }
    }
};

ARTD_BEGIN

#pragma pack(push,4) // int alignment

struct Fooniforms {
    // scene frame specific items
    glm::mat4x4 projectionMatrix;
    glm::mat4x4 viewMatrix;
    glm::mat4x4 modelMatrix;  // model specific
    int xx_[3];
};

#pragma pack(pop) // back to prior packing

// Have the compiler check byte alignment
//static_assert(sizeof(SceneUniforms) % 16 == 0);

#pragma pack(push,16)

struct Aligned
    : public Fooniforms
{
    
};

#pragma pack(pop)

static void testit() {
    AD_LOG(info) << "\ntsize " << sizeof(Fooniforms)
    << "\n align " << (sizeof(Aligned) % 16);
}


ARTD_END

#include "artd/TypedPropertyMap.h"


extern "C" {

    typedef artd::GpuEngineImpl Engine;

    int runGpuTest(int, char **) {


        using namespace artd;

        TypedPropertyMap::test();


        bool headless = false;
    
        artd::testit();
        
        Engine &test = Engine::getInstance();
        using namespace artd;
    
            
        int ret = test.init(headless, 1920, 1080);

        if(ret != 0) {
            return(ret);
        }

        ObjectPtr<ImageReceiver> receiver;
        ObjectPtr<Thread> thread;

        if(headless) {
            receiver = ObjectPtr<ImageReceiver>::make();
            thread = ObjectPtr<Thread>::make(receiver);
            thread->start();
        }
        
        for(;;) {
            ret = test.renderFrame();
            if(ret != 0) {
                break;
            }
        }
        if(headless) {
            receiver->running_ = false;
            thread->join(5000);
        }
        test.releaseResources();
        return(ret);
    }

}

