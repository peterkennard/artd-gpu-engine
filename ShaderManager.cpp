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

/// *********************** simple triangle shader

static const char* triangleShaderSource = R"(
@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4<f32> {
var p = vec2f(0.0, 0.0);
if (in_vertex_index == 0u) {
    p = vec2f(-0.5, -0.5);
} else if (in_vertex_index == 1u) {
    p = vec2f(0.5, -0.5);
} else {
    p = vec2f(0.0, 0.5);
}
return vec4f(p, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4f {
return vec4f(0.0, 0.4, 1.0, 1.0);
}
)";

/// ************************* shader for pyramid 056

static const char* pyramid056ShaderSource = R"(
struct VertexInput {
	@location(0) position: vec3f,
	@location(1) normal: vec3f, // new attribute
	@location(2) color: vec3f,
};

struct VertexOutput {
	@builtin(position) position: vec4f,
	@location(0) color: vec3f,
	@location(1) normal: vec3f, // <--- Add a normal output
};

/**
 * A structure holding the value of our uniforms
 */
struct MyUniforms {
    projectionMatrix: mat4x4f,
    viewMatrix: mat4x4f,
    modelMatrix: mat4x4f,
    color: vec4f,
    time: f32,
};

// Instead of the simple uTime variable, our uniform variable is a struct
@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
	out.position = uMyUniforms.projectionMatrix * uMyUniforms.viewMatrix * uMyUniforms.modelMatrix * vec4f(in.position, 1.0);
	// Forward the normal
    out.normal = (uMyUniforms.modelMatrix * vec4f(in.normal, 0.0)).xyz;
	out.color = in.color;
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
	let normal = normalize(in.normal);

	let lightColor1 = vec3f(1.0, 1.0, 1.0);
	let lightColor2 = vec3f(0.4, 0.4, 0.6);
	let lightDirection1 = normalize(vec3f(0.5, -0.1, -0.4));
	let lightDirection2 = normalize(vec3f(-0.5, 0.1, 0.3));
	let shading1 = max(0.0, dot(lightDirection1, normal));
	let shading2 = max(0.0, dot(lightDirection2, normal));

	let shading = shading1 * lightColor1 + shading2 * lightColor2;

	var color = vec3(.8,0.8,0.8) * shading;
    if(color.x > 1.0) {
        color.x = 1.0;
    }
    if(color.y > 1.0) {
        color.y = 1.0;
    }
    if(color.z > 1.0) {
        color.z = 1.0;
    }

	// Gamma-correction
	let corrected_color = pow(color, vec3f(2.2));
	return vec4f(corrected_color,1.0); // corrected_color, uMyUniforms.color.a);
}
)";

ShaderModule
ShaderManager::loadShaderModule(const fs::path& path) {

    const char *shaderCode = "";
    std::string shaderSource(" ");

    if( path == "pyramid056.wgsl")
        shaderCode = pyramid056ShaderSource;
    else if(path == "triangle")  {
        shaderCode = triangleShaderSource;
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
