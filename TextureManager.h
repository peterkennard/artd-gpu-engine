#pragma once

#include "artd/ObjectBase.h"
#include "artd/StringArg.h"
#include "./Texture.h"

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE

class GpuEngineImpl;
class TextureManagerImpl;

class TextureManager
{
protected:
    TextureManager();
    TextureManagerImpl &impl();
    const TextureManagerImpl &impl() const;

    ObjectPtr<TextureView> nullTexView_;
    ObjectPtr<Texture> nullTexture_;
public:
    virtual ~TextureManager();
    virtual void shutdown() = 0;

    static ObjectPtr<TextureManager> create(GpuEngineImpl *owner);
    
    virtual void loadTexture( StringArg pathName,  const std::function<void(ObjectPtr<Texture>)> &onDone) = 0;
    virtual void loadBindableTexture( StringArg pathName, const std::function<void(ObjectPtr<TextureView>) > &onDone, const wgpu::TextureViewDescriptor *tvd = nullptr) = 0;

    INL void loadBindableTexture( StringArg pathName, const std::function<void(ObjectPtr<TextureView>) > &onDone, const wgpu::TextureViewDescriptor &tvd) {
        loadBindableTexture(pathName, onDone, &tvd);
    };
\
    INL ObjectPtr<TextureView> &getNullTextureView() {
        return(nullTexView_);
    }
    INL ObjectPtr<Texture> &getNullTexture() {
        return(nullTexture_);
    }
};


#undef INL

ARTD_END
