#pragma once
#include "artd/ResourceManager.h"
#include <filesystem>
#include <vector>


ARTD_BEGIN

namespace fs = std::filesystem;

class CachedMeshLoader {
public:
    CachedMeshLoader() {
    }
    ~CachedMeshLoader() {
    }
    bool loadGeometry(const fs::path& path, std::vector<float>& pointData, std::vector<uint16_t>& indexData, int dimensions);
};


ARTD_END
