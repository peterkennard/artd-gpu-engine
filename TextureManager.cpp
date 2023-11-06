#include "./GpuEngineImpl.h"
#include "./TextureManager.h"
#include "artd/Logger.h"
#include <map>
#include "artd/RcString.h"
#include <string>

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE

TextureManager::TextureManager() {
}
TextureManager::~TextureManager() {
}



class TextureManagerImpl
    : public TextureManager
{
    GpuEngineImpl &owner_;
    
    INL wgpu::Device &device() {
        return(owner_.device);
    }
    class CachedTexture;
    
    typedef std::map<RcString,WeakPtr<CachedTexture>>  TMapT;

    class CachedTexture
        : public Texture
    {
    public:
        TextureManagerImpl *owner;
        const TMapT::key_type *pKey;
        INL void setTexture(wgpu::Texture t) {
            tex_ = t;
        }
        ~CachedTexture() override {
            owner->onTextureDestroy(this);
        }
        const char *getName() override {
            return(pKey->c_str());
        }
    };

    class ViewKey {
    public:
        uint64_t pTex; // pointer to the gpu texture
        uint64_t vdKey;
        
        INL ViewKey() : pTex(0), vdKey(0)
        {}
        
        ViewKey(const wgpu::TextureViewDescriptor &tvd, wgpu::Texture t) {
            pTex = (uint64_t)(void *)t;
            uint8_t *bytes = (uint8_t *)&vdKey;
            bytes[0] = (uint8_t)(tvd.aspect);  // always is "all" so coudl be used somewhere else
            bytes[1] = (uint8_t)(tvd.baseArrayLayer);
            bytes[2] = (uint8_t)(tvd.arrayLayerCount);
            bytes[3] = (uint8_t)(tvd.baseMipLevel);
            bytes[4] = (uint8_t)(tvd.mipLevelCount);
            bytes[5] = (uint8_t)(tvd.dimension); // always 2D ? cube maps ?
            bytes[6] = (uint8_t)(tvd.format);
            bytes[7] = 0;
        }
        struct Less
        {
            INL bool operator()(const ViewKey &a, const ViewKey &b) const {
                return((a.pTex < b.pTex) && (a.vdKey < b.vdKey));
            }
        };
        INL bool operator==(const ViewKey &b) const {
            return((pTex == b.pTex) && (vdKey == b.vdKey));
        }
    };

    
    typedef std::map<ViewKey,WeakPtr<TextureView>>  VMapT;

    class CachedTextureView
        : public TextureView
    {
    public:
        TextureManagerImpl *owner;
        const VMapT::key_type *pKey;
        INL void setTextureView(wgpu::TextureView t) {
            view_ = t;
        }
        INL void setTexture(ObjectPtr<Texture> t) {
            viewed_ = t;
        }
        ~CachedTextureView() override {
            owner->onTextureViewDestroy(this);
        }
    };

    TMapT cached_;
    VMapT cachedViews_;

    void onTextureDestroy(CachedTexture *ct) {
        auto found = cached_.find(*(ct->pKey));
        if(found != cached_.end()) {
            cached_.erase(found);
        }
    }
    void onTextureViewDestroy(CachedTextureView *ct) {
        auto found = cachedViews_.find(*(ct->pKey));
        if(found != cachedViews_.end()) {
            cachedViews_.erase(found);
        }
    }

    ObjectPtr<CachedTexture> generateTest0() {
    
        using namespace wgpu;

        ObjectPtr<CachedTexture> tex = ObjectBase::make<CachedTexture>();

        // todo: pick best size.
        uint32_t width = 0x40;  // 64
        uint32_t height = 0x40;

        TextureDescriptor renderTextureDesc;
        renderTextureDesc.dimension = TextureDimension::_2D;
        renderTextureDesc.format = TextureFormat::RGBA8Unorm;
        renderTextureDesc.mipLevelCount = 1;
        renderTextureDesc.sampleCount = 1;
        renderTextureDesc.size = { width, height, 1 };
        renderTextureDesc.usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding | TextureUsage::CopyDst;
        renderTextureDesc.viewFormatCount = 0;
        renderTextureDesc.viewFormats = nullptr;
        tex->tex_ = device().createTexture(renderTextureDesc);

        AD_LOG(print) << "view desc size " << sizeof(TextureViewDescriptor);

        //
        TextureViewDescriptor renderTextureViewDesc;
                renderTextureViewDesc.aspect = TextureAspect::All;
                renderTextureViewDesc.baseArrayLayer = 0;
                renderTextureViewDesc.arrayLayerCount = 1;
                renderTextureViewDesc.baseMipLevel = 0;
                renderTextureViewDesc.mipLevelCount = 1;
                renderTextureViewDesc.dimension = TextureViewDimension::_2D;
                renderTextureViewDesc.format = renderTextureDesc.format;

        AD_LOG(print) << "view desc size " << sizeof(TextureViewDescriptor);

        
        auto view = tex->tex_.createView(renderTextureViewDesc);

        AD_LOG(print) << "view " << (void *)view;
  

    //        // Create test image data -
    //        std::vector<uint8_t> pixels(4 * width * height);
    //        for (uint32_t i = 0; i < width; ++i) {
    //            for (uint32_t j = 0; j < height; ++j) {
    //                uint8_t *p = &pixels[4 * (j * width + i)];
    //                p[0] = (i / 16) % 2 == (j / 16) % 2 ? 255 : 0; // r
    //                p[1] = ((i - j) / 16) % 2 == 0 ? 255 : 0; // g
    //                p[2] = ((i + j) / 16) % 2 == 0 ? 255 : 0; // b
    //                p[3] = 255; // a
    //            }
    //        }
    //
    //
    //        // Upload texture data
    //        // Arguments telling which part of the texture we upload to
    //        // (together with the last argument of writeTexture)
    //        ImageCopyTexture destination;
    //        destination.texture = tex->tex_;
    //        destination.mipLevel = 0;
    //        destination.origin = { 0, 0, 0 }; // equivalent of the offset argument of Queue::writeBuffer
    //        destination.aspect = TextureAspect::All; // only relevant for depth/Stencil textures
    //
    //        // Arguments telling how the C++ side pixel memory is laid out
    //        TextureDataLayout source;
    //        source.offset = 0;
    //        source.bytesPerRow = 4 * renderTextureDesc.size.width;
    //        source.rowsPerImage = renderTextureDesc.size.height;
    //
    //        // Issue commands
    //        Queue queue = device().getQueue();
    //        queue.writeTexture(destination, pixels.data(), pixels.size(), source, renderTextureDesc.size);

        return(tex); //tex);
    }
    
public:
    
    TextureManagerImpl(GpuEngineImpl *owner)
        : owner_(*owner)
    {
    }
    
    void loadTexture( StringArg pathName,  const std::function<void(ObjectPtr<Texture>) > &onDone) override
    {
//        std::string path(pathName.c_str());
        RcString path(pathName);
        // TODO: make thread safe do async load
        auto found = cached_.find(path);

        ObjectPtr<CachedTexture> ret;
        
        if(found != cached_.end()) {
            ret = (found->second).lock();
        } else {
            if(path == "test0") {
                ret = generateTest0();
            }
            if(ret) {
                ret->owner = this;
                auto inserted = cached_.insert(MapT::value_type(path, WeakPtr<CachedTexture>(ret)));
                MapT::iterator it = inserted.first;
                ret->pKey = &(it->first);  // actuall address of key in map no-reallocs
            }
        }
        onDone(ret);
    }

};


ObjectPtr<TextureManager> TextureManager::create(GpuEngineImpl *owner) {
    return(ObjectBase::make<TextureManagerImpl>(owner));
}


ARTD_END
