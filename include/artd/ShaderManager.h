#pragma once

#include <webgpu/webgpu.hpp>
#include "artd/ResourceManager.h"
#include <filesystem>

namespace fs = std::filesystem;

ARTD_BEGIN

using namespace wgpu;

#define INL ARTD_ALWAYS_INLINE

class ShaderManager {
    Device device_ = nullptr;
public:
    ShaderManager(Device device);
    ShaderModule loadShaderModule(const fs::path& path);
};



ARTD_END

