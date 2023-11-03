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

static const char* test1Shader =
#include "./shaders/testShader1.wgsl"

ShaderModule
ShaderManager::loadShaderModule(const fs::path& path) {

    const char *shaderCode = "";
    std::string shaderSource("");

    if( path == "testShader1.wgsl") {
        shaderCode = test1Shader;
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
        // TODO: preprocessing needs to be handled in resource target in build
        // skip over leading R(" if present for the "includeable" shaders
        if(shaderCode[0] == 'R'
            && shaderCode[1] == '\"'
            && shaderCode[2] == ')'
           )
         {
            shaderCode += 3;
         }
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
