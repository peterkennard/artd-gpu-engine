#ifndef __artd_ResourceManagerImpl_h
#define __artd_ResourceManagerImpl_h

#include "artd/ResourceManager.h"

#define USE_TEXT_INDEX32

ARTD_BEGIN

class ResourceManagerImpl
    : public ResourceManager
{
public:
    ResourceManagerImpl();
    ~ResourceManagerImpl();
    void _asyncLoadBinaryResource(StringArg path, const std::function<void (ByteArray &)> &onDone,bool onUIThread) override;
    void shutdown();
};

ARTD_END

#endif // __artd_ResourceManagerImpl_h
