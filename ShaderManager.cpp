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

// SEE: https://www.w3.org/TR/WGSL/#alignment-and-size
// for alignment specs

struct LightData {
    pose:  mat3x3f,
    position: vec3f,
    diffuse: vec3f,
    lightType: u32,
};

/**
 * A structure holding the value of scene global uniforms
 */
struct SceneUniforms {
    projectionMatrix: mat4x4f,
    viewMatrix: mat4x4f,
    vpMatrix: mat4x4f,
    eyePose:  mat4x4f,  // need inverse view ? or this ?
    test: mat4x4f,
    time: f32,
    numLights: u32,
    pad1: f32,
    pad2: f32,
    lights: array<LightData,64>
};

struct InstanceData {
    modelMatrix: mat4x4f,
    materialId: u32,
    objectId: u32,
    unused2_: u32, // pad for 16 bytes alignment (needed ? )
};

struct MaterialData {
    diffuse: vec3f,
    unused_: f32,  // id ? we need transparency ??
    specular: vec3f
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) worldPos: vec4f,
    @location(1) normal: vec3f,
    @location(2) texUV:  vec2f,  // TBD unused now
    @location(3) @interpolate(flat) materialIx: u32
};



// Bound variable is a struct
@group(0) @binding(0) var<uniform> scnUniforms: SceneUniforms;
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

    let mMat = instanceArray[in.instanceIx].modelMatrix;

    out.worldPos = mMat * vec4f(in.position, 1.0);
    out.position = scnUniforms.vpMatrix * out.worldPos;

	// Forward the normal
	// TODO: pass in a normal "pose" matrix? in instance data ?
    let nMat = mat3x3f(mMat[0].xyz, mMat[1].xyz, mMat[2].xyz);
    out.normal = normalize(nMat * in.normal);

	out.materialIx = instanceArray[in.instanceIx].materialId;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
	let normal = normalize(in.normal); // the interpolator doesn't keep it normalized !!! ie: rotate it !

    let material = materialArray[in.materialIx]; // indirection or by value ? or is it a reference ?

    var diffuseMult = vec3f(0,0,0);
    var specularMult = vec3f(0,0,0);

    for(var lix = u32(0); lix < scnUniforms.numLights; lix += 1)  {

        let light = scnUniforms.lights[lix];

        switch light.lightType {
            case 0: { // directional

                var shading = dot(light.pose[2],normal); // z axis of rotation - pre normalized
                var wrap = scnUniforms.test[0].x;
                wrap = wrap * 4.;

                //, 2.0); //* wrap; // (wrap,5.0);

                shading += wrap;
                shading *= 1.0/(1.+wrap);

//                shading += .5;
//                shading *= 1.0/1.5;

//  https://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_reflection_model
//                //Calculate the half vector between the light vector and the view vector.
//                //This is typically slower than calculating the actual reflection vector
//                // due to the normalize function's reciprocal square root
//                float3 H = normalize(lightDir + viewDir);
//
//                //Intensity of the specular light
//                float NdotH = dot(normal, H);
//                intensity = pow(saturate(NdotH), specularHardness);
//
//                //Sum up the specular light factoring
//                OUT.Specular = intensity * light.specularColor * light.specularPower / distance;
//
//      https://registry.khronos.org/OpenGL-Refpages/gl4/html/reflect.xhtml
//      reflection dir = glsl::reflect(I, N) For a given incident vector vec
//      and surface normal N reflect returns the reflection direction
//      calculated as I - 2.0 * dot(N, I) * N.
//
                let sinCompare = scnUniforms.test[0].y;

	            var incidence = dot(light.pose[2], normal);
                if(incidence > 0.) {
                    var reflectDirection = reflect(light.pose[2], normal);
                    var toView = normalize(in.worldPos.xyz - scnUniforms.eyePose[3].xyz); // vector to eye from position.

                    var d = dot(reflectDirection, toView);
                    var specPower = 0.0;

                    // 1 minute × π/(60 × 180) = 0.0002909 radians
                    // sun == 32 minutes ( 16 minutes radius )
                    // so if d > cos 16 minutes then size of sun


                     let sc = cos(sinCompare * (3.14159f/2.f)); // if(d > sinCompare)  // way bigger than sun or moon which cos(.0002909)
                     if(d > sc)
                     {
                        specPower = 1.0;
                    } // .2 * (pow(max(d, 0.0), 10.1 ) - 1060.0); // shininess);
                    if(specPower < 0.0) {
                        specPower = 0.;
                    }
                    specularMult = vec3f(specPower,specPower,specPower);
                }

//	            var incidence = dot(light.pose[2], normal);
//                if(incidence > .98) {
//                    var inverseView = scnUniforms.eyePose;
//                    var eyePos = vec3(inverseView[3].xyz);
//                    var viewNormal = normalize(eyePos - vec3f(in.position.xyz));
//                    var reflectDirection = reflect(light.pose[2], normal);
//                    var d = .2 * dot(reflectDirection, viewNormal);
//		            var specPower = pow(max(d, 0.0), .01 ); // shininess);
//		            specularMult = vec3f(specPower,specPower,specPower);
//		        }

                if(shading > 0) {
                    diffuseMult += shading;
                 }
            }
            default: {
                // do nothing !
            }
        }
    }

	var color = material.diffuse * diffuseMult + specularMult; // shading;
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
	let corrected_color = color; // pow(color, vec3f(2.2));
    return vec4f(corrected_color,1.0); // corrected_color, scnUniforms.color.a);


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
