/**
 * This file is originally base on  the  "Learn WebGPU for C++" book.
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
#include "artd/GpuEngine-PanamaExports.h"


ARTD_BEGIN

using namespace wgpu;


GpuEngine::GpuEngine() {

};
GpuEngine::~GpuEngine() {

};


namespace fs = std::filesystem;

int GpuEngineImpl::init(bool headless, int width, int height) {
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

    if(!headless_) {
        if (!glfwInit()) {
            std::cerr << "Could not initialize GLFW!" << std::endl;
            return 1;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(width_, height_, "Learn WebGPU", NULL, NULL);
        if (!window) {
            std::cerr << "Could not open window!" << std::endl;
            return 1;
        }
        surface = glfwGetWGPUSurface(instance, window);
    } else {
        surface = nullptr;  // done in constructor
    }

    AD_LOG(info) << "Requesting adapter...";

    RequestAdapterOptions adapterOpts{};
    adapterOpts.compatibleSurface = surface;

    adapter = instance.requestAdapter(adapterOpts);
    std::cout << "Got adapter: " << adapter << std::endl;

    // specify device limits - we need to figure out a dynamic or "max" way of setting this up.
// 056
    SupportedLimits supportedLimits;
    adapter.getLimits(&supportedLimits);

    AD_LOG(info) << "Requesting device..." << std::endl;
    RequiredLimits requiredLimits = Default;
    requiredLimits.limits.maxVertexAttributes = 3;
    //                                          ^ This was a 2
    requiredLimits.limits.maxVertexBuffers = 1;
    requiredLimits.limits.maxBufferSize = 16 * sizeof(VertexAttributes);
    //                                         ^^^^^^^^^^^^^^^^^^^^^^^^ This was 6 * sizeof(float)
    requiredLimits.limits.maxVertexBufferArrayStride = sizeof(VertexAttributes);
    //                                                        ^^^^^^^^^^^^^^^^^^^^^^^^ This was 6 * sizeof(float)
    requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
    requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
    requiredLimits.limits.maxInterStageShaderComponents = 6;
    //                                                    ^ This was a 3
    requiredLimits.limits.maxBindGroups = 1;
    requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
    // Update max uniform buffer size:
    requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);

    // so is this a definition of the maximum renderer viewport size ?
    requiredLimits.limits.maxTextureDimension1D = height_;
    requiredLimits.limits.maxTextureDimension2D = width_;
    requiredLimits.limits.maxTextureArrayLayers = 1;
// e 056

    DeviceDescriptor deviceDesc;
    deviceDesc.label = "My Device";
    deviceDesc.requiredFeaturesCount = 0;

    deviceDesc.requiredLimits = &requiredLimits;
    deviceDesc.defaultQueue.label = "The default queue";
    device = adapter.requestDevice(deviceDesc);
    AD_LOG(info) << "Got device: " << device << std::endl;

    // Add an error callback for more debug info
    errorCallback_ = device.setUncapturedErrorCallback([](ErrorType type, char const* message) {
        AD_LOG(error) << "Device error: type " << type << "(" << (message ? message : "" ) << ")";
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

	AD_LOG(info) << "Creating shader module...";
	shaderModule = shaderManager_->loadShaderModule("pyramid056.wgsl");
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

	// Color attribute
	vertexAttribs[2].shaderLocation = 2;
	vertexAttribs[2].format = VertexFormat::Float32x3;
	vertexAttribs[2].offset = offsetof(VertexAttributes, color);

	VertexBufferLayout vertexBufferLayout;
	vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
	vertexBufferLayout.attributes = vertexAttribs.data();
	vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
	//                               ^^^^^^^^^^^^^^^^^^^^^^^^ This was 6 * sizeof(float)
	vertexBufferLayout.stepMode = VertexStepMode::Vertex;

	pipelineDesc.vertex.bufferCount = 1;
	pipelineDesc.vertex.buffers = &vertexBufferLayout;

    // Vertex shader
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;

    // Primitive assembly and rasterization
    // Each sequence of 3 vertices is considered as a triangle
    pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;
    // We'll see later how to specify the order in which vertices should be
    // connected. When not specified, vertices are considered sequentially.
    pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
    // The face orientation is defined by assuming that when looking
    // from the front of the face, its corner vertices are enumerated
    // in the counter-clockwise (CCW) order.
    //    pipelineDesc.primitive.frontFace = FrontFace::CCW;
        pipelineDesc.primitive.frontFace = FrontFace::CW;
    // But the face orientation does not matter much because we do not
    // cull (i.e. "hide") the faces pointing away from us (which is often
    // used for optimization).
    pipelineDesc.primitive.cullMode = CullMode::None;

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

	static DepthStencilState depthStencilState = Default;
	depthStencilState.depthCompare = CompareFunction::Less;
	depthStencilState.depthWriteEnabled = true;
	TextureFormat depthTextureFormat = TextureFormat::Depth24Plus;
	depthStencilState.format = depthTextureFormat;
	depthStencilState.stencilReadMask = 0;
	depthStencilState.stencilWriteMask = 0;

    pipelineDesc.depthStencil = &depthStencilState;

    // Multi-sampling
    // Samples per pixel
    pipelineDesc.multisample.count = 1;
    // Default value for the mask, meaning "all bits on"
    pipelineDesc.multisample.mask = ~0u;
    // Default value as well (irrelevant for count = 1 anyways)
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    // Create binding layout (don't forget to = Default)
    BindGroupLayoutEntry bindingLayout = Default;
    bindingLayout.binding = 0;
    bindingLayout.visibility = ShaderStage::Vertex | ShaderStage::Fragment;
    bindingLayout.buffer.type = BufferBindingType::Uniform;
    bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);

    // Create a bind group layout
    BindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries = &bindingLayout;
    BindGroupLayout bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

    // Create the pipeline layout
    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)&bindGroupLayout;
    static PipelineLayout layout = device.createPipelineLayout(layoutDesc);
    // Pipeline layout
    pipelineDesc.layout = layout;

    pipeline = device.createRenderPipeline(pipelineDesc);
    AD_LOG(info) << "Render pipeline: " << pipeline;

	// Create the depth texture
	TextureDescriptor depthTextureDesc;
	depthTextureDesc.dimension = TextureDimension::_2D;
	depthTextureDesc.format = depthTextureFormat;
	depthTextureDesc.mipLevelCount = 1;
	depthTextureDesc.sampleCount = 1;
	depthTextureDesc.size = {width_, height_, 1};
	depthTextureDesc.usage = TextureUsage::RenderAttachment;
	depthTextureDesc.viewFormatCount = 1;
	depthTextureDesc.viewFormats = (WGPUTextureFormat*)&depthTextureFormat;
	depthTexture = device.createTexture(depthTextureDesc);
	std::cout << "Depth texture: " << depthTexture << std::endl;

	// Create the view of the depth texture manipulated by the rasterizer
	TextureViewDescriptor depthTextureViewDesc;
	depthTextureViewDesc.aspect = TextureAspect::DepthOnly;
	depthTextureViewDesc.baseArrayLayer = 0;
	depthTextureViewDesc.arrayLayerCount = 1;
	depthTextureViewDesc.baseMipLevel = 0;
	depthTextureViewDesc.mipLevelCount = 1;
	depthTextureViewDesc.dimension = TextureViewDimension::_2D;
	depthTextureViewDesc.format = depthTextureFormat;
	depthTextureView = depthTexture.createView(depthTextureViewDesc);
	std::cout << "Depth texture view: " << depthTextureView << std::endl;

	bool success = meshLoader_->loadGeometry("cone", pointData, indexData, 6 /* dimensions */);
	//                                                                             ^ This was a 3
	if (!success) {
		std::cerr << "Could not load geometry!" << std::endl;
		return 1;
	}

	// Create vertex buffer
	static BufferDescriptor bufferDesc;
	bufferDesc.size = pointData.size() * sizeof(float);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
	bufferDesc.mappedAtCreation = false;
	vertexBuffer = device.createBuffer(bufferDesc);
	queue.writeBuffer(vertexBuffer, 0, pointData.data(), bufferDesc.size);

	// int indexCount = static_cast<int>(indexData.size());

	// Create index buffer
	bufferDesc.size = indexData.size() * sizeof(float);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
	bufferDesc.mappedAtCreation = false;
	indexBuffer = device.createBuffer(bufferDesc);
	queue.writeBuffer(indexBuffer, 0, indexData.data(), bufferDesc.size);

	// Create uniform buffer
	bufferDesc.size = sizeof(MyUniforms);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	bufferDesc.mappedAtCreation = false;
	uniformBuffer = device.createBuffer(bufferDesc);

	// Build transform matrices

	// Rotate the object
	// float angle1 = 2.0f; // arbitrary time
            
	// model transform
	S = glm::scale(glm::mat4x4(1.0), glm::vec3(0.3f));
	T1 = glm::translate(glm::mat4x4(1.0), glm::vec3(0.5, 0.0, 0.0));

	glm::mat4x4 M(1.0);
	// M = glm::rotate(M, angle1, glm::vec3(0.0, 1.0, 0.0));
	M = glm::translate(M, glm::vec3(0.0, 0.0, 1.0));
	M = glm::scale(M, glm::vec3(0.9f));
	uniforms.modelMatrix = M;

    // setup camera
    {
        glm::mat4 camPose(1.0);
        camPose[3] = glm::vec4(0,0,-3,1);
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
    
    // not really needed here first thing done when rendering a frame
	uniforms.time = 1.0f;
	uniforms.color = { 0.0f, 1.0f, 0.4f, 1.0f };
	queue.writeBuffer(uniformBuffer, 0, &uniforms, sizeof(MyUniforms));

	// Create a binding
	BindGroupEntry binding{};
	binding.binding = 0;
	binding.buffer = uniformBuffer;
	binding.offset = 0;
	binding.size = sizeof(MyUniforms);

	// A bind group contains one or multiple bindings
	BindGroupDescriptor bindGroupDesc;
	bindGroupDesc.layout = bindGroupLayout;
	bindGroupDesc.entryCount = bindGroupLayoutDesc.entryCount;
	bindGroupDesc.entries = &binding;
	bindGroup = device.createBindGroup(bindGroupDesc);


    if(headless_) {
        pixelGetter_ = artd::ObjectBase::make<PixelReader>(device, width_,height_);
        pixelUnLockLock_.signal(); // start enabled !!
    }
    return(0);
}

void
GpuEngineImpl::presentImage(TextureView /*texture*/ )  {
    if(headless_) {
        pixelLockLock_.signal();
    } else {
        swapChain.present();
    }
}
TextureView
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


int
GpuEngineImpl::renderFrame()  {

    if(headless_) {

        int ret = pixelUnLockLock_.waitOnSignal(1000);
        if(ret != 0) {
            AD_LOG(error) << " signal error " << ret;
        }

    } else {
        glfwPollEvents();
        if(glfwWindowShouldClose(window)) {
            return(1);
        }
    }
    if(!instance) {
        return(-1);
    }

    // Update per frame uniforms  TODO: now includes model matrix for one model.
    uniforms.time = static_cast<float>(glfwGetTime()); // glfwGetTime returns a double
    // Only update the 1-st float of the buffer

    {
        auto camera = camNode_->getCamera();
        uniforms.viewMatrix = camera->getView();
        uniforms.projectionMatrix = camera->getProjection();
        
        queue.writeBuffer(uniformBuffer, 0, &uniforms, offsetof(MyUniforms, _pad[0]));
    }
    
        // Update view matrix
//        float angle1 = uniforms.time;
//        auto R1 = glm::rotate(glm::mat4x4(1.0), angle1, glm::vec3(0.0, 0.0, 1.0));
//        uniforms.modelMatrix = R1 * T1 * S;
//        queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, modelMatrix), &uniforms.modelMatrix, sizeof(MyUniforms::modelMatrix));
    

    TextureView nextTexture = getNextTexture();
    if (!nextTexture) {
        AD_LOG(error) << "Cannot acquire next swap chain texture";
        return(1);
    }

    CommandEncoderDescriptor commandEncoderDesc;
    commandEncoderDesc.label = "Command Encoder";
    CommandEncoder encoder = device.createCommandEncoder(commandEncoderDesc);

    RenderPassDescriptor renderPassDesc{};

    RenderPassColorAttachment renderPassColorAttachment{};
    renderPassColorAttachment.view = nextTexture;
    renderPassColorAttachment.resolveTarget = nullptr;
    renderPassColorAttachment.loadOp = LoadOp::Clear;
    renderPassColorAttachment.storeOp = StoreOp::Store;
    renderPassColorAttachment.clearValue = Color{ 0.3, 0.3, 0.3, 1.0 }; // background color
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;

// 056
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
// e 056

    renderPassDesc.timestampWriteCount = 0;
    renderPassDesc.timestampWrites = nullptr;
    RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

    // In its overall outline, drawing a triangle is as simple as this:
    // Select which render pipeline to use
    renderPass.setPipeline(pipeline);
// 056

		renderPass.setVertexBuffer(0, vertexBuffer, 0, pointData.size() * sizeof(float));
		renderPass.setIndexBuffer(indexBuffer, IndexFormat::Uint16, 0, indexData.size() * sizeof(uint16_t));

		// Set binding group
		renderPass.setBindGroup(0, bindGroup, 0, nullptr);

		renderPass.drawIndexed((int)indexData.size(), 1, 0, 0, 0);

		renderPass.end();

		nextTexture.release();

		CommandBufferDescriptor cmdBufferDescriptor{};
		cmdBufferDescriptor.label = "Command buffer";
		CommandBuffer command = encoder.finish(cmdBufferDescriptor);
		queue.submit(command);
// e 056
        presentImage(nextTexture);

#ifdef WEBGPU_BACKEND_DAWN
		// Check for pending error callbacks
		device.tick();
#endif

    return(0);
}

void
GpuEngineImpl::releaseResources() {
    if(instance) {

        if(vertexBuffer) {
            vertexBuffer.destroy();
            vertexBuffer.release();
        }
        if(indexBuffer) {
            indexBuffer.destroy();
            indexBuffer.release();
        }
        if(depthTextureView) {
            depthTextureView.release();
        }
        if(depthTexture) {
            depthTexture.destroy();
            depthTexture.release();
        }
        meshLoader_ = nullptr;
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

ARTD_END

#ifndef _WIN32
    __attribute__((constructor))
    static void initializer(void) {
        AD_LOG(info) << "loaded into process";
    }

    // Finalizer.
    __attribute__((destructor))
    static void finalizer(void) {
        AD_LOG(info) << "unloaded from process";
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

    int runGpuTest(int, char **) {
        Engine &test = Engine::getInstance();
        
        int ret = test.init(false, 1920, 1080);
        //     int ret = test.init(false, 640, 480);
        if(ret != 0) {
            return(ret);
        }
        
        for(;;) {
            ret = test.renderFrame();
            if(ret != 0) {
                break;
            }
        }
        test.releaseResources();
        return(ret);
    }

}

