#pragma once
#include "artd/ResourceManager.h"
#include "artd/RcString.h"
#include <filesystem>
#include <vector>
#include <map>


ARTD_BEGIN

namespace fs = std::filesystem;

#define INL ARTD_ALWAYS_INLINE

class DrawableMesh;
class GpuEngineImpl;

class CachedMeshLoader {
    GpuEngineImpl *owner_;

    class CachedMesh;
    friend class CachedMesh;

    typedef std::map<RcString,WeakPtr<CachedMesh>>  MMapT;

    MMapT cache_;
protected:
    void onMeshDestroy(CachedMesh *mesh);
public:

    INL GpuEngineImpl &owner() {
        return(*owner_);
    }

    CachedMeshLoader(GpuEngineImpl *owner)
        : owner_(owner)
    {
    }

    ~CachedMeshLoader() {
    }
    bool loadGeometry(const fs::path& path, std::vector<float>& pointData, std::vector<uint16_t>& indexData, int dimensions = 0);

// TODO: we don't have this yet
//    void asyncLoadMesh( StringArg pathName,  const std::function<void(ObjectPtr<DrawableMesh>)> &onDone);

    // direct loaded - not async.
    ObjectPtr<DrawableMesh> loadMesh( StringArg pathName);
};

#undef INL

ARTD_END
