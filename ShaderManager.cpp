/**
 * This code ( snippet from ) is part of the "Learn WebGPU for C++" book.
 *   https://github.com/eliemichel/LearnWebGPU
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

#include "artd/ShaderManager.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

ARTD_BEGIN

ShaderManager::ShaderManager(Device device)
    : device_(device)
{
}

static const char* pyramid056ShaderSource = R"(

struct VertexInput {
	@builtin(instance_index) instanceIx: u32,
	@location(0) position: vec3f,
	@location(1) normal: vec3f, // new attribute
	@location(2) color: vec3f,
};

/**
 * A structure holding the value of scene global uniforms
 */
struct SceneUniforms {
    projectionMatrix: mat4x4f,
    viewMatrix: mat4x4f,
    modelMatrix: mat4x4f,
    color: vec4f,
    time: f32,
};

struct InstanceData {
    modelMatrix: mat4x4f,
    materialId: u32,
    objectId: u32,
    unused1_: u32, // pad for 16 byte alignment
    unused2_: u32, // pad for 16 bytes alignment
};

struct MaterialData {
    diffuse: vec3f,
    unused_: f32,  // id ? we need transparency ??
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) normal: vec3f,
    @location(1) texUV:  vec2f,  // TBD unused now
    @location(2) @interpolate(flat) materialIx: u32
};



// Bound variable is a struct
@group(0) @binding(0) var<uniform> uMyUniforms: SceneUniforms;
// TODO: not sure if this is right for the array guessing
@group(0) @binding(1) var<storage> instanceArray : array<InstanceData>;
@group(0) @binding(2) var<storage> materialArray : array<MaterialData>;

// TODO: from a stack overflow question
// let grid = &voxel_volume.indirection_pool[pool_index];
// let cell = (*grid).cells[grid_index].data;


@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
// Use the instance index to retrieve the matrix !!  indexData[in.instanceIx]

    out.position = uMyUniforms.projectionMatrix * uMyUniforms.viewMatrix * instanceArray[in.instanceIx].modelMatrix * vec4f(in.position, 1.0);
	// Forward the normal TODO: pass in a normal "pose" matrix this uses a square root.
    out.normal = normalize((instanceArray[in.instanceIx].modelMatrix * vec4f(in.normal, 0.0)).xyz);
	out.materialIx = instanceArray[in.instanceIx].materialId;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
	let normal = in.normal;
    let material = materialArray[in.materialIx]; // indirection or by value ? or is it a reference ?

    // for testing
    return vec4f(material.diffuse,1.0);

//	let lightColor1 = vec3f(1.0, 1.0, 1.0);
//	let lightColor2 = vec3f(0.4, 0.4, 0.6);
//	let lightDirection1 = normalize(vec3f(0.5, -0.1, -0.4));
//	let lightDirection2 = normalize(vec3f(-0.5, 0.1, 0.3));
//	let shading1 = max(0.0, dot(lightDirection1, normal));
//	let shading2 = max(0.0, dot(lightDirection2, normal));
//
//	let shading = shading1 * lightColor1 + shading2 * lightColor2;
//
//	var color = vec3(.8,0.8,0.8) * shading;
//    if(color.x > 1.0) {
//        color.x = 1.0;
//    }
//    if(color.y > 1.0) {
//        color.y = 1.0;
//    }
//    if(color.z > 1.0) {
//        color.z = 1.0;
//    }
//
//	// Gamma-correction
//	let corrected_color = pow(color, vec3f(2.2));
//    return vec4f(corrected_color,1.0); // corrected_color, uMyUniforms.color.a);


}
)";


ShaderModule
ShaderManager::loadShaderModule(const fs::path& path) {

    const char *shaderCode = "";
    std::string shaderSource(" ");

    if( path == "pyramid056.wgsl") {
        shaderCode = pyramid056ShaderSource;
    } else {

        std::ifstream file(path);
        if (!file.is_open()) {
            return nullptr;
        }
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        shaderSource.resize(size, ' ');
        file.seekg(0);
        file.read(shaderSource.data(), size);
        shaderCode = shaderSource.c_str();
    }
	ShaderModuleWGSLDescriptor shaderCodeDesc;
	shaderCodeDesc.chain.next = nullptr;
	shaderCodeDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
	shaderCodeDesc.code = shaderCode;
	ShaderModuleDescriptor shaderDesc;
	shaderDesc.nextInChain = &shaderCodeDesc.chain;
#ifdef WEBGPU_BACKEND_WGPU
	shaderDesc.hintCount = 0;
	shaderDesc.hints = nullptr;
#endif

	return(device_.createShaderModule(shaderDesc));
}


ARTD_END
