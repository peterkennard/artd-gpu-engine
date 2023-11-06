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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
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
    viewport_ = ObjectBase::make<Viewport>();
    camNode_ = ObjectBase::make<CameraNode>();
    auto cam = ObjectBase::make<Camera>();
    camNode_->setCamera(cam);
    meshLoader_ = ObjectBase::make<CachedMeshLoader>();
    cam->setViewport(viewport_);
    // TODO: we need a scene
    ringGroup_ = ObjectBase::make<TransformNode>();
    std::memset(&uniforms,0,sizeof(uniforms));
}

GpuEngineImpl::~GpuEngineImpl()
{
    releaseResources();
}
void
GpuEngineImpl::releaseResources() {
    if(instance) {

        if(depthTextureView) {
            depthTextureView.release();
        }
        if(depthTexture) {
            depthTexture.destroy();
            depthTexture.release();
        }

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


int GpuEngineImpl::init(bool headless, int width, int height) {
    
    {
        auto eventPool = ObjectBase::make<LambdaEventQueue::LambdaEventPool>();
 
        inputQueue_ = ObjectBase::make<LambdaEventQueue>(eventPool);
        updateQueue_ = ObjectBase::make<LambdaEventQueue>(eventPool);
    }
    
    
    
    width_ = width;
    height_ = height;
    headless_ = headless;
    
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

    inputManager_ = ObjectBase::make<InputManager>(this);
    
    if(headless_) {
        surface = nullptr;  // done in constructor
    } else {
        int ret = initGlfwDisplay();
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
    requiredLimits.limits.maxBufferSize = 128 * sizeof(VertexAttributes);
    
    requiredLimits.limits.maxVertexBufferArrayStride = sizeof(VertexAttributes) * 2;  // cx ? does bigger hurt
    
    requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
    requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
    requiredLimits.limits.maxInterStageShaderComponents = 16; // this was 3 PK
    
    requiredLimits.limits.maxBindGroups = 4;  // max supported seems to be 4
    requiredLimits.limits.maxUniformBuffersPerShaderStage = 8; // 12 is max here
    // Update max uniform buffer size:
    requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float) * 2;  // 2x added enough ?

    requiredLimits.limits.maxSamplersPerShaderStage = 1;

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

    
    shaderManager_ = ObjectBase::make<ShaderManager>(device);
    
    queue = device.getQueue();
    
    TextureFormat swapChainFormat = TextureFormat::BGRA8Unorm;
    
    if(headless_) {
        // No more swap chain, but still a target format and a target texture
        
        TextureDescriptor targetTextureDesc;
        targetTextureDesc.label = "Render texture";
        targetTextureDesc.dimension = TextureDimension::_2D;
        // Any size works here, this is the equivalent of the window size
        targetTextureDesc.size = { width_, height_, 1 };
        // Use the same format here and in the render pipeline's color target
        targetTextureDesc.format = swapChainFormat;
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
        swapChainDesc.width = width_;
        swapChainDesc.height = height_;
        swapChainDesc.usage = TextureUsage::RenderAttachment;
        swapChainDesc.format = swapChainFormat;
        swapChainDesc.presentMode = PresentMode::Fifo;
        swapChain = device.createSwapChain(surface, swapChainDesc);
        AD_LOG(info) << "Swapchain created " << swapChain;
    }
    
    // ****** shader module part
    resourceManager_ = ResourceManager::create();
    textureManager_ = TextureManager::create(this);

    pickerPass_ = ObjectBase::make<PickerPass>(this);
    
    AD_LOG(info) << "Creating shader module...";
    shaderModule = shaderManager_->loadShaderModule("testShader1.wgsl");
    AD_LOG(info)  << "Shader module: " << shaderModule;
    
    AD_LOG(info) << "Creating render pipeline...";
    RenderPipelineDescriptor pipelineDesc;
    
    // Vertex fetch  ###########
    static std::vector<VertexAttribute> vertexAttribs(3);  // should hold reference ???
    //                                         ^ This was a 2
    
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
    vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
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
    colorTarget.format = swapChainFormat;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = ColorWriteMask::All; // We could write to only some of the color channels.
    
    // We have only one target because our render pass has only one output color
    // attachment.
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    
    TextureFormat depthTextureFormat = TextureFormat::Depth24Plus;
    
    DepthStencilState depthStencilState = Default;
    depthStencilState.depthCompare = CompareFunction::Less;
    depthStencilState.depthWriteEnabled = true;
    depthStencilState.format = depthTextureFormat;
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
    BindGroupLayoutEntry bindingLayouts[3];
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
    }
    
    // Create a bind group layout
    BindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.entryCount = 3; // todo take from data struct
    bindGroupLayoutDesc.entries =  bindingLayouts;
    BindGroupLayout bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

// create bind group layout for texture !
#ifdef WEBGPU_BACKEND_DAWN
        // Check for pending error callbacks
        device.tick();
#endif

    
	// The texture binding
    
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
    BindGroupLayout textureBindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc1);
        
#ifdef WEBGPU_BACKEND_DAWN
        // Check for pending error callbacks
        device.tick();
#endif

    // a sampler binding

    {
        // Create a sampler
//        SamplerDescriptor samplerDesc;
//        samplerDesc.addressModeU = AddressMode::ClampToEdge;
//        samplerDesc.addressModeV = AddressMode::ClampToEdge;
//        samplerDesc.addressModeW = AddressMode::ClampToEdge;
//        samplerDesc.magFilter = FilterMode::Linear;
//        samplerDesc.minFilter = FilterMode::Linear;
//        samplerDesc.mipmapFilter = MipmapFilterMode::Linear;
//        samplerDesc.lodMinClamp = 0.0f;
//        samplerDesc.lodMaxClamp = 1.0f;
//        samplerDesc.compare = CompareFunction::Undefined;
//        samplerDesc.maxAnisotropy = 1;
//        Sampler sampler = device.createSampler(samplerDesc);


        // The texture sampler binding
//        BindGroupLayoutEntry& samplerBindingLayout = bindingLayoutEntries[2];
//        samplerBindingLayout.binding = 2;
//        samplerBindingLayout.visibility = ShaderStage::Fragment;
//        samplerBindingLayout.sampler.type = SamplerBindingType::Filtering;
    }


    // Create the pipeline layout
    {
        BindGroupLayout layouts[2] { bindGroupLayout, textureBindGroupLayout  };
        
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

    // Create the depth texture
	{
        TextureDescriptor depthTextureDesc;
        depthTextureDesc.dimension = TextureDimension::_2D;
        depthTextureDesc.format = depthTextureFormat;
        depthTextureDesc.mipLevelCount = 1;
        depthTextureDesc.sampleCount = 1;
        depthTextureDesc.size = {width_, height_, 1};
        depthTextureDesc.usage = TextureUsage::RenderAttachment;
        depthTextureDesc.viewFormatCount = 1;
        depthTextureDesc.viewFormats = (WGPUTextureFormat*)&depthTextureFormat;
        AD_LOG(info) << "Creating Depth texture";
        depthTexture = device.createTexture(depthTextureDesc);
        AD_LOG(info) << "Depth texture: " << depthTexture;
    }

	// Create the view of the depth texture manipulated by the rasterizer
    {
        TextureViewDescriptor depthTextureViewDesc;
        depthTextureViewDesc.aspect = TextureAspect::DepthOnly;
        depthTextureViewDesc.baseArrayLayer = 0;
        depthTextureViewDesc.arrayLayerCount = 1;
        depthTextureViewDesc.baseMipLevel = 0;
        depthTextureViewDesc.mipLevelCount = 1;
        depthTextureViewDesc.dimension = TextureViewDimension::_2D;
        depthTextureViewDesc.format = depthTextureFormat;
        AD_LOG(info) << "Creating Depth texture view";
        depthTextureView = depthTexture.createView(depthTextureViewDesc);
        AD_LOG(info) << "Depth texture view: " << depthTextureView;
    }

    // not really needed here first thing done when rendering a frame
	uniforms.time = 1.0f;

	// Create bindings for test objects
	{
        uniformBuffer_ = bufferManager_->allocUniformChunk(sizeof(SceneUniforms) + (64 * sizeof(LightShaderData)) );
        
        BindGroupEntry bindings[3];

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

        // A bind group contains one or multiple bindings
        BindGroupDescriptor bindGroupDesc;
        bindGroupDesc.layout = bindGroupLayout;
        bindGroupDesc.entryCount = 3;
        bindGroupDesc.entries = bindings;
        bindGroup = device.createBindGroup(bindGroupDesc);
    }

#ifdef WEBGPU_BACKEND_DAWN
        // Check for pending error callbacks
        device.tick();
#endif

    {
        BindGroupEntry bindings[1];

	    bindings[0].binding = 0;
        bindings[0].textureView = pickerPass_->getTextureView();
        
        BindGroupDescriptor bindGroupDesc;
        bindGroupDesc.layout = textureBindGroupLayout;
        bindGroupDesc.entryCount = 1;
        bindGroupDesc.entries = bindings;

        textureBindGroup = device.createBindGroup(bindGroupDesc);
    }



#ifdef WEBGPU_BACKEND_DAWN
        // Check for pending error callbacks
        device.tick();
#endif

    if(headless_) {
        pixelGetter_ = artd::ObjectBase::make<PixelReader>(device, width_,height_);
        pixelUnLockLock_.signal(); // start enabled !!
    }


    // setup camera
    {
        glm::mat4 camPose(1.0);
        camPose = glm::rotate(camPose, glm::pi<float>()/8, glm::vec3(1.0,0,0)); // glm::vec4(1.0,0,-3,1);
        camPose = glm::translate(camPose, glm::vec3(0,1.0,-5.1));

        camNode_->setLocalTransform(camPose);
        auto camera = camNode_->getCamera();
        camera->setNearClip(0.01f);
        camera->setFarClip(100.0f);
        camera->setFocalLength(2.0);
        viewport_->setRect(0,0,width_,height_);
        camera->setViewport(viewport_);

        uniforms.viewMatrix = camera->getView();
        uniforms.projectionMatrix = camera->getProjection();
    }

    // initialize a scene (big hack)
    {
        // create some simple test materials.
        materials_.clear();
        
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
            materials_.push_back(ObjectBase::make<Material>(this));
            auto pMat = materials_[i].get();
            pMat->setDiffuse(colors[i]);
            pMat->setId(i);
        }

        textureManager_->loadTexture("test0", [](ObjectPtr<Texture> loaded){
            AD_LOG(print) << "Texture:  " << loaded->getName() << " loaded";
        });
        
        textureManager_->loadTexture(RcString("default"), [](ObjectPtr<Texture> loaded){
            AD_LOG(info) << loaded;
        });
        
        // create two meshes cube and cone

        std::vector<float> pointData;
        std::vector<uint16_t> indexData;

        ObjectPtr<DrawableMesh> coneMesh;

    	bool success = meshLoader_->loadGeometry("cone", pointData, indexData, 6 /* dimensions */);
    	if (!success) {
    		AD_LOG(info) << "Could not load geometry!";
    		return 1;
    	}

        coneMesh = ObjectBase::make<DrawableMesh>();
        coneMesh->indexCount_ = (int)indexData.size();
        coneMesh->iChunk_ = bufferManager_->allocIndexChunk((int)indexData.size(), (const uint16_t *)(indexData.data()));
        coneMesh->vChunk_ = bufferManager_->allocVertexChunk((int)pointData.size(), pointData.data());

        ObjectPtr<DrawableMesh> cubeMesh;

    	if (!success) {
    		AD_LOG(info) << "Could not load geometry!";
    		return 1;
    	}

        //        success = meshLoader_->loadGeometry("cube", pointData, indexData, 6 /* dimensions */);
        success = meshLoader_->loadGeometry("sphere", pointData, indexData, 6 /* dimensions */);

        cubeMesh = ObjectBase::make<DrawableMesh>();
        cubeMesh->indexCount_ = (int)indexData.size();
        cubeMesh->iChunk_ = bufferManager_->allocIndexChunk((int)indexData.size(), (const uint16_t *)(indexData.data()));
        cubeMesh->vChunk_ = bufferManager_->allocVertexChunk((int)pointData.size(), pointData.data());

        // lay them out

        Matrix4f lt(1.0);
        
        lt = glm::translate(lt, glm::vec3(0.0, 0.0, 3.0));
        ringGroup_->setLocalTransform(lt);
        
        AD_LOG(info) << lt;

        
        glm::mat4 drot(1.0);
        drot = glm::rotate(drot, -glm::pi<float>()/6, glm::vec3(0,1.0,0)); // rotate about Y
        
        Vec3f trans = glm::vec3(0.0, 0.0, 3.5);

        AD_LOG(info) << drot;

        // create a test "Scene"
        // Create some lights
        
        {
            lights_.clear();
            ObjectPtr<LightNode> light = ObjectBase::make<LightNode>();
            light->setLightType(LightNode::directional);
            light->setDirection(Vec3f(0.5, .5, 0.1));
            light->setDiffuse(Color3f(1.f,1.f,1.f));
            
            lights_.push_back(light);
        }
        
        // layout some objects in a ring around the ringGroup_ node
        // assign one of the test materials to it.
        uint32_t materialId = 0;
        for(uint32_t i = 0; i < 12; ++i)  {
        
            MeshNode *node = (MeshNode *)ringGroup_->addChild(ObjectBase::make<MeshNode>());
            node->setId(i + 10);

            lt = glm::mat4(1.0);
            lt[3] = glm::vec4(trans,1.0);
            drawables_.push_back(node);  // add to bodgy list of visible drawables
            node->setLocalTransform(lt);
            trans = glm::mat3(drot) * trans;
            
            node->setMaterial(materials_[materialId]);
            if(++materialId >= materials_.size()) {
                materialId = 0;
            }
            
            if((i & 1) != 0) {
                node->setMesh(coneMesh);
            } else {
                node->setMesh(cubeMesh);
            }
        }
    }

#ifdef WEBGPU_BACKEND_DAWN
        // Check for pending error callbacks
        device.tick();
#endif

    AD_LOG(info) << "init complete !";
    timing_.init(0);
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
    
    if(!headless_) {
        glfwPollEvents();
        if(glfwWindowShouldClose(window)) {
            return(false);
        }
    }
    timing_.tickFrame();
    fpsMonitor_.tickFrame(timing_);
    inputQueue_->executeEvents();
    updateQueue_->executeEvents();
    return(true);
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
    }

    if(!instance) {
        return(-1);
    }

    // This is here so we have some animation for updating the secne graph test :)
    // we need animation tasks
    {
        static float angle = 0; // bodge for test
        
        // Matrix4f lt = ringGroup_->getLocalTransform();
        Matrix4f lt = lights_[0]->getLocalTransform();

        Matrix4f rot;
        angle += timing_.lastFrameDt() * .2;
        if( angle > (glm::pi<float>()*2)) {
            angle -= (glm::pi<float>()*2);
        }
        
        rot = glm::rotate(rot, -angle, glm::vec3(0,1.0,0));
        lt[0] = rot[0];
        lt[1] = rot[1];
        lt[2] = rot[2];


        lights_[0]->setLocalTransform(lt);
    //    ringGroup_->setLocalTransform(lt);
    }

    {
        static const int bufferSize = 0x2000;
        std::unique_ptr<uint8_t> workBuffer(new uint8_t[bufferSize]); // todo pick optimum buffer size ?

        // update data on GPU for active (visible) object instances.
 
        // upload material data array
        {
            Buffer iBuffer = materialBuffer_->getBuffer();
            auto offset = materialBuffer_->getStartOffset();

            int countLeft = (int)materials_.size();
            // temp buffer to upload
            const int maxCount = (int)(bufferSize/sizeof(MaterialShaderData));
            int uploadCount = (int)std::min(maxCount,countLeft);
            MaterialShaderData *iData = (MaterialShaderData *)workBuffer.get();

            while(countLeft > 0) {
                int i = 0;
                
                for(; i < uploadCount; ++i) {
                    materials_[i]->loadShaderData(iData[i]);
                }
                countLeft -= i;
                // upload a buffer full
                i *= sizeof(*iData);
                queue.writeBuffer(iBuffer, offset, &iData[0], i );
                offset += i;
                uploadCount = (int)std::min(maxCount,countLeft);
            }
        }

        
        // upload instance data array
        {
            Buffer iBuffer = instanceBuffer_->getBuffer();
            auto offset = instanceBuffer_->getStartOffset();

            int countLeft = (int)drawables_.size();
            // temp buffer to upload
            const int maxCount = (int)(bufferSize/sizeof(InstanceData));
            int uploadCount = (int)std::min(maxCount,countLeft);
            InstanceData *iData = (InstanceData *)workBuffer.get();

            while(countLeft > 0) {
                int i = 0;
                
                for(; i < uploadCount; ++i) {
                    drawables_[i]->loadInstanceData(iData[i]);
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
            auto camera = camNode_->getCamera();
            uniforms.viewMatrix = camera->getView();
            uniforms.projectionMatrix = camera->getProjection();
            uniforms.eyePose = camera->getPose(); // glm::inverse(camera->getView());
            uniforms.vpMatrix = uniforms.projectionMatrix * uniforms.viewMatrix;

            uniforms.numLights = (uint32_t)lights_.size();

            const int initialMax = (int)((bufferSize-headerSize)/sizeof(LightShaderData));
            int countLeft = (int)lights_.size();
            int uploadCount = (int)std::min(initialMax,countLeft);
            

            const int maxCount = (int)(bufferSize/sizeof(LightShaderData));
 
            Buffer iBuffer = uniformBuffer_->getBuffer();
            auto offset = uniformBuffer_->getStartOffset();

            uint8_t *outBytes = workBuffer.get();
            (*(SceneUniforms *)outBytes) = uniforms;
            outBytes += headerSize;
            
            int lightIx = 0;
                
            for(;;) {
                for(int i = 0; i < uploadCount; ++i) {
                    lights_[lightIx]->loadShaderData(*(LightShaderData*)outBytes);
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
    renderPassColorAttachment.clearValue = Color{ 0.3, 0.3, 0.3, 1.0 }; // background color
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

        wgpu::BindGroup lastMatlBindings = textureBindGroup;

//        if(timing().isDebugFrame()) {
//            AD_LOG(info)  << "DEBUG FRAME " <<  timing().frameNumber();
//        }
                
        // set group for scene specific data being used.
        renderPass.setBindGroup(0, bindGroup, 0, nullptr);
        renderPass.setBindGroup(1, lastMatlBindings, 0, nullptr); // default texture

        for(size_t i = 0; i < drawables_.size(); ++i) {

            auto drawable = drawables_[i];
            Material *matl = drawable->getMaterial().get();
            auto bindings = matl->getBindings();
                        
            if(bindings && ( (void*)bindings) != ((void*)lastMatlBindings) ) {
//                if(bindings && ( ((WGPUBindGroupImpl *)bindings) != (WGPUBindGroupImpl *)lastMatlBindings) ) {
                lastMatlBindings = bindings;
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

    if(!headless_) {
        nextTexture.release();
    }

    CommandBufferDescriptor cmdBufferDescriptor{};
    cmdBufferDescriptor.label = "Command buffer";
    CommandBuffer command = encoder.finish(cmdBufferDescriptor);
    queue.submit(command);

    presentImage(nextTexture);


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


        artd::TypedPropertyMap::test();


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
            receiver = ObjectBase::make<ImageReceiver>();
            thread = ObjectBase::make<Thread>(receiver);
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

