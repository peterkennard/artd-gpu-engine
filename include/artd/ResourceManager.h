#ifndef __artd_ResourceManager_h
#define __artd_ResourceManager_h

#include "artd/gpu_engine.h"
#include "artd/RcArray.h"
#include "artd/RcString.h"
#include <functional>

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE

enum ResourceType  {
    RES_INVALID         = 0,
    RES_BUNDLE          = 1,  // A Bundle Of named Resources
    RES_DATA            = 2,  // A ByteArray - Binary data
    RES_FONT            = 3,  // A font
    RES_TEXTURE          = 4,  // A texture image ( loaded into GPU )
    RES_SHADER_PROGRAM    = 5,  // A Shader program
    RES_ATTRIBUTE_BUFFER   = 6,   // vertex/attribute buffer
    RES_INDEX_BUFFER      = 7   // index buffer
};


class ParentBuffer {
protected:
    int32_t glh_ = -1;
    friend class BufferData;
};

class BufferData {
protected:
    ParentBuffer *parent_;
    uint32_t start_ = 0;
public:
    INL int32_t getGLH() const { return(parent_->glh_); }
    INL int32_t getStartOffset() const { return(start_); }
    INL const uint8_t *getStart() const { return((const uint8_t *)((size_t)start_)); }
    INL const uint8_t *getStart(int32_t offset) const { return((const uint8_t *)((size_t)(start_ + offset))); }
};


class BufferHandle {

    const BufferData *p;
    ARTD_API_GPU_ENGINE void addRef() const;
    ARTD_API_GPU_ENGINE void release() const;

public:
    INL ~BufferHandle() {
        if(p) release();
    }

    INL BufferHandle() : p(nullptr) {}
    INL BufferHandle(BufferHandle &&src) noexcept : p(src.p) {
        src.p = nullptr;
    }
    INL explicit BufferHandle(const BufferData *d) : p(d) { if(d) addRef(); }
    INL BufferHandle(const BufferHandle &h) : BufferHandle(h.p) {}

    INL BufferHandle &operator =(BufferHandle &&src) {
        this->~BufferHandle();
        return(*new(this) BufferHandle(std::move(src)));
    }
    INL BufferHandle &operator =(const BufferData *d) {
        this->~BufferHandle();
        return(*new(this) BufferHandle(d));
    }

    INL bool operator !() { return(p == nullptr); }
    INL explicit operator bool() { return(p != nullptr); }
    INL explicit operator const BufferData *() const { return(p); }

    INL const BufferData *operator->() const { return(p); }
};


class ARTD_API_GPU_ENGINE EngineResource
    : public ObjectBase
{
protected:
    RcString name_;
    EngineResource(StringArg name) : name_(name) {
    }
    EngineResource() {
        name_ = RcString::format("%p", this);
    }
public:
    virtual ~EngineResource() override {}
    INL const RcString &getName() const {
        return(name_);
    }
    virtual ResourceType getType() const = 0;
};


class ARTD_API_GPU_ENGINE ResourceManager
{
protected:
    ResourceManager() {}
    virtual ~ResourceManager() {}
    static ResourceManager *instance_;
    virtual void _asyncLoadBinaryResource(StringArg path, const std::function<void(ByteArray &)> &onDone, bool onUIThread = true) = 0;
    virtual void _asyncLoadStringResource(StringArg path, const std::function<void(RcString &)> &onDone, bool onUIThread = true) = 0;
public:
    
    static ObjectPtr<ResourceManager> create();
    
    INL static void asyncLoadBinaryResource(StringArg path, const std::function<void(ByteArray &)> &onDone, bool onUIThread = true) {
        instance_->_asyncLoadBinaryResource(path, onDone, onUIThread);
    }
    INL static void asyncLoadStringResource(StringArg path, const std::function<void(RcString &)> &onDone, bool onUIThread = true) {
        instance_->_asyncLoadStringResource(path, onDone, onUIThread);
    }
};

#undef INL

ARTD_END

#endif // __artd_ResourceManager_h
