#pragma once

#include "artd/gpu_engine.h"
#include <webgpu/webgpu.hpp>
#include "artd/Color3f.h"
#include "artd/ObjectBase.h"

ARTD_BEGIN

class TextureManagerImpl;

#define INL ARTD_ALWAYS_INLINE

class Texture
{
protected:
    friend class TextureManagerImpl;
    friend class ObjectBase;
    wgpu::Texture tex_;
    Texture();
public:
    virtual ~Texture();
    virtual const char *getName();

    INL wgpu::Texture getTexture() const {
        return(tex_);
    }
};

// debate call this BindableTexture ?
class TextureView
{
protected:
   wgpu::TextureView view_;
   ObjectPtr<Texture> viewed_;
public:
    virtual ~TextureView() = 0;
};


#undef INL

ARTD_END
